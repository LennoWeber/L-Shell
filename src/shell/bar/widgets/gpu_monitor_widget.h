// L-Shell GPU Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
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
// GPU Monitor Widget - New feature for extended functionality
// Displays GPU usage, VRAM, temperature, and fan speed
// ============================================================================

class GPUMonitorWidget : public ModernDesignedWidget {
public:
    struct Options {
        bool showIcon = true;
        bool showPercentage = true;
        bool showVRAM = true;
        bool showTemperature = true;
        bool showFanSpeed = false;
        bool compactMode = true;
        std::string icon = "🖥️";
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
        OptionsBuilder& showVRAM(bool show);
        OptionsBuilder& showTemperature(bool show);
        OptionsBuilder& showFanSpeed(bool show);
        OptionsBuilder& compactMode(bool compact);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    GPUMonitorWidget(wl_output* output = nullptr, Options options = {});
    ~GPUMonitorWidget() override;

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
    void updateGPUStatus();
    std::string formatGPUText() const;
    std::string formatTooltip() const;
    lshell::design::Color getUsageColor(float percentage) const;

private:
    Options m_options;
    
    // GPU state
    float m_usage;          // GPU usage percentage (0-100)
    float m_vramUsed;       // VRAM used in GB
    float m_vramTotal;      // Total VRAM in GB
    float m_temperature;    // Temperature in °C
    int m_fanSpeed;         // Fan speed in RPM
    std::string m_gpuName;  // GPU model name
    
    // UI elements
    Label* m_icon;
    Label* m_usageLabel;
    Label* m_vramLabel;
    Label* m_tempLabel;
    Label* m_fanLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
