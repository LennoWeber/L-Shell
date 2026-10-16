// L-Shell System Monitor Widget - FULL IMPLEMENTATION with Design System
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
class ProgressBar;

// ============================================================================
// SystemMonitorWidget - Full implementation with new design system
// Displays CPU, RAM, GPU, Disk, and Temperature information
// ============================================================================

class SystemMonitorWidget : public ModernDesignedWidget {
public:
    struct Metric {
        std::string name;
        std::string icon;
        float value;      // Percentage (0-100)
        float usage;      // Actual usage value
        std::string unit;
        lshell::design::Color color;
    };

    struct Options {
        bool showCPU = true;
        bool showRAM = true;
        bool showGPU = true;
        bool showDisk = true;
        bool showTemperature = true;
        bool showIcons = true;
        bool showPercentages = true;
        bool showUsage = true;
        bool compactMode = true;  // Show all in one line
        bool showGraphs = false;   // Show sparkline graphs
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 1000; // Update interval for metrics
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showCPU(bool show);
        OptionsBuilder& showRAM(bool show);
        OptionsBuilder& showGPU(bool show);
        OptionsBuilder& showDisk(bool show);
        OptionsBuilder& showTemperature(bool show);
        OptionsBuilder& showIcons(bool show);
        OptionsBuilder& showPercentages(bool show);
        OptionsBuilder& showUsage(bool show);
        OptionsBuilder& compactMode(bool compact);
        OptionsBuilder& showGraphs(bool show);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    SystemMonitorWidget(wl_output* output = nullptr, Options options = {});
    ~SystemMonitorWidget() override;

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
    void updateMetrics();
    void updateColors();
    std::string formatMetric(const Metric& metric) const;
    std::string formatTooltip() const;
    lshell::design::Color getMetricColor(float percentage) const;

private:
    Options m_options;
    
    // System metrics
    Metric m_cpuMetric;
    Metric m_ramMetric;
    Metric m_gpuMetric;
    Metric m_diskMetric;
    Metric m_tempMetric;
    
    // UI elements
    Label* m_cpuLabel;
    Label* m_ramLabel;
    Label* m_gpuLabel;
    Label* m_diskLabel;
    Label* m_tempLabel;
    Label* m_icon;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
