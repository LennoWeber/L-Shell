// L-Shell CPU Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
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
// CPUMonitorWidget - New feature for extended functionality
// Displays CPU usage per core and overall
// ============================================================================

class CPUMonitorWidget : public ModernDesignedWidget {
public:
    struct Options {
        bool showIcon = true;
        bool showOverall = true;
        bool showPerCore = false;
        bool showTemperature = true;
        bool showFrequency = false;
        std::string icon = "⚡";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 1000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showOverall(bool show);
        OptionsBuilder& showPerCore(bool show);
        OptionsBuilder& showTemperature(bool show);
        OptionsBuilder& showFrequency(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    CPUMonitorWidget(wl_output* output = nullptr, Options options = {});
    ~CPUMonitorWidget() override;

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
    void updateCPUStatus();
    std::string formatCPUText() const;
    std::string formatTooltip() const;
    lshell::design::Color getUsageColor(float percentage) const;

private:
    Options m_options;
    
    // CPU state
    float m_usage;              // Overall CPU usage percentage (0-100)
    std::vector<float> m_coreUsage; // Per-core usage
    float m_temperature;        // Temperature in °C
    float m_frequency;         // Frequency in GHz
    std::string m_cpuName;      // CPU model name
    
    // UI elements
    Label* m_icon;
    Label* m_usageLabel;
    Label* m_tempLabel;
    Label* m_freqLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
