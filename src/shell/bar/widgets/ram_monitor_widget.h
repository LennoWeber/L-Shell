// L-Shell RAM Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <string>
#include <memory>

class Label;
class Node;
class ProgressBar;

// ============================================================================
// RAMMonitorWidget - New feature for extended functionality
// Displays RAM usage, swap usage, and memory pressure
// ============================================================================

class RAMMonitorWidget : public ModernDesignedWidget {
public:
    struct Options {
        bool showIcon = true;
        bool showPercentage = true;
        bool showUsage = true;
        bool showSwap = true;
        bool showPressure = false;
        std::string icon = "🧠";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 1000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showPercentage(bool show);
        OptionsBuilder& showUsage(bool show);
        OptionsBuilder& showSwap(bool show);
        OptionsBuilder& showPressure(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    RAMMonitorWidget(wl_output* output = nullptr, Options options = {});
    ~RAMMonitorWidget() override;

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
    void updateRAMStatus();
    std::string formatRAMText() const;
    std::string formatTooltip() const;
    lshell::design::Color getUsageColor(float percentage) const;
    std::string formatBytes(float bytes) const;

private:
    Options m_options;
    
    // RAM state
    float m_usage;              // RAM usage percentage (0-100)
    float m_usedGB;             // Used RAM in GB
    float m_totalGB;            // Total RAM in GB
    float m_swapUsage;          // Swap usage percentage (0-100)
    float m_swapUsedGB;         // Used swap in GB
    float m_swapTotalGB;        // Total swap in GB
    bool m_pressureWarning;     // Memory pressure warning
    std::string m_ramType;      // RAM type (DDR4, DDR5, etc.)
    
    // UI elements
    Label* m_icon;
    Label* m_usageLabel;
    Label* m_usageValueLabel;
    Label* m_swapLabel;
    Label* m_pressureLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
