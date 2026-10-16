// L-Shell Calendar Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/calendar_widget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/design/typography.h"
#include "shell/design/spacing.h"
#include "shell/design/animations.h"
#include "shell/render/core/renderer.h"
#include "shell/bar/ModernDesignedWidget.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <vector>
#include "render/backend/cairo/cairo_wrapper.h"

// ============================================================================
// CalendarWidget Implementation
// ============================================================================

CalendarWidget::CalendarWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_date(""),
      m_day(""),
      m_weekday(""),
      m_icon(nullptr),
      m_dateLabel(nullptr),
      m_dayLabel(nullptr),
      m_eventLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
    updateEvents();
}

CalendarWidget::~CalendarWidget() {
    delete m_icon;
    delete m_dateLabel;
    delete m_dayLabel;
    delete m_eventLabel;
    delete m_innerRoot;
}

void CalendarWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create date label
    if (m_options.showDate) {
        m_dateLabel = new Label("");
        m_dateLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_dateLabel->setFontSize(theme.font_size_sm.size);
        m_dateLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_dateLabel->setLetterSpacing(m_options.letterSpacing);
        m_dateLabel->setLineHeight(m_options.lineHeight);
        m_dateLabel->setColor(theme.text_primary);
    }
    
    // Create day label
    if (m_options.showDay) {
        m_dayLabel = new Label("");
        m_dayLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_dayLabel->setFontSize(theme.font_size_tiny.size);
        m_dayLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_dayLabel->setLetterSpacing(m_options.letterSpacing);
        m_dayLabel->setLineHeight(m_options.lineHeight);
        m_dayLabel->setColor(theme.text_secondary);
    }
    
    // Create event label
    if (m_options.showEvents) {
        m_eventLabel = new Label("");
        m_eventLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_eventLabel->setFontSize(theme.font_size_tiny.size);
        m_eventLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_eventLabel->setLetterSpacing(m_options.letterSpacing);
        m_eventLabel->setLineHeight(m_options.lineHeight);
        m_eventLabel->setColor(theme.color_info);
    }
    
    // Update display
    updateDisplay();
    
    // Set tooltip
    setTooltip(m_lastTooltip);
    
    // Create container node
    m_innerRoot = new Node;
    m_innerRoot->setData(this);
    
    // Add children to container
    if (m_icon) m_innerRoot->addChild(m_icon->root());
    if (m_dateLabel) m_innerRoot->addChild(m_dateLabel->root());
    if (m_dayLabel) m_innerRoot->addChild(m_dayLabel->root());
    if (m_eventLabel) m_innerRoot->addChild(m_eventLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 2, theme.widget_height);
}

void CalendarWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update date/time
    m_date = getCurrentDate();
    m_day = getCurrentDay();
    m_weekday = getCurrentWeekday();
    updateEvents();
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(theme.text_primary);
    }
    
    // Update date label
    if (m_dateLabel) {
        m_dateLabel->setText(formatDate());
        m_dateLabel->setColor(theme.text_primary);
    }
    
    // Update day label
    if (m_dayLabel) {
        if (m_options.showWeekday) {
            m_dayLabel->setText(m_weekday + ", " + m_day);
        } else {
            m_dayLabel->setText(m_day);
        }
        m_dayLabel->setColor(theme.text_secondary);
    }
    
    // Update event label
    if (m_eventLabel) {
        m_eventLabel->setText(formatNextEvent());
        m_eventLabel->setColor(m_events.empty() ? theme.text_secondary : theme.color_info);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void CalendarWidget::updateEvents() {
    // In a real implementation, this would fetch from Evolution, Google Calendar, or Nextcloud
    // For now, we simulate some events
    
    m_events.clear();
    
    // Add some sample events
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    // Meeting at 14:00 if it's before 14:00
    if (now_tm->tm_hour < 14) {
        m_events.push_back({"Team Meeting", "14:00", "15:00", "Conference Room A", false});
    }
    
    // Lunch at 12:30 if it's before 12:30
    if (now_tm->tm_hour < 12 || (now_tm->tm_hour == 12 && now_tm->tm_min < 30)) {
        m_events.push_back({"Lunch", "12:30", "13:30", "Cafeteria", false});
    }
    
    // Sort events by start time
    std::sort(m_events.begin(), m_events.end(), [](const Event& a, const Event& b) {
        return a.startTime < b.startTime;
    });
}

std::string CalendarWidget::getCurrentDate() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", now_tm);
    return std::string(buffer);
}

std::string CalendarWidget::getCurrentDay() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    char buffer[10];
    strftime(buffer, sizeof(buffer), "%d", now_tm);
    return std::string(buffer);
}

std::string CalendarWidget::getCurrentWeekday() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return weekdays[now_tm->tm_wday];
}

std::string CalendarWidget::formatDate() const {
    // Use the user-provided format or default
    char buffer[50];
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    strftime(buffer, sizeof(buffer), m_options.dateFormat.c_str(), now_tm);
    return std::string(buffer);
}

std::string CalendarWidget::formatDay() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return days[now_tm->tm_wday];
}

std::string CalendarWidget::formatNextEvent() const {
    if (m_events.empty()) {
        return "No events";
    }
    
    const auto& nextEvent = m_events[0];
    if (nextEvent.allDay) {
        return "All Day: " + nextEvent.title;
    } else {
        return nextEvent.startTime + " - " + nextEvent.title;
    }
}

std::string CalendarWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "Calendar\n";
    oss << "Date: " << m_date << "\n";
    oss << "Day: " << formatDay() << "\n";
    oss << "\n";
    
    if (m_events.empty()) {
        oss << "No events scheduled for today\n";
    } else {
        oss << "Today's Events:\n";
        for (const auto& event : m_events) {
            if (event.allDay) {
                oss << "  All Day: " << event.title;
                if (!event.location.empty()) {
                    oss << " at " << event.location;
                }
            } else {
                oss << "  " << event.startTime << "-" << event.endTime << " " << event.title;
                if (!event.location.empty()) {
                    oss << " at " << event.location;
                }
            }
            oss << "\n";
        }
    }
    
    oss << "\nClick to view full calendar";
    return oss.str();
}

void CalendarWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
    const auto& theme = getDesignTheme();
    
    float paddingX = static_cast<float>(theme.widget_padding_x);
    float paddingY = static_cast<float>(theme.widget_padding_y);
    float spacing = static_cast<float>(theme.widget_spacing);
    float availableWidth = containerWidth - 2 * paddingX;
    
    float x = paddingX;
    
    // Layout icon
    if (m_icon) {
        float iconWidth = theme.font_size_sm.size * 1.5f;
        m_icon->setSize(iconWidth, containerHeight - 2 * paddingY);
        m_icon->layout(renderer, iconWidth, containerHeight - 2 * paddingY);
        m_icon->root()->setPosition({x, paddingY});
        x += iconWidth + spacing;
        availableWidth -= iconWidth + spacing;
    }
    
    // Layout date
    if (m_dateLabel) {
        float dateWidth = availableWidth * 0.5f;
        m_dateLabel->setSize(dateWidth, containerHeight - 2 * paddingY);
        m_dateLabel->layout(renderer, dateWidth, containerHeight - 2 * paddingY);
        m_dateLabel->root()->setPosition({x, paddingY});
        x += dateWidth + spacing;
        availableWidth -= dateWidth + spacing;
    }
    
    // Layout day
    if (m_dayLabel) {
        float dayWidth = availableWidth * 0.3f;
        m_dayLabel->setSize(dayWidth, containerHeight - 2 * paddingY);
        m_dayLabel->layout(renderer, dayWidth, containerHeight - 2 * paddingY);
        m_dayLabel->root()->setPosition({x, paddingY});
        x += dayWidth + spacing;
        availableWidth -= dayWidth + spacing;
    }
    
    // Layout event
    if (m_eventLabel) {
        m_eventLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_eventLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_eventLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void CalendarWidget::doUpdate(Renderer& renderer) {
    // Update events periodically
    updateEvents();
    updateDisplay();
}

void CalendarWidget::onThemeChanged(const lshell::design::Theme& theme) {
    if (m_icon) {
        m_icon->setColor(theme.text_primary);
        m_icon->queueRedraw();
    }
    
    if (m_dateLabel) {
        m_dateLabel->setColor(theme.text_primary);
        m_dateLabel->queueRedraw();
    }
    
    if (m_dayLabel) {
        m_dayLabel->setColor(theme.text_secondary);
        m_dayLabel->queueRedraw();
    }
    
    if (m_eventLabel) {
        m_eventLabel->setColor(m_events.empty() ? theme.text_secondary : theme.color_info);
        m_eventLabel->queueRedraw();
    }
}

void CalendarWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// CalendarWidget::Options Builder
// ============================================================================

CalendarWidget::Options CalendarWidget::OptionsBuilder::build() const {
    return options;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::showDate(bool show) {
    options.showDate = show;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::showDay(bool show) {
    options.showDay = show;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::showEvents(bool show) {
    options.showEvents = show;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::showWeekday(bool show) {
    options.showWeekday = show;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::dateFormat(std::string format) {
    options.dateFormat = std::move(format);
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

CalendarWidget::OptionsBuilder& CalendarWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
