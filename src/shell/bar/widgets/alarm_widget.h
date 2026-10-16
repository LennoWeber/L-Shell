// L-Shell Alarm Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>

class Label;
class Node;

// ============================================================================
// AlarmWidget - New feature for productivity
// Displays and manages alarms, timers, and stopwatch
// ============================================================================

class AlarmWidget : public ModernDesignedWidget {
public:
    struct Alarm {
        std::string name;
        int hour;
        int minute;
        bool enabled;
        bool repeatDaily;
        bool triggered;
    };

    struct Timer {
        std::string name;
        int remainingSeconds;
        bool running;
    };

    enum class DisplayMode {
        NEXT_ALARM,
        TIMER,
        STOPWATCH
    };

    struct Options {
        bool showIcon = true;
        bool showNextAlarm = true;
        bool showTimer = true;
        bool showStopwatch = false;
        std::string alarmIcon = "⏰";
        std::string timerIcon = "⏳";
        std::string stopwatchIcon = "⏱️";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 1000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showNextAlarm(bool show);
        OptionsBuilder& showTimer(bool show);
        OptionsBuilder& showStopwatch(bool show);
        OptionsBuilder& withAlarmIcon(std::string icon);
        OptionsBuilder& withTimerIcon(std::string icon);
        OptionsBuilder& withStopwatchIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    AlarmWidget(wl_output* output = nullptr, Options options = {});
    ~AlarmWidget() override;

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
    void updateAlarms();
    void updateTimers();
    void updateStopwatch();
    std::string getDisplayIcon() const;
    std::string formatNextAlarm() const;
    std::string formatTimer() const;
    std::string formatStopwatch() const;
    std::string formatTooltip() const;
    void checkAlarms();
    void notifyAlarm(const Alarm& alarm);
    void startTimer(int seconds);
    void pauseTimer();
    void resetTimer();
    void startStopwatch();
    void pauseStopwatch();
    void resetStopwatch();

private:
    Options m_options;
    
    // Alarm state
    std::vector<Alarm> m_alarms;
    std::vector<Timer> m_timers;
    int m_stopwatchSeconds;
    bool m_stopwatchRunning;
    DisplayMode m_displayMode;
    
    // UI elements
    Label* m_icon;
    Label* m_timeLabel;
    Label* m_infoLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
