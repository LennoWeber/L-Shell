// Network Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#pragma once

#include "shell/control_center/ModernControlCenter.h"
#include "shell/design/theme.h"
#include "ui/controls/label.h"
#include "ui/controls/switch.h"

#include <vector>
#include <string>

namespace lshell::control_center {

// ============================================================================
// NetworkTab - Displays network connections and controls
// ============================================================================

class NetworkTab : public ControlCenterTab {
public:
    NetworkTab();
    ~NetworkTab() override = default;

    void create() override;
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;

private:
    // UI Elements
    ui::Label* m_titleLabel = nullptr;
    
    // WiFi Section
    ui::Label* m_wifiLabel = nullptr;
    ui::Switch* m_wifiSwitch = nullptr;
    ui::Label* m_wifiStatus = nullptr;
    ui::Label* m_wifiSsid = nullptr;
    ui::Label* m_wifiSignal = nullptr;
    
    // Ethernet Section
    ui::Label* m_ethernetLabel = nullptr;
    ui::Label* m_ethernetStatus = nullptr;
    ui::Label* m_ethernetSpeed = nullptr;
    
    // VPN Section
    ui::Label* m_vpnLabel = nullptr;
    ui::Switch* m_vpnSwitch = nullptr;
    ui::Label* m_vpnStatus = nullptr;
    
    // Bluetooth Section
    ui::Label* m_bluetoothLabel = nullptr;
    ui::Switch* m_bluetoothSwitch = nullptr;
    ui::Label* m_bluetoothStatus = nullptr;
    
    // Mobile Data Section
    ui::Label* m_mobileLabel = nullptr;
    ui::Switch* m_mobileSwitch = nullptr;
    ui::Label* m_mobileStatus = nullptr;
    
    void updateNetworkStatus();
    void applyModernTheme();
};

} // namespace lshell::control_center
