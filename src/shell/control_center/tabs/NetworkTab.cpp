// Network Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/tabs/NetworkTab.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

namespace lshell::control_center {

// ============================================================================
// NetworkTab Implementation
// ============================================================================

NetworkTab::NetworkTab() 
    : ControlCenterTab(Options{
        .name = "network",
        .icon = "network-wired-symbolic",
        .title = "Network"
    }) {
}

void NetworkTab::create() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& typography = getTypography();
    const auto& spacing = getSpacing();
    
    // Create title
    m_titleLabel = new ui::Label("Network Connections");
    m_titleLabel->setFontFamily(typography.heading.font_family);
    m_titleLabel->setFontSize(typography.heading.font_size);
    m_titleLabel->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_titleLabel->setColor(colors.text_primary);
    addChild(m_titleLabel->root());
    
    // WiFi Section
    auto* wifiSectionLabel = new ui::Label("WiFi");
    wifiSectionLabel->setFontFamily(typography.subheading.font_family);
    wifiSectionLabel->setFontSize(typography.subheading.font_size);
    wifiSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    wifiSectionLabel->setColor(colors.text_primary);
    addChild(wifiSectionLabel->root());
    
    m_wifiLabel = new ui::Label("WiFi:");
    m_wifiLabel->setFontFamily(typography.body.font_family);
    m_wifiLabel->setFontSize(typography.body.font_size);
    m_wifiLabel->setColor(colors.text_secondary);
    addChild(m_wifiLabel->root());
    
    m_wifiSwitch = new ui::Switch;
    m_wifiSwitch->setActive(true);
    m_wifiSwitch->onToggled = [this](bool active) {
        // Toggle WiFi
    };
    addChild(m_wifiSwitch->root());
    
    m_wifiStatus = new ui::Label("Connected");
    m_wifiStatus->setFontFamily(typography.body.font_family);
    m_wifiStatus->setFontSize(typography.body.font_size);
    m_wifiStatus->setColor(colors.status_success);
    addChild(m_wifiStatus->root());
    
    m_wifiSsid = new ui::Label("SSID: MyNetwork");
    m_wifiSsid->setFontFamily(typography.body_small.font_family);
    m_wifiSsid->setFontSize(typography.body_small.font_size);
    m_wifiSsid->setColor(colors.text_secondary);
    addChild(m_wifiSsid->root());
    
    m_wifiSignal = new ui::Label("Signal: 85%");
    m_wifiSignal->setFontFamily(typography.body_small.font_family);
    m_wifiSignal->setFontSize(typography.body_small.font_size);
    m_wifiSignal->setColor(colors.text_secondary);
    addChild(m_wifiSignal->root());
    
    // Ethernet Section
    auto* ethernetSectionLabel = new ui::Label("Ethernet");
    ethernetSectionLabel->setFontFamily(typography.subheading.font_family);
    ethernetSectionLabel->setFontSize(typography.subheading.font_size);
    ethernetSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    ethernetSectionLabel->setColor(colors.text_primary);
    addChild(ethernetSectionLabel->root());
    
    m_ethernetLabel = new ui::Label("Ethernet:");
    m_ethernetLabel->setFontFamily(typography.body.font_family);
    m_ethernetLabel->setFontSize(typography.body.font_size);
    m_ethernetLabel->setColor(colors.text_secondary);
    addChild(m_ethernetLabel->root());
    
    m_ethernetStatus = new ui::Label("Disconnected");
    m_ethernetStatus->setFontFamily(typography.body.font_family);
    m_ethernetStatus->setFontSize(typography.body.font_size);
    m_ethernetStatus->setColor(colors.text_secondary);
    addChild(m_ethernetStatus->root());
    
    m_ethernetSpeed = new ui::Label("Speed: N/A");
    m_ethernetSpeed->setFontFamily(typography.body_small.font_family);
    m_ethernetSpeed->setFontSize(typography.body_small.font_size);
    m_ethernetSpeed->setColor(colors.text_secondary);
    addChild(m_ethernetSpeed->root());
    
    // VPN Section
    auto* vpnSectionLabel = new ui::Label("VPN");
    vpnSectionLabel->setFontFamily(typography.subheading.font_family);
    vpnSectionLabel->setFontSize(typography.subheading.font_size);
    vpnSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    vpnSectionLabel->setColor(colors.text_primary);
    addChild(vpnSectionLabel->root());
    
    m_vpnLabel = new ui::Label("VPN:");
    m_vpnLabel->setFontFamily(typography.body.font_family);
    m_vpnLabel->setFontSize(typography.body.font_size);
    m_vpnLabel->setColor(colors.text_secondary);
    addChild(m_vpnLabel->root());
    
    m_vpnSwitch = new ui::Switch;
    m_vpnSwitch->setActive(false);
    m_vpnSwitch->onToggled = [this](bool active) {
        // Toggle VPN
    };
    addChild(m_vpnSwitch->root());
    
    m_vpnStatus = new ui::Label("Disconnected");
    m_vpnStatus->setFontFamily(typography.body.font_family);
    m_vpnStatus->setFontSize(typography.body.font_size);
    m_vpnStatus->setColor(colors.text_secondary);
    addChild(m_vpnStatus->root());
    
    // Bluetooth Section
    auto* bluetoothSectionLabel = new ui::Label("Bluetooth");
    bluetoothSectionLabel->setFontFamily(typography.subheading.font_family);
    bluetoothSectionLabel->setFontSize(typography.subheading.font_size);
    bluetoothSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    bluetoothSectionLabel->setColor(colors.text_primary);
    addChild(bluetoothSectionLabel->root());
    
    m_bluetoothLabel = new ui::Label("Bluetooth:");
    m_bluetoothLabel->setFontFamily(typography.body.font_family);
    m_bluetoothLabel->setFontSize(typography.body.font_size);
    m_bluetoothLabel->setColor(colors.text_secondary);
    addChild(m_bluetoothLabel->root());
    
    m_bluetoothSwitch = new ui::Switch;
    m_bluetoothSwitch->setActive(true);
    m_bluetoothSwitch->onToggled = [this](bool active) {
        // Toggle Bluetooth
    };
    addChild(m_bluetoothSwitch->root());
    
    m_bluetoothStatus = new ui::Label("Enabled");
    m_bluetoothStatus->setFontFamily(typography.body.font_family);
    m_bluetoothStatus->setFontSize(typography.body.font_size);
    m_bluetoothStatus->setColor(colors.status_success);
    addChild(m_bluetoothStatus->root());
    
    // Mobile Data Section
    auto* mobileSectionLabel = new ui::Label("Mobile Data");
    mobileSectionLabel->setFontFamily(typography.subheading.font_family);
    mobileSectionLabel->setFontSize(typography.subheading.font_size);
    mobileSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    mobileSectionLabel->setColor(colors.text_primary);
    addChild(mobileSectionLabel->root());
    
    m_mobileLabel = new ui::Label("Mobile Data:");
    m_mobileLabel->setFontFamily(typography.body.font_family);
    m_mobileLabel->setFontSize(typography.body.font_size);
    m_mobileLabel->setColor(colors.text_secondary);
    addChild(m_mobileLabel->root());
    
    m_mobileSwitch = new ui::Switch;
    m_mobileSwitch->setActive(false);
    m_mobileSwitch->onToggled = [this](bool active) {
        // Toggle Mobile Data
    };
    addChild(m_mobileSwitch->root());
    
    m_mobileStatus = new ui::Label("Disabled");
    m_mobileStatus->setFontFamily(typography.body.font_family);
    m_mobileStatus->setFontSize(typography.body.font_size);
    m_mobileStatus->setColor(colors.text_secondary);
    addChild(m_mobileStatus->root());
    
    applyModernTheme();
    updateNetworkStatus();
}

void NetworkTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float padding = static_cast<float>(spacing.lg);
    float sectionSpacing = static_cast<float>(spacing.xl);
    float itemSpacing = static_cast<float>(spacing.md);
    float labelWidth = 120.0f;
    
    float y = padding;
    
    // Title
    m_titleLabel->setSize(width - 2 * padding, 30);
    m_titleLabel->layout(renderer, width - 2 * padding, 30);
    m_titleLabel->root()->setPosition({padding, y});
    y += 30 + sectionSpacing;
    
    // WiFi Section
    auto* wifiSectionLabel = static_cast<ui::Label*>(getChildren()[1]);
    wifiSectionLabel->setSize(width - 2 * padding, 24);
    wifiSectionLabel->layout(renderer, width - 2 * padding, 24);
    wifiSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // WiFi controls
    auto layoutRow = [&](ui::Label* label, ui::Switch* sw, ui::Label* status, float& y_pos) {
        label->setSize(labelWidth, 24);
        label->layout(renderer, labelWidth, 24);
        label->root()->setPosition({padding, y_pos});
        
        sw->setSize(40, 20);
        sw->layout(renderer, 40, 20);
        sw->root()->setPosition({padding + labelWidth + itemSpacing, y_pos});
        
        status->setSize(100, 24);
        status->layout(renderer, 100, 24);
        status->root()->setPosition({padding + labelWidth + itemSpacing + 40 + itemSpacing, y_pos});
        
        y_pos += 24 + itemSpacing;
    };
    
    layoutRow(m_wifiLabel, m_wifiSwitch, m_wifiStatus, y);
    
    // WiFi details
    auto layoutDetailRow = [&](ui::Label* label, float& y_pos) {
        label->setSize(width - 2 * padding, 20);
        label->layout(renderer, width - 2 * padding, 20);
        label->root()->setPosition({padding + labelWidth + itemSpacing, y_pos});
        y_pos += 20;
    };
    
    layoutDetailRow(m_wifiSsid, y);
    layoutDetailRow(m_wifiSignal, y);
    y += itemSpacing;
    
    // Ethernet Section
    auto* ethernetSectionLabel = static_cast<ui::Label*>(getChildren()[7]);
    ethernetSectionLabel->setSize(width - 2 * padding, 24);
    ethernetSectionLabel->layout(renderer, width - 2 * padding, 24);
    ethernetSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Ethernet status
    m_ethernetLabel->setSize(labelWidth, 24);
    m_ethernetLabel->layout(renderer, labelWidth, 24);
    m_ethernetLabel->root()->setPosition({padding, y});
    
    m_ethernetStatus->setSize(150, 24);
    m_ethernetStatus->layout(renderer, 150, 24);
    m_ethernetStatus->root()->setPosition({padding + labelWidth + itemSpacing, y});
    y += 24 + itemSpacing;
    
    layoutDetailRow(m_ethernetSpeed, y);
    y += itemSpacing;
    
    // VPN Section
    auto* vpnSectionLabel = static_cast<ui::Label*>(getChildren()[11]);
    vpnSectionLabel->setSize(width - 2 * padding, 24);
    vpnSectionLabel->layout(renderer, width - 2 * padding, 24);
    vpnSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    layoutRow(m_vpnLabel, m_vpnSwitch, m_vpnStatus, y);
    y += itemSpacing;
    
    // Bluetooth Section
    auto* bluetoothSectionLabel = static_cast<ui::Label*>(getChildren()[15]);
    bluetoothSectionLabel->setSize(width - 2 * padding, 24);
    bluetoothSectionLabel->layout(renderer, width - 2 * padding, 24);
    bluetoothSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    layoutRow(m_bluetoothLabel, m_bluetoothSwitch, m_bluetoothStatus, y);
    y += itemSpacing;
    
    // Mobile Data Section
    auto* mobileSectionLabel = static_cast<ui::Label*>(getChildren()[19]);
    mobileSectionLabel->setSize(width - 2 * padding, 24);
    mobileSectionLabel->layout(renderer, width - 2 * padding, 24);
    mobileSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    layoutRow(m_mobileLabel, m_mobileSwitch, m_mobileStatus, y);
}

void NetworkTab::doUpdate(Renderer& renderer) {
    // Update network status periodically
    static int counter = 0;
    if (counter % 60 == 0) { // Update every ~60 frames (~1 second)
        updateNetworkStatus();
    }
    counter++;
}

void NetworkTab::updateNetworkStatus() {
    // This would normally query NetworkManager or similar
    // For now, we'll use mock data
    
    // Update WiFi status
    m_wifiSwitch->setActive(true);
    m_wifiStatus->setText("Connected");
    m_wifiStatus->setColor(getColorPalette().status_success);
    
    // Update Ethernet status
    m_ethernetStatus->setText("Disconnected");
    m_ethernetStatus->setColor(getColorPalette().text_secondary);
    
    // Update VPN status
    m_vpnSwitch->setActive(false);
    m_vpnStatus->setText("Disconnected");
    m_vpnStatus->setColor(getColorPalette().text_secondary);
    
    // Update Bluetooth status
    m_bluetoothSwitch->setActive(true);
    m_bluetoothStatus->setText("Enabled");
    m_bluetoothStatus->setColor(getColorPalette().status_success);
    
    // Update Mobile Data status
    m_mobileSwitch->setActive(false);
    m_mobileStatus->setText("Disabled");
    m_mobileStatus->setColor(getColorPalette().text_secondary);
}

void NetworkTab::applyModernTheme() {
    const auto& colors = getColorPalette();
    
    setBackgroundColor(colors.background_primary);
    
    if (m_wifiSwitch) m_wifiSwitch->setActiveColor(colors.accent_primary);
    if (m_vpnSwitch) m_vpnSwitch->setActiveColor(colors.accent_secondary);
    if (m_bluetoothSwitch) m_bluetoothSwitch->setActiveColor(colors.accent_tertiary);
    if (m_mobileSwitch) m_mobileSwitch->setActiveColor(colors.accent_quaternary);
}

} // namespace lshell::control_center
