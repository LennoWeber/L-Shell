// L-Shell Pomodoro Timer Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <string>
#include <memory>
#include <chrono>

class Label;
class Node;
class ProgressBar;

// ============================================================================
// PomodoroWidget - New feature for productivity
// Implements the Pomodoro technique: 25-minute focused work, 5-minute breaks
// ============================================================================

class PomodoroWidget : public ModernDesignedWidget {
public:
    enum class PomodoroState {
        WORKING,
        SHORT_BREAK,
        LONG_BREAK,
        IDLE,
        PAUSED
    };

    struct Options {
        bool showIcon = true;
        bool showTimer = true;
        bool showSessions = true;
        bool showProgress = false;
        int workDuration = 25;   // minutes
        int shortBreakDuration = 5; // minutes
        int longBreakDuration = 15; // minutes
        int sessionsBeforeLongBreak = 4;
        std::string workIcon = "⏳";
        std::string breakIcon = "☕";
        std::string longBreakIcon = "🏖️";
        std::string pauseIcon = "⏸️";
        std::string idleIcon = "⏰";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 1000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showTimer(bool show);
        OptionsBuilder& showSessions(bool show);
        OptionsBuilder& showProgress(bool show);
        OptionsBuilder& workDuration(int minutes);
        OptionsBuilder& shortBreakDuration(int minutes);
        OptionsBuilder& longBreakDuration(int minutes);
        OptionsBuilder& sessionsBeforeLongBreak(int count);
        OptionsBuilder& withWorkIcon(std::string icon);
        OptionsBuilder& withBreakIcon(std::string icon);
        OptionsBuilder& withLongBreakIcon(std::string icon);
        OptionsBuilder& withPauseIcon(std::string icon);
        OptionsBuilder& withIdleIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    PomodoroWidget(wl_output* output = nullptr, Options options = {});
    ~PomodoroWidget() override;

    void create() override;
    void applyCommonOptions(
        const CommonWidgetOptions& options, FontWeight barFontWeight, 
        const std::string& barFontFamily, std::string_view logContext) override;

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;
    
    // Widget actions
    void onPrimaryClick() override;
    void onSecondaryClick() override;

private:
    void updateDisplay();
    void updateTimer();
    void startTimer();
    void pauseTimer();
    void resetTimer();
    void switchToShortBreak();
    void switchToLongBreak();
    void switchToWork();
    void tick();
    std::string getStateIcon() const;
    std::string getStateText() const;
    std::string formatTime() const;
    std::string formatTooltip() const;
    lshell::design::Color getStateColor() const;
    void notify(const std::string& title, const std::string& message);

private:
    Options m_options;
    
    // Pomodoro state
    PomodoroState m_state;
    int m_remainingSeconds;
    int m_completedSessions;
    std::chrono::system_clock::time_point m_lastTick;
    
    // UI elements
    Label* m_icon;
    Label* m_timerLabel;
    Label* m_sessionsLabel;
    ProgressBar* m_progressBar;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
