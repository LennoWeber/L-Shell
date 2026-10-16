// L-Shell Disk Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
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
// DiskMonitorWidget - New feature for extended functionality
// Displays disk usage, read/write speeds, and health
// ============================================================================

class DiskMonitorWidget : public ModernDesignedWidget {
public:
    struct Disk {
        std::string name;
        std::string mountPoint;
        float usagePercentage;
        float usedGB;
        float totalGB;
        float readSpeedMB;
        float writeSpeedMB;
        int temperature;
        bool isSSD;
    };

    struct Options {
        bool showIcon = true;
        bool showUsage = true;
        bool showSpeed = true;
        bool showTemperature = false;
        bool showMultipleDisks = false;
        std::string icon = "💾";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 2000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showUsage(bool show);
        OptionsBuilder& showSpeed(bool show);
        OptionsBuilder& showTemperature(bool show);
        OptionsBuilder& showMultipleDisks(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    DiskMonitorWidget(wl_output* output = nullptr, Options options = {});
    ~DiskMonitorWidget() override;

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
    void updateDiskStatus();
    std::string formatDiskText() const;
    std::string formatTooltip() const;
    lshell::design::Color getUsageColor(float percentage) const;
    std::string formatBytes(float bytes) const;

private:
    Options m_options;
    
    // Disk state
    std::vector<Disk> m_disks;
    int m_activeDiskIndex;
    float m_readSpeed;   // Combined read speed in MB/s
    float m_writeSpeed;  // Combined write speed in MB/s
    
    // UI elements
    Label* m_icon;
    Label* m_usageLabel;
    Label* m_speedLabel;
    Label* m_tempLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
