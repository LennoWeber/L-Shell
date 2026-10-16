// L-Shell Pomodoro Timer Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/pomodoro_widget.h"
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
#include "render/backend/cairo/cairo_wrapper.h"

// ============================================================================
// PomodoroWidget Implementation
// ============================================================================

PomodoroWidget::PomodoroWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_state(PomodoroState::IDLE),
      m_remainingSeconds(0),
      m_completedSessions(0),
      m_lastTick(std::chrono::system_clock::now()),
      m_icon(nullptr),
      m_timerLabel(nullptr),
      m_sessionsLabel(nullptr),
      m_progressBar(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
    resetTimer();
}

PomodoroWidget::~PomodoroWidget() {
    delete m_icon;
    delete m_timerLabel;
    delete m_sessionsLabel;
    delete m_progressBar;
    delete m_innerRoot;
}

void PomodoroWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label("");
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create timer label
    if (m_options.showTimer) {
        m_timerLabel = new Label("");
        m_timerLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_timerLabel->setFontSize(theme.font_size_sm.size);
        m_timerLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_timerLabel->setLetterSpacing(m_options.letterSpacing);
        m_timerLabel->setLineHeight(m_options.lineHeight);
        m_timerLabel->setColor(theme.text_primary);
    }
    
    // Create sessions label
    if (m_options.showSessions) {
        m_sessionsLabel = new Label("");
        m_sessionsLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_sessionsLabel->setFontSize(theme.font_size_tiny.size);
        m_sessionsLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_sessionsLabel->setLetterSpacing(m_options.letterSpacing);
        m_sessionsLabel->setLineHeight(m_options.lineHeight);
        m_sessionsLabel->setColor(theme.text_secondary);
    }
    
    // Create progress bar if enabled
    if (m_options.showProgress) {
        // Progress bar implementation would go here
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
    if (m_timerLabel) m_innerRoot->addChild(m_timerLabel->root());
    if (m_sessionsLabel) m_innerRoot->addChild(m_sessionsLabel->root());
    if (m_progressBar) m_innerRoot->addChild(m_progressBar->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void PomodoroWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update icon
    if (m_icon) {
        m_icon->setText(getStateIcon());
        m_icon->setColor(getStateColor());
    }
    
    // Update timer label
    if (m_timerLabel) {
        m_timerLabel->setText(formatTime());
        m_timerLabel->setColor(getStateColor());
    }
    
    // Update sessions label
    if (m_sessionsLabel) {
        std::ostringstream oss;
        if (m_state == PomodoroState::WORKING) {
            oss << m_completedSessions << "/" << m_options.sessionsBeforeLongBreak;
        } else if (m_state == PomodoroState::SHORT_BREAK) {
            oss << "Short Break";
        } else if (m_state == PomodoroState::LONG_BREAK) {
            oss << "Long Break";
        } else if (m_state == PomodoroState::PAUSED) {
            oss << "Paused";
        } else {
            oss << "Idle";
        }
        m_sessionsLabel->setText(oss.str());
        m_sessionsLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

std::string PomodoroWidget::getStateIcon() const {
    switch (m_state) {
        case PomodoroState::WORKING: return m_options.workIcon;
        case PomodoroState::SHORT_BREAK: return m_options.breakIcon;
        case PomodoroState::LONG_BREAK: return m_options.longBreakIcon;
        case PomodoroState::PAUSED: return m_options.pauseIcon;
        default: return m_options.idleIcon;
    }
}

std::string PomodoroWidget::getStateText() const {
    switch (m_state) {
        case PomodoroState::WORKING: return "Working";
        case PomodoroState::SHORT_BREAK: return "Short Break";
        case PomodoroState::LONG_BREAK: return "Long Break";
        case PomodoroState::PAUSED: return "Paused";
        default: return "Idle";
    }
}

std::string PomodoroWidget::formatTime() const {
    int minutes = m_remainingSeconds / 60;
    int seconds = m_remainingSeconds % 60;
    
    std::ostringstream oss;
    oss << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

std::string PomodoroWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "Pomodoro: " << getStateText() << "\n";
    oss << "Time Remaining: " << formatTime() << "\n";
    oss << "Sessions: " << m_completedSessions << " / " << m_options.sessionsBeforeLongBreak << "\n";
    
    switch (m_state) {
        case PomodoroState::WORKING:
            oss << "Status: Focus time - Stay concentrated!\n";
            oss << "Next: " << (m_completedSessions + 1 >= m_options.sessionsBeforeLongBreak ? "Long Break" : "Short Break");
            break;
        case PomodoroState::SHORT_BREAK:
            oss << "Status: Short break - Relax!\n";
            oss << "Next: Work Session";
            break;
        case PomodoroState::LONG_BREAK:
            oss << "Status: Long break - Take your time!\n";
            oss << "Next: Work Session";
            break;
        case PomodoroState::PAUSED:
            oss << "Status: Paused\n";
            oss << "Click to resume";
            break;
        default:
            oss << "Status: Idle\n";
            oss << "Click to start";
    }
    
    return oss.str();
}

lshell::design::Color PomodoroWidget::getStateColor() const {
    const auto& theme = getDesignTheme();
    
    switch (m_state) {
        case PomodoroState::WORKING:
            return theme.color_primary;
        case PomodoroState::SHORT_BREAK:
        case PomodoroState::LONG_BREAK:
            return theme.color_success;
        case PomodoroState::PAUSED:
            return theme.color_warning;
        default:
            return theme.text_primary;
    }
}

void PomodoroWidget::startTimer() {
    if (m_state == PomodoroState::WORKING || m_state == PomodoroState::SHORT_BREAK || m_state == PomodoroState::LONG_BREAK) {
        return; // Already running
    }
    
    if (m_state == PomodoroState::IDLE || m_state == PomodoroState::PAUSED) {
        switchToWork();
    }
}

void PomodoroWidget::pauseTimer() {
    if (m_state == PomodoroState::PAUSED) {
        // Resume
        switch (m_state) {
            case PomodoroState::PAUSED:
                // Already paused, can't resume from here
                break;
        }
    } else if (m_state == PomodoroState::WORKING || m_state == PomodoroState::SHORT_BREAK || m_state == PomodoroState::LONG_BREAK) {
        m_state = PomodoroState::PAUSED;
        updateDisplay();
    }
}

void PomodoroWidget::resetTimer() {
    m_state = PomodoroState::IDLE;
    m_remainingSeconds = 0;
    m_completedSessions = 0;
    updateDisplay();
}

void PomodoroWidget::switchToWork() {
    m_state = PomodoroState::WORKING;
    m_remainingSeconds = m_options.workDuration * 60;
    m_lastTick = std::chrono::system_clock::now();
    updateDisplay();
    notify("Pomodoro", "Work session started - Stay focused!");
}

void PomodoroWidget::switchToShortBreak() {
    m_state = PomodoroState::SHORT_BREAK;
    m_remainingSeconds = m_options.shortBreakDuration * 60;
    m_lastTick = std::chrono::system_clock::now();
    updateDisplay();
    notify("Pomodoro", "Short break started - Take a rest!");
}

void PomodoroWidget::switchToLongBreak() {
    m_state = PomodoroState::LONG_BREAK;
    m_remainingSeconds = m_options.longBreakDuration * 60;
    m_lastTick = std::chrono::system_clock::now();
    updateDisplay();
    notify("Pomodoro", "Long break started - Relax and recharge!");
}

void PomodoroWidget::tick() {
    if (m_state == PomodoroState::PAUSED || m_state == PomodoroState::IDLE) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastTick);
    
    if (elapsed.count() >= 1) {
        m_remainingSeconds -= elapsed.count();
        m_lastTick = now;
        
        if (m_remainingSeconds <= 0) {
            // Time's up!
            if (m_state == PomodoroState::WORKING) {
                m_completedSessions++;
                if (m_completedSessions >= m_options.sessionsBeforeLongBreak) {
                    m_completedSessions = 0;
                    switchToLongBreak();
                } else {
                    switchToShortBreak();
                }
            } else {
                switchToWork();
            }
        }
        
        updateDisplay();
    }
}

void PomodoroWidget::updateTimer() {
    // Tick the timer
    tick();
}

void PomodoroWidget::notify(const std::string& title, const std::string& message) {
    // In a real implementation, this would show a notification
    // For now, we just print to console (in simulation)
    printf("Pomodoro Notification: %s - %s\n", title.c_str(), message.c_str());
}

void PomodoroWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
    
    // Layout timer
    if (m_timerLabel) {
        float timerWidth = availableWidth * 0.5f;
        m_timerLabel->setSize(timerWidth, containerHeight - 2 * paddingY);
        m_timerLabel->layout(renderer, timerWidth, containerHeight - 2 * paddingY);
        m_timerLabel->root()->setPosition({x, paddingY});
        x += timerWidth + spacing;
        availableWidth -= timerWidth + spacing;
    }
    
    // Layout sessions
    if (m_sessionsLabel) {
        m_sessionsLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_sessionsLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_sessionsLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void PomodoroWidget::doUpdate(Renderer& renderer) {
    // Update timer
    updateTimer();
    
    // Update display
    updateDisplay();
}

void PomodoroWidget::onPrimaryClick() {
    if (m_state == PomodoroState::IDLE) {
        startTimer();
    } else if (m_state == PomodoroState::PAUSED) {
        // Resume from pause
        switch (m_state) {
            case PomodoroState::PAUSED:
                // This would need to restore the previous state
                break;
        }
    } else if (m_state == PomodoroState::WORKING || m_state == PomodoroState::SHORT_BREAK || m_state == PomodoroState::LONG_BREAK) {
        pauseTimer();
    }
}

void PomodoroWidget::onSecondaryClick() {
    // Right-click to reset
    resetTimer();
}

void PomodoroWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_timerLabel) m_timerLabel->queueRedraw();
    if (m_sessionsLabel) m_sessionsLabel->queueRedraw();
}

void PomodoroWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// PomodoroWidget::Options Builder
// ============================================================================

PomodoroWidget::Options PomodoroWidget::OptionsBuilder::build() const {
    return options;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::showTimer(bool show) {
    options.showTimer = show;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::showSessions(bool show) {
    options.showSessions = show;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::showProgress(bool show) {
    options.showProgress = show;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::workDuration(int minutes) {
    options.workDuration = minutes;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::shortBreakDuration(int minutes) {
    options.shortBreakDuration = minutes;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::longBreakDuration(int minutes) {
    options.longBreakDuration = minutes;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::sessionsBeforeLongBreak(int count) {
    options.sessionsBeforeLongBreak = count;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::withWorkIcon(std::string icon) {
    options.workIcon = std::move(icon);
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::withBreakIcon(std::string icon) {
    options.breakIcon = std::move(icon);
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::withLongBreakIcon(std::string icon) {
    options.longBreakIcon = std::move(icon);
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::withPauseIcon(std::string icon) {
    options.pauseIcon = std::move(icon);
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::withIdleIcon(std::string icon) {
    options.idleIcon = std::move(icon);
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

PomodoroWidget::OptionsBuilder& PomodoroWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
