#pragma once

// Modern Clock Widget - Example Implementation using L-Shell Design System
// This is a reference implementation showing how to use the new design system

#include "shell/design/theme.h"
#include "shell/bar/widget.h"

#include <string>

/**
 * ModernClockWidget - A clock widget implementing the new L-Shell design system
 * 
 * Features:
 * - Modern, minimalist design
 * - Smooth animations
 * - Theme-aware colors
 * - Consistent spacing and typography
 */
class ModernClockWidget : public Widget {
public:
    struct Options {
        std::string time_format = "%H:%M";      // 24-hour format by default
        std::string date_format = "%a, %b %d";   // Abbreviated day, month, day
        bool show_date = false;                  // Show date below time
        bool show_seconds = false;               // Show seconds
        bool use_24_hour = true;                 // 24-hour vs 12-hour format
    };

    ModernClockWidget(const Options& options = {});
    ~ModernClockWidget() override;

    void create() override;

    // Set time update interval (default: every minute)
    void setUpdateInterval(int seconds);

    // Set format options
    void setTimeFormat(const std::string& format);
    void setDateFormat(const std::string& format);
    void setShowDate(bool show);
    void setShowSeconds(bool show);
    void set24HourFormat(bool use_24_hour);

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    bool onPointerEvent(const PointerEvent& event) override;

private:
    // Update the displayed time
    void updateTime();

    // Apply theme colors
    void applyThemeColors();

    // Theme change listener
    void onThemeChanged(const lshell::design::Theme& theme);

    // Design system references
    const lshell::design::Theme& getTheme() const;

    // Options
    Options m_options;

    // State
    std::string m_currentTime;
    std::string m_currentDate;
    int m_updateInterval = 60;  // Update every minute by default

    // Visual elements (to be implemented with actual rendering)
    // These would be pointers to your rendering primitives
    // For this example, we'll use placeholders

    // Cached theme values
    lshell::design::Color m_textColor;
    lshell::design::Color m_secondaryTextColor;
    lshell::design::FontFamily m_fontFamily;
    lshell::design::FontSize m_timeFontSize;
    lshell::design::FontSize m_dateFontSize;
    lshell::design::Spacing m_horizontalPadding;
    lshell::design::Spacing m_verticalPadding;
    lshell::design::Spacing m_spacing;
};
