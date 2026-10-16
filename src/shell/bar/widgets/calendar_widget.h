// L-Shell Calendar Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <string>
#include <vector>
#include <memory>

class Label;
class Node;

// ============================================================================
// CalendarWidget - New feature for productivity
// Displays current date, day of week, and upcoming events
// ============================================================================

class CalendarWidget : public ModernDesignedWidget {
public:
    struct Event {
        std::string title;
        std::string startTime;
        std::string endTime;
        std::string location;
        bool allDay;
    };

    struct Options {
        bool showIcon = true;
        bool showDate = true;
        bool showDay = true;
        bool showEvents = true;
        bool showWeekday = false;
        std::string icon = "📅";
        std::string dateFormat = "%d. %B %Y"; // DD. MonthName YYYY
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 60000; // 1 minute
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showDate(bool show);
        OptionsBuilder& showDay(bool show);
        OptionsBuilder& showEvents(bool show);
        OptionsBuilder& showWeekday(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& dateFormat(std::string format);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    CalendarWidget(wl_output* output = nullptr, Options options = {});
    ~CalendarWidget() override;

    void create() override;
    void applyCommonOptions(
        const CommonWidgetOptions& options, FontWeight barFontWeight, 
        const std::string& barFontFamily, std::string_view logContext) override;

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;

private:
    void updateDisplay();
    void updateEvents();
    std::string getCurrentDate() const;
    std::string getCurrentDay() const;
    std::string getCurrentWeekday() const;
    std::string formatDate() const;
    std::string formatDay() const;
    std::string formatNextEvent() const;
    std::string formatTooltip() const;

private:
    Options m_options;
    
    // Calendar state
    std::string m_date;
    std::string m_day;
    std::string m_weekday;
    std::vector<Event> m_events;
    
    // UI elements
    Label* m_icon;
    Label* m_dateLabel;
    Label* m_dayLabel;
    Label* m_eventLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
