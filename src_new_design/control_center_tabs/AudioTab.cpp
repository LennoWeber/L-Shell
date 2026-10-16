// Audio Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/tabs/AudioTab.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

namespace lshell::control_center {

// ============================================================================
// AudioTab Implementation
// ============================================================================

AudioTab::AudioTab() 
    : ControlCenterTab(Options{
        .name = "audio",
        .icon = "audio-speakers-symbolic",
        .title = "Audio"
    }) {
}

void AudioTab::create() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& typography = getTypography();
    const auto& spacing = getSpacing();
    
    // Create title
    m_titleLabel = new ui::Label("Audio Settings");
    m_titleLabel->setFontFamily(typography.heading.font_family);
    m_titleLabel->setFontSize(typography.heading.font_size);
    m_titleLabel->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_titleLabel->setColor(colors.text_primary);
    addChild(m_titleLabel->root());
    
    // Output Devices Section
    m_outputSectionLabel = new ui::Label("Output Devices");
    m_outputSectionLabel->setFontFamily(typography.subheading.font_family);
    m_outputSectionLabel->setFontSize(typography.subheading.font_size);
    m_outputSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_outputSectionLabel->setColor(colors.text_primary);
    addChild(m_outputSectionLabel->root());
    
    m_outputDeviceLabel = new ui::Label("Device: Built-in Audio");
    m_outputDeviceLabel->setFontFamily(typography.body.font_family);
    m_outputDeviceLabel->setFontSize(typography.body.font_size);
    m_outputDeviceLabel->setColor(colors.text_primary);
    addChild(m_outputDeviceLabel->root());
    
    m_outputVolumeSlider = new ui::Slider;
    m_outputVolumeSlider->setValue(0.7f);
    m_outputVolumeSlider->setHeight(8);
    m_outputVolumeSlider->onValueChanged = [this](float value) {
        m_outputVolumeValue->setText(std::to_string(static_cast<int>(value * 100)) + "%");
    };
    addChild(m_outputVolumeSlider->root());
    
    m_outputVolumeValue = new ui::Label("70%");
    m_outputVolumeValue->setFontFamily(typography.body.font_family);
    m_outputVolumeValue->setFontSize(typography.body.font_size);
    m_outputVolumeValue->setColor(colors.text_primary);
    m_outputVolumeValue->setWidth(50);
    m_outputVolumeValue->setTextAlign(::TextAlign::Right);
    addChild(m_outputVolumeValue->root());
    
    m_outputMuteSwitch = new ui::Switch;
    m_outputMuteSwitch->setActive(false);
    m_outputMuteSwitch->onToggled = [this](bool active) {
        // Toggle mute
    };
    addChild(m_outputMuteSwitch->root());
    
    auto* outputMuteLabel = new ui::Label("Mute");
    outputMuteLabel->setFontFamily(typography.body.font_family);
    outputMuteLabel->setFontSize(typography.body.font_size);
    outputMuteLabel->setColor(colors.text_secondary);
    addChild(outputMuteLabel->root());
    
    // Input Devices Section
    m_inputSectionLabel = new ui::Label("Input Devices");
    m_inputSectionLabel->setFontFamily(typography.subheading.font_family);
    m_inputSectionLabel->setFontSize(typography.subheading.font_size);
    m_inputSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_inputSectionLabel->setColor(colors.text_primary);
    addChild(m_inputSectionLabel->root());
    
    m_inputDeviceLabel = new ui::Label("Device: Microphone");
    m_inputDeviceLabel->setFontFamily(typography.body.font_family);
    m_inputDeviceLabel->setFontSize(typography.body.font_size);
    m_inputDeviceLabel->setColor(colors.text_primary);
    addChild(m_inputDeviceLabel->root());
    
    m_inputVolumeSlider = new ui::Slider;
    m_inputVolumeSlider->setValue(0.5f);
    m_inputVolumeSlider->setHeight(8);
    m_inputVolumeSlider->onValueChanged = [this](float value) {
        m_inputVolumeValue->setText(std::to_string(static_cast<int>(value * 100)) + "%");
    };
    addChild(m_inputVolumeSlider->root());
    
    m_inputVolumeValue = new ui::Label("50%");
    m_inputVolumeValue->setFontFamily(typography.body.font_family);
    m_inputVolumeValue->setFontSize(typography.body.font_size);
    m_inputVolumeValue->setColor(colors.text_primary);
    m_inputVolumeValue->setWidth(50);
    m_inputVolumeValue->setTextAlign(::TextAlign::Right);
    addChild(m_inputVolumeValue->root());
    
    m_inputMuteSwitch = new ui::Switch;
    m_inputMuteSwitch->setActive(false);
    m_inputMuteSwitch->onToggled = [this](bool active) {
        // Toggle mute
    };
    addChild(m_inputMuteSwitch->root());
    
    auto* inputMuteLabel = new ui::Label("Mute");
    inputMuteLabel->setFontFamily(typography.body.font_family);
    inputMuteLabel->setFontSize(typography.body.font_size);
    inputMuteLabel->setColor(colors.text_secondary);
    addChild(inputMuteLabel->root());
    
    // Media Controls Section
    m_mediaSectionLabel = new ui::Label("Media Controls");
    m_mediaSectionLabel->setFontFamily(typography.subheading.font_family);
    m_mediaSectionLabel->setFontSize(typography.subheading.font_size);
    m_mediaSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    m_mediaSectionLabel->setColor(colors.text_primary);
    addChild(m_mediaSectionLabel->root());
    
    m_mediaTitle = new ui::Label("No media playing");
    m_mediaTitle->setFontFamily(typography.body.font_family);
    m_mediaTitle->setFontSize(typography.body.font_size);
    m_mediaTitle->setColor(colors.text_secondary);
    addChild(m_mediaTitle->root());
    
    m_mediaArtist = new ui::Label("");
    m_mediaArtist->setFontFamily(typography.body_small.font_family);
    m_mediaArtist->setFontSize(typography.body_small.font_size);
    m_mediaArtist->setColor(colors.text_secondary);
    addChild(m_mediaArtist->root());
    
    applyModernTheme();
    updateAudioStatus();
}

void AudioTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float padding = static_cast<float>(spacing.lg);
    float sectionSpacing = static_cast<float>(spacing.xl);
    float itemSpacing = static_cast<float>(spacing.md);
    float sliderWidth = 200.0f;
    float labelWidth = 150.0f;
    
    float y = padding;
    
    // Title
    m_titleLabel->setSize(width - 2 * padding, 30);
    m_titleLabel->layout(renderer, width - 2 * padding, 30);
    m_titleLabel->root()->setPosition({padding, y});
    y += 30 + sectionSpacing;
    
    // Output Devices Section
    m_outputSectionLabel->setSize(width - 2 * padding, 24);
    m_outputSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_outputSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Output device label
    m_outputDeviceLabel->setSize(width - 2 * padding, 24);
    m_outputDeviceLabel->layout(renderer, width - 2 * padding, 24);
    m_outputDeviceLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Output volume slider row
    m_outputVolumeSlider->setSize(sliderWidth, 8);
    m_outputVolumeSlider->layout(renderer, sliderWidth, 8);
    m_outputVolumeSlider->root()->setPosition({padding, y + 10});
    
    m_outputVolumeValue->setSize(50, 24);
    m_outputVolumeValue->layout(renderer, 50, 24);
    m_outputVolumeValue->root()->setPosition({padding + sliderWidth + itemSpacing, y});
    
    m_outputMuteSwitch->setSize(40, 20);
    m_outputMuteSwitch->layout(renderer, 40, 20);
    m_outputMuteSwitch->root()->setPosition({padding + sliderWidth + itemSpacing + 50 + itemSpacing, y + 2});
    
    auto* outputMuteLabel = static_cast<ui::Label*>(getChildren()[7]);
    outputMuteLabel->setSize(50, 20);
    outputMuteLabel->layout(renderer, 50, 20);
    outputMuteLabel->root()->setPosition({padding + sliderWidth + itemSpacing + 50 + itemSpacing + 40 + itemSpacing, y});
    
    y += 24 + sectionSpacing;
    
    // Input Devices Section
    m_inputSectionLabel->setSize(width - 2 * padding, 24);
    m_inputSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_inputSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Input device label
    m_inputDeviceLabel->setSize(width - 2 * padding, 24);
    m_inputDeviceLabel->layout(renderer, width - 2 * padding, 24);
    m_inputDeviceLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Input volume slider row
    m_inputVolumeSlider->setSize(sliderWidth, 8);
    m_inputVolumeSlider->layout(renderer, sliderWidth, 8);
    m_inputVolumeSlider->root()->setPosition({padding, y + 10});
    
    m_inputVolumeValue->setSize(50, 24);
    m_inputVolumeValue->layout(renderer, 50, 24);
    m_inputVolumeValue->root()->setPosition({padding + sliderWidth + itemSpacing, y});
    
    m_inputMuteSwitch->setSize(40, 20);
    m_inputMuteSwitch->layout(renderer, 40, 20);
    m_inputMuteSwitch->root()->setPosition({padding + sliderWidth + itemSpacing + 50 + itemSpacing, y + 2});
    
    auto* inputMuteLabel = static_cast<ui::Label*>(getChildren()[12]);
    inputMuteLabel->setSize(50, 20);
    inputMuteLabel->layout(renderer, 50, 20);
    inputMuteLabel->root()->setPosition({padding + sliderWidth + itemSpacing + 50 + itemSpacing + 40 + itemSpacing, y});
    
    y += 24 + sectionSpacing;
    
    // Media Controls Section
    m_mediaSectionLabel->setSize(width - 2 * padding, 24);
    m_mediaSectionLabel->layout(renderer, width - 2 * padding, 24);
    m_mediaSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    m_mediaTitle->setSize(width - 2 * padding, 24);
    m_mediaTitle->layout(renderer, width - 2 * padding, 24);
    m_mediaTitle->root()->setPosition({padding, y});
    y += 24;
    
    m_mediaArtist->setSize(width - 2 * padding, 20);
    m_mediaArtist->layout(renderer, width - 2 * padding, 20);
    m_mediaArtist->root()->setPosition({padding, y});
}

void AudioTab::doUpdate(Renderer& renderer) {
    // Update audio status periodically
    static int counter = 0;
    if (counter % 30 == 0) { // Update every ~30 frames (~0.5 seconds)
        updateAudioStatus();
    }
    counter++;
}

void AudioTab::updateAudioStatus() {
    // This would normally query PipeWire or PulseAudio
    // For now, we'll use mock data
    
    // Output device
    m_outputDeviceLabel->setText("Device: Built-in Audio");
    m_outputVolumeSlider->setValue(0.7f);
    m_outputVolumeValue->setText("70%");
    m_outputMuteSwitch->setActive(false);
    
    // Input device
    m_inputDeviceLabel->setText("Device: Microphone");
    m_inputVolumeSlider->setValue(0.5f);
    m_inputVolumeValue->setText("50%");
    m_inputMuteSwitch->setActive(false);
    
    // Media info
    m_mediaTitle->setText("No media playing");
    m_mediaArtist->setText("");
}

void AudioTab::applyModernTheme() {
    const auto& colors = getColorPalette();
    
    setBackgroundColor(colors.background_primary);
    
    if (m_outputVolumeSlider) m_outputVolumeSlider->setActiveColor(colors.accent_primary);
    if (m_inputVolumeSlider) m_inputVolumeSlider->setActiveColor(colors.accent_primary);
    if (m_outputMuteSwitch) m_outputMuteSwitch->setActiveColor(colors.accent_secondary);
    if (m_inputMuteSwitch) m_inputMuteSwitch->setActiveColor(colors.accent_secondary);
}

} // namespace lshell::control_center
