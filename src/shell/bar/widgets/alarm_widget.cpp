// L-Shell Alarm Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/alarm_widget.h"
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
#include "render/backend/cairo/cairo_wrapper.h"

// ============================================================================
// AlarmWidget Implementation
// ============================================================================

AlarmWidget::AlarmWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_stopwatchSeconds(0),
      m_stopwatchRunning(false),
      m_displayMode(DisplayMode::NEXT_ALARM),
      m_icon(nullptr),
      m_timeLabel(nullptr),
      m_infoLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
    
    // Add some default alarms
    m_alarms.push_back({"Morning", 7, 0, true, true, false});
    m_alarms.push_back({"Meeting", 14, 30, true, false, false});
    
    // Start a default timer
    startTimer(60 * 5); // 5 minutes
}

AlarmWidget::~AlarmWidget() {
    delete m_icon;
    delete m_timeLabel;
    delete m_infoLabel;
    delete m_innerRoot;
}

void AlarmWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label("");
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create time label
    if (m_options.showNextAlarm || m_options.showTimer || m_options.showStopwatch) {
        m_timeLabel = new Label("");
        m_timeLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_timeLabel->setFontSize(theme.font_size_sm.size);
        m_timeLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_timeLabel->setLetterSpacing(m_options.letterSpacing);
        m_timeLabel->setLineHeight(m_options.lineHeight);
        m_timeLabel->setColor(theme.text_primary);
    }
    
    // Create info label
    if (m_options.showNextAlarm) {
        m_infoLabel = new Label("");
        m_infoLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_infoLabel->setFontSize(theme.font_size_tiny.size);
        m_infoLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_infoLabel->setLetterSpacing(m_options.letterSpacing);
        m_infoLabel->setLineHeight(m_options.lineHeight);
        m_infoLabel->setColor(theme.text_secondary);
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
    if (m_timeLabel) m_innerRoot->addChild(m_timeLabel->root());
    if (m_infoLabel) m_innerRoot->addChild(m_infoLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void AlarmWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update alarms and timers
    updateAlarms();
    updateTimers();
    updateStopwatch();
    
    // Update icon
    if (m_icon) {
        m_icon->setText(getDisplayIcon());
        m_icon->setColor(theme.color_info);
    }
    
    // Update time label based on display mode
    if (m_timeLabel) {
        switch (m_displayMode) {
            case DisplayMode::NEXT_ALARM:
                m_timeLabel->setText(formatNextAlarm());
                break;
            case DisplayMode::TIMER:
                m_timeLabel->setText(formatTimer());
                break;
            case DisplayMode::STOPWATCH:
                m_timeLabel->setText(formatStopwatch());
                break;
        }
        m_timeLabel->setColor(theme.text_primary);
    }
    
    // Update info label
    if (m_infoLabel) {
        switch (m_displayMode) {
            case DisplayMode::NEXT_ALARM:
                m_infoLabel->setText(formatNextAlarm());
                break;
            case DisplayMode::TIMER:
                m_infoLabel->setText("Timer");
                break;
            case DisplayMode::STOPWATCH:
                m_infoLabel->setText("Stopwatch");
                break;
        }
        m_infoLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void AlarmWidget::updateAlarms() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    for (auto& alarm : m_alarms) {
        if (!alarm.enabled || alarm.triggered) continue;
        
        // Check if alarm should trigger
        if (now_tm->tm_hour == alarm.hour && now_tm->tm_min == alarm.minute) {
            alarm.triggered = true;
            notifyAlarm(alarm);
        }
    }
    
    // Reset triggered alarms if they repeat daily
    for (auto& alarm : m_alarms) {
        if (alarm.triggered && alarm.repeatDaily) {
            alarm.triggered = false;
        }
    }
}

void AlarmWidget::updateTimers() {
    if (m_timers.empty()) return;
    
    for (auto& timer : m_timers) {
        if (!timer.running) continue;
        
        timer.remainingSeconds--;
        if (timer.remainingSeconds <= 0) {
            timer.running = false;
            // Timer finished - would play sound in real implementation
        }
    }
}

void AlarmWidget::updateStopwatch() {
    if (!m_stopwatchRunning) return;
    
    static auto lastTick = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTick);
    
    if (elapsed.count() >= 1) {
        m_stopwatchSeconds += elapsed.count();
        lastTick = now;
    }
}

std::string AlarmWidget::getDisplayIcon() const {
    switch (m_displayMode) {
        case DisplayMode::NEXT_ALARM:
            return m_options.alarmIcon;
        case DisplayMode::TIMER:
            return m_options.timerIcon;
        case DisplayMode::STOPWATCH:
            return m_options.stopwatchIcon;
        default:
            return m_options.alarmIcon;
    }
}

std::string AlarmWidget::formatNextAlarm() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    
    // Find the next alarm
    const Alarm* nextAlarm = nullptr;
    int minDiff = 24 * 60; // 24 hours in minutes
    
    for (const auto& alarm : m_alarms) {
        if (!alarm.enabled) continue;
        
        int totalMinutes = alarm.hour * 60 + alarm.minute;
        int nowMinutes = now_tm->tm_hour * 60 + now_tm->tm_min;
        
        int diff = totalMinutes - nowMinutes;
        if (diff < 0) diff += 24 * 60; // Tomorrow
        
        if (diff < minDiff) {
            minDiff = diff;
            nextAlarm = &alarm;
        }
    }
    
    if (nextAlarm) {
        std::ostringstream oss;
        oss << nextAlarm->name << " at " << std::setw(2) << std::setfill('0') << nextAlarm->hour
            << ":" << std::setw(2) << std::setfill('0') << nextAlarm->minute;
        return oss.str();
    }
    
    return "No alarms";
}

std::string AlarmWidget::formatTimer() const {
    if (m_timers.empty() || !m_timers[0].running) {
        return "0:00";
    }
    
    int seconds = m_timers[0].remainingSeconds;
    int minutes = seconds / 60;
    int secs = seconds % 60;
    
    std::ostringstream oss;
    oss << minutes << ":" << std::setw(2) << std::setfill('0') << secs;
    return oss.str();
}

std::string AlarmWidget::formatStopwatch() const {
    int minutes = m_stopwatchSeconds / 60;
    int seconds = m_stopwatchSeconds % 60;
    
    std::ostringstream oss;
    oss << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

std::string AlarmWidget::formatTooltip() const {
    std::ostringstream oss;
    
    switch (m_displayMode) {
        case DisplayMode::NEXT_ALARM:
            oss << "Alarms\n";
            for (const auto& alarm : m_alarms) {
                std::string status = alarm.enabled ? "Enabled" : "Disabled";
                if (alarm.triggered) status = "TRIGGERED";
                oss << "  " << std::setw(2) << std::setfill('0') << alarm.hour
                    << ":" << std::setw(2) << std::setfill('0') << alarm.minute
                    << " " << alarm.name << " (" << status << ")\n";
            }
            oss << "\nClick to add alarm";
            break;
        
        case DisplayMode::TIMER:
            oss << "Timer\n";
            for (size_t i = 0; i < m_timers.size(); ++i) {
                oss << "  Timer " << i + 1 << ": " << formatTimer() << "\n";
            }
            oss << "\nClick to start timer";
            break;
        
        case DisplayMode::STOPWATCH:
            oss << "Stopwatch\n";
            oss << "  Time: " << formatStopwatch() << "\n";
            oss << "\nClick to start/stop";
            break;
    }
    
    return oss.str();
}

void AlarmWidget::checkAlarms() {
    // Check if any alarms should trigger
    updateAlarms();
}

void AlarmWidget::notifyAlarm(const Alarm& alarm) {
    // In a real implementation, this would show a notification
    printf("Alarm: %s at %02d:%02d\n", alarm.name.c_str(), alarm.hour, alarm.minute);
}

void AlarmWidget::startTimer(int seconds) {
    if (m_timers.empty()) {
        m_timers.push_back({"Timer 1", seconds, true});
    } else {
        m_timers[0].remainingSeconds = seconds;
        m_timers[0].running = true;
    }
    m_displayMode = DisplayMode::TIMER;
    updateDisplay();
}

void AlarmWidget::pauseTimer() {
    if (!m_timers.empty()) {
        m_timers[0].running = false;
    }
    updateDisplay();
}

void AlarmWidget::resetTimer() {
    if (!m_timers.empty()) {
        m_timers[0].remainingSeconds = 0;
        m_timers[0].running = false;
    }
    updateDisplay();
}

void AlarmWidget::startStopwatch() {
    m_stopwatchRunning = true;
    m_displayMode = DisplayMode::STOPWATCH;
    updateDisplay();
}

void AlarmWidget::pauseStopwatch() {
    m_stopwatchRunning = false;
    updateDisplay();
}

void AlarmWidget::resetStopwatch() {
    m_stopwatchSeconds = 0;
    m_stopwatchRunning = false;
    updateDisplay();
}

void AlarmWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
    
    // Layout time
    if (m_timeLabel) {
        float timeWidth = availableWidth * 0.5f;
        m_timeLabel->setSize(timeWidth, containerHeight - 2 * paddingY);
        m_timeLabel->layout(renderer, timeWidth, containerHeight - 2 * paddingY);
        m_timeLabel->root()->setPosition({x, paddingY});
        x += timeWidth + spacing;
        availableWidth -= timeWidth + spacing;
    }
    
    // Layout info
    if (m_infoLabel) {
        m_infoLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_infoLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_infoLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void AlarmWidget::doUpdate(Renderer& renderer) {
    // Update timers and stopwatch
    updateTimers();
    updateStopwatch();
    checkAlarms();
    
    // Update display
    updateDisplay();
}

void AlarmWidget::onPrimaryClick() {
    // Cycle through display modes
    switch (m_displayMode) {
        case DisplayMode::NEXT_ALARM:
            m_displayMode = DisplayMode::TIMER;
            break;
        case DisplayMode::TIMER:
            m_displayMode = DisplayMode::STOPWATCH;
            break;
        case DisplayMode::STOPWATCH:
            m_displayMode = DisplayMode::NEXT_ALARM;
            break;
    }
    updateDisplay();
}

void AlarmWidget::onSecondaryClick() {
    // Right-click to start/stop
    switch (m_displayMode) {
        case DisplayMode::NEXT_ALARM:
            // Toggle first alarm
            if (!m_alarms.empty()) {
                m_alarms[0].enabled = !m_alarms[0].enabled;
            }
            break;
        case DisplayMode::TIMER:
            if (!m_timers.empty()) {
                if (m_timers[0].running) {
                    pauseTimer();
                } else {
                    startTimer(m_timers[0].remainingSeconds > 0 ? m_timers[0].remainingSeconds : 300); // 5 minutes
                }
            }
            break;
        case DisplayMode::STOPWATCH:
            if (m_stopwatchRunning) {
                pauseStopwatch();
            } else {
                startStopwatch();
            }
            break;
    }
    updateDisplay();
}

void AlarmWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_timeLabel) m_timeLabel->queueRedraw();
    if (m_infoLabel) m_infoLabel->queueRedraw();
}

void AlarmWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// AlarmWidget::Options Builder
// ============================================================================

AlarmWidget::Options AlarmWidget::OptionsBuilder::build() const {
    return options;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::showNextAlarm(bool show) {
    options.showNextAlarm = show;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::showTimer(bool show) {
    options.showTimer = show;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::showStopwatch(bool show) {
    options.showStopwatch = show;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::withAlarmIcon(std::string icon) {
    options.alarmIcon = std::move(icon);
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::withTimerIcon(std::string icon) {
    options.timerIcon = std::move(icon);
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::withStopwatchIcon(std::string icon) {
    options.stopwatchIcon = std::move(icon);
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

AlarmWidget::OptionsBuilder& AlarmWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
