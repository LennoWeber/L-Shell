// Audio Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#pragma once

#include "shell/control_center/ModernControlCenter.h"
#include "shell/design/theme.h"
#include "ui/controls/label.h"
#include "ui/controls/slider.h"
#include "ui/controls/switch.h"

#include <vector>
#include <string>

namespace lshell::control_center {

// ============================================================================
// AudioTab - Displays audio devices and volume controls
// ============================================================================

class AudioTab : public ControlCenterTab {
public:
    AudioTab();
    ~AudioTab() override = default;

    void create() override;
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;

private:
    // UI Elements
    ui::Label* m_titleLabel = nullptr;
    
    // Output Devices Section
    ui::Label* m_outputSectionLabel = nullptr;
    ui::Label* m_outputDeviceLabel = nullptr;
    ui::Slider* m_outputVolumeSlider = nullptr;
    ui::Label* m_outputVolumeValue = nullptr;
    ui::Switch* m_outputMuteSwitch = nullptr;
    
    // Input Devices Section
    ui::Label* m_inputSectionLabel = nullptr;
    ui::Label* m_inputDeviceLabel = nullptr;
    ui::Slider* m_inputVolumeSlider = nullptr;
    ui::Label* m_inputVolumeValue = nullptr;
    ui::Switch* m_inputMuteSwitch = nullptr;
    
    // Media Controls Section
    ui::Label* m_mediaSectionLabel = nullptr;
    ui::Label* m_mediaTitle = nullptr;
    ui::Label* m_mediaArtist = nullptr;
    
    void updateAudioStatus();
    void applyModernTheme();
};

} // namespace lshell::control_center
