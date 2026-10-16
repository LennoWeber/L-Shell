// Display Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#pragma once

#include "shell/control_center/ModernControlCenter.h"
#include "shell/design/theme.h"
#include "ui/controls/label.h"
#include "ui/controls/slider.h"
#include "ui/controls/switch.h"
#include "ui/controls/button.h"

#include <vector>
#include <string>

namespace lshell::control_center {

// ============================================================================
// DisplayTab - Displays display and brightness controls
// ============================================================================

class DisplayTab : public ControlCenterTab {
public:
    DisplayTab();
    ~DisplayTab() override = default;

    void create() override;
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;

private:
    // UI Elements
    ui::Label* m_titleLabel = nullptr;
    
    // Brightness Section
    ui::Label* m_brightnessSectionLabel = nullptr;
    ui::Label* m_brightnessLabel = nullptr;
    ui::Slider* m_brightnessSlider = nullptr;
    ui::Label* m_brightnessValue = nullptr;
    
    // Night Light Section
    ui::Label* m_nightlightSectionLabel = nullptr;
    ui::Switch* m_nightlightSwitch = nullptr;
    ui::Label* m_nightlightStatus = nullptr;
    ui::Label* m_nightlightTemperatureLabel = nullptr;
    ui::Slider* m_nightlightTemperatureSlider = nullptr;
    ui::Label* m_nightlightTemperatureValue = nullptr;
    
    // Display Arrangement Section
    ui::Label* m_arrangementSectionLabel = nullptr;
    ui::Button* m_arrangementButton = nullptr;
    
    // Refresh Rate Section
    ui::Label* m_refreshRateSectionLabel = nullptr;
    ui::Label* m_refreshRateLabel = nullptr;
    
    void updateDisplayStatus();
    void applyModernTheme();
};

} // namespace lshell::control_center
