// Power Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/tabs/PowerTab.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

namespace lshell::control_center {

// ============================================================================
// PowerTab Implementation
// ============================================================================

PowerTab::PowerTab() 
    : ControlCenterTab(Options{
        .name = "power",
        .icon = "system-shutdown-symbolic",
        .title = "Power"
    }) {
}

void PowerTab::create() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& typography = getTypography();
    const auto& spacing = getSpacing();
    
    // Create title
    m_titleLabel = new ui::Label("Power Management");
    m_titleLabel->setFontFamily(typography.heading.font_family);
    m_titleLabel->setFontSize(typography.heading.font_size);
    m_titleLabel->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_titleLabel->setColor(colors.text_primary);
    addChild(m_titleLabel->root());
    
    // Battery Section
    m_batterySectionLabel = new ui::Label("Battery");
    m_batterySectionLabel->setFontFamily(typography.subheading.font_family);
    m_batterySectionLabel->setFontSize(typography.subheading.font_size);
    m_batterySectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_batterySectionLabel->setColor(colors.text_primary);
    addChild(m_batterySectionLabel->root());
    
    m_batteryStatus = new ui::Label("Status: Discharging");
    m_batteryStatus->setFontFamily(typography.body.font_family);
    m_batteryStatus->setFontSize(typography.body.font_size);
    m_batteryStatus->setColor(colors.text_primary);
    addChild(m_batteryStatus->root());
    
    m_batterySlider = new ui::Slider;
    m_batterySlider->setValue(0.85f);
    m_batterySlider->setHeight(12);
    m_batterySlider->setEnabled(false);
    addChild(m_batterySlider->root());
    
    m_batteryPercentage = new ui::Label("85%");
    m_batteryPercentage->setFontFamily(typography.heading.font_family);
    m_batteryPercentage->setFontSize(typography.heading.font_size);
    m_batteryPercentage->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_batteryPercentage->setColor(colors.status_success);
    addChild(m_batteryPercentage->root());
    
    m_batteryTime = new ui::Label("~2 hours 30 minutes remaining");
    m_batteryTime->setFontFamily(typography.body_small.font_family);
    m_batteryTime->setFontSize(typography.body_small.font_size);
    m_batteryTime->setColor(colors.text_secondary);
    addChild(m_batteryTime->root());
    
    // Power Profile Section
    m_profileSectionLabel = new ui::Label("Power Profile");
    m_profileSectionLabel->setFontFamily(typography.subheading.font_family);
    m_profileSectionLabel->setFontSize(typography.subheading.font_size);
    m_profileSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_profileSectionLabel->setColor(colors.text_primary);
    addChild(m_profileSectionLabel->root());
    
    m_powerProfileButton = new ui::Button;
    m_powerProfileButton->setText("Balanced");
    m_powerProfileButton->setCornerRadius(8);
    m_powerProfileButton->onClick = [this]() {
        // Cycle through power profiles
    };
    addChild(m_powerProfileButton->root());
    
    m_powerProfileStatus = new ui::Label("Recommended for typical use");
    m_powerProfileStatus->setFontFamily(typography.body_small.font_family);
    m_powerProfileStatus->setFontSize(typography.body_small.font_size);
    m_powerProfileStatus->setColor(colors.text_secondary);
    addChild(m_powerProfileStatus->root());
    
    // Power Actions Section
    m_actionsSectionLabel = new ui::Label("Power Actions");
    m_actionsSectionLabel->setFontFamily(typography.subheading.font_family);
    m_actionsSectionLabel->setFontSize(typography.subheading.font_size);
    m_actionsSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_actionsSectionLabel->setColor(colors.text_primary);
    addChild(m_actionsSectionLabel->root());
    
    // Action buttons
    m_suspendButton = new ui::Button;
    m_suspendButton->setText("Suspend");
    m_suspendButton->setIcon("system-suspend-symbolic");
    m_suspendButton->setCornerRadius(8);
    m_suspendButton->setStyle(ui::Button::Style::Secondary);
    m_suspendButton->onClick = [this]() {
        // Suspend system
    };
    addChild(m_suspendButton->root());
    
    m_hibernateButton = new ui::Button;
    m_hibernateButton->setText("Hibernate");
    m_hibernateButton->setIcon("system-hibernate-symbolic");
    m_hibernateButton->setCornerRadius(8);
    m_hibernateButton->setStyle(ui::Button::Style::Secondary);
    m_hibernateButton->onClick = [this]() {
        // Hibernate system
    };
    addChild(m_hibernateButton->root());
    
    m_rebootButton = new ui::Button;
    m_rebootButton->setText("Restart");
    m_rebootButton->setIcon("system-reboot-symbolic");
    m_rebootButton->setCornerRadius(8);
    m_rebootButton->setStyle(ui::Button::Style::Secondary);
    m_rebootButton->onClick = [this]() {
        // Reboot system
    };
    addChild(m_rebootButton->root());
    
    m_shutdownButton = new ui::Button;
    m_shutdownButton->setText("Shutdown");
    m_shutdownButton->setIcon("system-shutdown-symbolic");
    m_shutdownButton->setCornerRadius(8);
    m_shutdownButton->setStyle(ui::Button::Style::Danger);
    m_shutdownButton->onClick = [this]() {
        // Shutdown system
    };
    addChild(m_shutdownButton->root());
    
    applyModernTheme();
    updatePowerStatus();
}

void PowerTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float padding = static_cast<float>(spacing.lg);
    float sectionSpacing = static_cast<float>(spacing.xl);
    float itemSpacing = static_cast<float>(spacing.md);
    float buttonWidth = 140.0f;
    
    float y = padding;
    
    // Title
    m_titleLabel->setSize(width - 2 * padding, 30);
    m_titleLabel->layout(renderer, width - 2 * padding, 30);
    m_titleLabel->root()->setPosition({padding, y});
    y += 30 + sectionSpacing;
    
    // Battery Section
    m_batterySectionLabel->setSize(width - 2 * padding, 24);
    m_batterySectionLabel->layout(renderer, width - 2 * padding, 24);
    m_batterySectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Battery status
    m_batteryStatus->setSize(width - 2 * padding, 24);
    m_batteryStatus->layout(renderer, width - 2 * padding, 24);
    m_batteryStatus->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Battery slider
    m_batterySlider->setSize(width - 4 * padding, 12);
    m_batterySlider->layout(renderer, width - 4 * padding, 12);
    m_batterySlider->root()->setPosition({2 * padding, y});
    y += 12 + itemSpacing;
    
    // Battery percentage (centered on slider)
    m_batteryPercentage->setSize(width - 4 * padding, 40);
    m_batteryPercentage->layout(renderer, width - 4 * padding, 40);
    m_batteryPercentage->root()->setPosition({2 * padding, y});
    y += 40 + itemSpacing;
    
    // Battery time
    m_batteryTime->setSize(width - 2 * padding, 20);
    m_batteryTime->layout(renderer, width - 2 * padding, 20);
    m_batteryTime->root()->setPosition({padding, y});
    y += 20 + sectionSpacing;
    
    // Power Profile Section
    m_profileSectionLabel->setSize(width - 2 * padding, 24);
    m_profileSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_profileSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Power profile button
    m_powerProfileButton->setSize(180, 40);
    m_powerProfileButton->layout(renderer, 180, 40);
    m_powerProfileButton->root()->setPosition({padding, y});
    y += 40 + itemSpacing;
    
    // Power profile status
    m_powerProfileStatus->setSize(width - 2 * padding, 20);
    m_powerProfileStatus->layout(renderer, width - 2 * padding, 20);
    m_powerProfileStatus->root()->setPosition({padding, y});
    y += 20 + sectionSpacing;
    
    // Power Actions Section
    m_actionsSectionLabel->setSize(width - 2 * padding, 24);
    m_actionsSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_actionsSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Action buttons
    float buttonsStart = padding;
    float buttonsTotalWidth = 4 * buttonWidth + 3 * itemSpacing;
    float buttonsX = (width - buttonsTotalWidth) / 2.0f;
    
    // Layout buttons in a row
    m_suspendButton->setSize(buttonWidth, 40);
    m_suspendButton->layout(renderer, buttonWidth, 40);
    m_suspendButton->root()->setPosition({buttonsX, y});
    
    m_hibernateButton->setSize(buttonWidth, 40);
    m_hibernateButton->layout(renderer, buttonWidth, 40);
    m_hibernateButton->root()->setPosition({buttonsX + buttonWidth + itemSpacing, y});
    
    m_rebootButton->setSize(buttonWidth, 40);
    m_rebootButton->layout(renderer, buttonWidth, 40);
    m_rebootButton->root()->setPosition({buttonsX + 2 * (buttonWidth + itemSpacing), y});
    
    m_shutdownButton->setSize(buttonWidth, 40);
    m_shutdownButton->layout(renderer, buttonWidth, 40);
    m_shutdownButton->root()->setPosition({buttonsX + 3 * (buttonWidth + itemSpacing), y});
}

void PowerTab::doUpdate(Renderer& renderer) {
    // Update power status periodically
    static int counter = 0;
    if (counter % 60 == 0) { // Update every ~60 frames (~1 second)
        updatePowerStatus();
    }
    counter++;
}

void PowerTab::updatePowerStatus() {
    // This would normally query UPower or similar
    // For now, we'll use mock data
    
    // Battery status
    static float batteryLevel = 0.85f;
    batteryLevel -= 0.001f;
    if (batteryLevel < 0.1f) batteryLevel = 0.85f;
    
    m_batterySlider->setValue(batteryLevel);
    m_batteryPercentage->setText(std::to_string(static_cast<int>(batteryLevel * 100)) + "%");
    
    if (batteryLevel > 0.7f) {
        m_batteryPercentage->setColor(getColorPalette().status_success);
    } else if (batteryLevel > 0.3f) {
        m_batteryPercentage->setColor(getColorPalette().status_warning);
    } else {
        m_batteryPercentage->setColor(getColorPalette().status_error);
    }
    
    // Battery time
    int hours = static_cast<int>((batteryLevel * 4.0f) + 0.5f);
    int minutes = static_cast<int>(((batteryLevel * 4.0f) - hours) * 60.0f + 0.5f);
    m_batteryTime->setText("~" + std::to_string(hours) + " hours " + std::to_string(minutes) + " minutes remaining");
    
    // Power profile
    m_powerProfileButton->setText("Balanced");
    m_powerProfileStatus->setText("Recommended for typical use");
}

void PowerTab::applyModernTheme() {
    const auto& colors = getColorPalette();
    
    setBackgroundColor(colors.background_primary);
    
    if (m_batterySlider) m_batterySlider->setActiveColor(colors.accent_primary);
    if (m_powerProfileButton) m_powerProfileButton->setBackgroundColor(colors.accent_primary);
    if (m_suspendButton) m_suspendButton->setBackgroundColor(colors.background_secondary);
    if (m_hibernateButton) m_hibernateButton->setBackgroundColor(colors.background_secondary);
    if (m_rebootButton) m_rebootButton->setBackgroundColor(colors.background_secondary);
    if (m_shutdownButton) m_shutdownButton->setBackgroundColor(colors.status_error);
}

} // namespace lshell::control_center
