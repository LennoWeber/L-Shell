// Display Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/tabs/DisplayTab.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

namespace lshell::control_center {

// ============================================================================
// DisplayTab Implementation
// ============================================================================

DisplayTab::DisplayTab() 
    : ControlCenterTab(Options{
        .name = "display",
        .icon = "video-display-symbolic",
        .title = "Display"
    }) {
}

void DisplayTab::create() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& typography = getTypography();
    const auto& spacing = getSpacing();
    
    // Create title
    m_titleLabel = new ui::Label("Display Settings");
    m_titleLabel->setFontFamily(typography.heading.font_family);
    m_titleLabel->setFontSize(typography.heading.font_size);
    m_titleLabel->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_titleLabel->setColor(colors.text_primary);
    addChild(m_titleLabel->root());
    
    // Brightness Section
    m_brightnessSectionLabel = new ui::Label("Brightness");
    m_brightnessSectionLabel->setFontFamily(typography.subheading.font_family);
    m_brightnessSectionLabel->setFontSize(typography.subheading.font_size);
    m_brightnessSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_brightnessSectionLabel->setColor(colors.text_primary);
    addChild(m_brightnessSectionLabel->root());
    
    m_brightnessLabel = new ui::Label("Screen Brightness:");
    m_brightnessLabel->setFontFamily(typography.body.font_family);
    m_brightnessLabel->setFontSize(typography.body.font_size);
    m_brightnessLabel->setColor(colors.text_secondary);
    addChild(m_brightnessLabel->root());
    
    m_brightnessSlider = new ui::Slider;
    m_brightnessSlider->setValue(0.7f);
    m_brightnessSlider->setHeight(8);
    m_brightnessSlider->onValueChanged = [this](float value) {
        m_brightnessValue->setText(std::to_string(static_cast<int>(value * 100)) + "%");
    };
    addChild(m_brightnessSlider->root());
    
    m_brightnessValue = new ui::Label("70%");
    m_brightnessValue->setFontFamily(typography.body.font_family);
    m_brightnessValue->setFontSize(typography.body.font_size);
    m_brightnessValue->setColor(colors.text_primary);
    m_brightnessValue->setWidth(50);
    m_brightnessValue->setTextAlign(::TextAlign::Right);
    addChild(m_brightnessValue->root());
    
    // Night Light Section
    m_nightlightSectionLabel = new ui::Label("Night Light");
    m_nightlightSectionLabel->setFontFamily(typography.subheading.font_family);
    m_nightlightSectionLabel->setFontSize(typography.subheading.font_size);
    m_nightlightSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_nightlightSectionLabel->setColor(colors.text_primary);
    addChild(m_nightlightSectionLabel->root());
    
    m_nightlightSwitch = new ui::Switch;
    m_nightlightSwitch->setActive(false);
    m_nightlightSwitch->onToggled = [this](bool active) {
        // Toggle night light
    };
    addChild(m_nightlightSwitch->root());
    
    m_nightlightStatus = new ui::Label("Off");
    m_nightlightStatus->setFontFamily(typography.body.font_family);
    m_nightlightStatus->setFontSize(typography.body.font_size);
    m_nightlightStatus->setColor(colors.text_secondary);
    addChild(m_nightlightStatus->root());
    
    m_nightlightTemperatureLabel = new ui::Label("Temperature:");
    m_nightlightTemperatureLabel->setFontFamily(typography.body.font_family);
    m_nightlightTemperatureLabel->setFontSize(typography.body.font_size);
    m_nightlightTemperatureLabel->setColor(colors.text_secondary);
    addChild(m_nightlightTemperatureLabel->root());
    
    m_nightlightTemperatureSlider = new ui::Slider;
    m_nightlightTemperatureSlider->setValue(0.5f);
    m_nightlightTemperatureSlider->setHeight(8);
    m_nightlightTemperatureSlider->setEnabled(false);
    m_nightlightTemperatureSlider->onValueChanged = [this](float value) {
        m_nightlightTemperatureValue->setText(std::to_string(static_cast<int>(value * 1000 + 1000)) + "K");
    };
    addChild(m_nightlightTemperatureSlider->root());
    
    m_nightlightTemperatureValue = new ui::Label("1500K");
    m_nightlightTemperatureValue->setFontFamily(typography.body.font_family);
    m_nightlightTemperatureValue->setFontSize(typography.body.font_size);
    m_nightlightTemperatureValue->setColor(colors.text_primary);
    m_nightlightTemperatureValue->setWidth(70);
    m_nightlightTemperatureValue->setTextAlign(::TextAlign::Right);
    addChild(m_nightlightTemperatureValue->root());
    
    // Display Arrangement Section
    m_arrangementSectionLabel = new ui::Label("Display Arrangement");
    m_arrangementSectionLabel->setFontFamily(typography.subheading.font_family);
    m_arrangementSectionLabel->setFontSize(typography.subheading.font_size);
    m_arrangementSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_arrangementSectionLabel->setColor(colors.text_primary);
    addChild(m_arrangementSectionLabel->root());
    
    m_arrangementButton = new ui::Button;
    m_arrangementButton->setText("Open Display Settings");
    m_arrangementButton->setIcon("video-display-symbolic");
    m_arrangementButton->setCornerRadius(8);
    m_arrangementButton->setStyle(ui::Button::Style::Secondary);
    m_arrangementButton->onClick = [this]() {
        // Open display settings
    };
    addChild(m_arrangementButton->root());
    
    // Refresh Rate Section
    m_refreshRateSectionLabel = new ui::Label("Refresh Rate");
    m_refreshRateSectionLabel->setFontFamily(typography.subheading.font_family);
    m_refreshRateSectionLabel->setFontSize(typography.subheading.font_size);
    m_refreshRateSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_refreshRateSectionLabel->setColor(colors.text_primary);
    addChild(m_refreshRateSectionLabel->root());
    
    m_refreshRateLabel = new ui::Label("Current: 144Hz");
    m_refreshRateLabel->setFontFamily(typography.body.font_family);
    m_refreshRateLabel->setFontSize(typography.body.font_size);
    m_refreshRateLabel->setColor(colors.text_primary);
    addChild(m_refreshRateLabel->root());
    
    applyModernTheme();
    updateDisplayStatus();
}

void DisplayTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float padding = static_cast<float>(spacing.lg);
    float sectionSpacing = static_cast<float>(spacing.xl);
    float itemSpacing = static_cast<float>(spacing.md);
    float sliderWidth = 200.0f;
    
    float y = padding;
    
    // Title
    m_titleLabel->setSize(width - 2 * padding, 30);
    m_titleLabel->layout(renderer, width - 2 * padding, 30);
    m_titleLabel->root()->setPosition({padding, y});
    y += 30 + sectionSpacing;
    
    // Brightness Section
    m_brightnessSectionLabel->setSize(width - 2 * padding, 24);
    m_brightnessSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_brightnessSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Brightness label
    m_brightnessLabel->setSize(width - 2 * padding, 24);
    m_brightnessLabel->layout(renderer, width - 2 * padding, 24);
    m_brightnessLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Brightness slider row
    m_brightnessSlider->setSize(sliderWidth, 8);
    m_brightnessSlider->layout(renderer, sliderWidth, 8);
    m_brightnessSlider->root()->setPosition({padding, y + 10});
    
    m_brightnessValue->setSize(50, 24);
    m_brightnessValue->layout(renderer, 50, 24);
    m_brightnessValue->root()->setPosition({padding + sliderWidth + itemSpacing, y});
    y += 24 + sectionSpacing;
    
    // Night Light Section
    m_nightlightSectionLabel->setSize(width - 2 * padding, 24);
    m_nightlightSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_nightlightSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Night light toggle row
    auto* nightlightLabel = new ui::Label("Enable Night Light:");
    nightlightLabel->setFontFamily(getTypography().body.font_family);
    nightlightLabel->setFontSize(getTypography().body.font_size);
    nightlightLabel->setColor(getColorPalette().text_secondary);
    nightlightLabel->setWidth(150);
    nightlightLabel->setTextAlign(::TextAlign::Right);
    // This label will be added to children in doLayout
    
    m_nightlightSwitch->setSize(40, 20);
    m_nightlightSwitch->layout(renderer, 40, 20);
    m_nightlightSwitch->root()->setPosition({padding + 150 + itemSpacing, y + 2});
    
    m_nightlightStatus->setSize(80, 24);
    m_nightlightStatus->layout(renderer, 80, 24);
    m_nightlightStatus->root()->setPosition({padding + 150 + itemSpacing + 40 + itemSpacing, y});
    y += 24 + itemSpacing;
    
    // Night light temperature row
    m_nightlightTemperatureLabel->setSize(150, 24);
    m_nightlightTemperatureLabel->layout(renderer, 150, 24);
    m_nightlightTemperatureLabel->root()->setPosition({padding, y});
    
    m_nightlightTemperatureSlider->setSize(sliderWidth, 8);
    m_nightlightTemperatureSlider->layout(renderer, sliderWidth, 8);
    m_nightlightTemperatureSlider->root()->setPosition({padding + 150 + itemSpacing, y + 10});
    
    m_nightlightTemperatureValue->setSize(70, 24);
    m_nightlightTemperatureValue->layout(renderer, 70, 24);
    m_nightlightTemperatureValue->root()->setPosition({padding + 150 + itemSpacing + sliderWidth + itemSpacing, y});
    y += 24 + sectionSpacing;
    
    // Display Arrangement Section
    m_arrangementSectionLabel->setSize(width - 2 * padding, 24);
    m_arrangementSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_arrangementSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    m_arrangementButton->setSize(240, 40);
    m_arrangementButton->layout(renderer, 240, 40);
    m_arrangementButton->root()->setPosition({padding, y});
    y += 40 + sectionSpacing;
    
    // Refresh Rate Section
    m_refreshRateSectionLabel->setSize(width - 2 * padding, 24);
    m_refreshRateSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_refreshRateSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    m_refreshRateLabel->setSize(width - 2 * padding, 24);
    m_refreshRateLabel->layout(renderer, width - 2 * padding, 24);
    m_refreshRateLabel->root()->setPosition({padding, y});
}

void DisplayTab::doUpdate(Renderer& renderer) {
    // Update display status periodically
    static int counter = 0;
    if (counter % 60 == 0) { // Update every ~60 frames (~1 second)
        updateDisplayStatus();
    }
    counter++;
}

void DisplayTab::updateDisplayStatus() {
    // This would normally query the display service
    // For now, we'll use mock data
    
    // Brightness
    m_brightnessSlider->setValue(0.7f);
    m_brightnessValue->setText("70%");
    
    // Night Light
    m_nightlightSwitch->setActive(false);
    m_nightlightStatus->setText("Off");
    m_nightlightStatus->setColor(getColorPalette().text_secondary);
    m_nightlightTemperatureSlider->setValue(0.5f);
    m_nightlightTemperatureValue->setText("1500K");
    
    // Refresh Rate
    m_refreshRateLabel->setText("Current: 144Hz");
}

void DisplayTab::applyModernTheme() {
    const auto& colors = getColorPalette();
    
    setBackgroundColor(colors.background_primary);
    
    if (m_brightnessSlider) m_brightnessSlider->setActiveColor(colors.accent_primary);
    if (m_nightlightTemperatureSlider) m_nightlightTemperatureSlider->setActiveColor(colors.accent_secondary);
    if (m_nightlightSwitch) m_nightlightSwitch->setActiveColor(colors.accent_primary);
    if (m_arrangementButton) m_arrangementButton->setBackgroundColor(colors.accent_primary);
}

} // namespace lshell::control_center
