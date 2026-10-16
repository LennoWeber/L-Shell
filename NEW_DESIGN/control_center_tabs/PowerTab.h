// Power Tab - Modern Control Center
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
// PowerTab - Displays power information and controls
// ============================================================================

class PowerTab : public ControlCenterTab {
public:
    PowerTab();
    ~PowerTab() override = default;

    void create() override;
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;

private:
    // UI Elements
    ui::Label* m_titleLabel = nullptr;
    
    // Battery Section
    ui::Label* m_batterySectionLabel = nullptr;
    ui::Label* m_batteryStatus = nullptr;
    ui::Label* m_batteryPercentage = nullptr;
    ui::Slider* m_batterySlider = nullptr;
    ui::Label* m_batteryTime = nullptr;
    
    // Power Profile Section
    ui::Label* m_profileSectionLabel = nullptr;
    ui::Button* m_powerProfileButton = nullptr;
    ui::Label* m_powerProfileStatus = nullptr;
    
    // Power Actions Section
    ui::Label* m_actionsSectionLabel = nullptr;
    ui::Button* m_suspendButton = nullptr;
    ui::Button* m_hibernateButton = nullptr;
    ui::Button* m_rebootButton = nullptr;
    ui::Button* m_shutdownButton = nullptr;
    
    void updatePowerStatus();
    void applyModernTheme();
};

} // namespace lshell::control_center
