// System Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#pragma once

#include "shell/control_center/ModernControlCenter.h"
#include "shell/design/theme.h"
#include "ui/controls/label.h"
#include "ui/controls/slider.h"
#include "ui/controls/progress_bar.h"

#include <vector>
#include <string>

namespace lshell::control_center {

// ============================================================================
// SystemTab - Displays system information and controls
// ============================================================================

class SystemTab : public ControlCenterTab {
public:
    SystemTab();
    ~SystemTab() override = default;

    void create() override;
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;

private:
    // UI Elements
    ui::Label* m_titleLabel = nullptr;
    
    // System Info Section
    ui::Label* m_hostnameLabel = nullptr;
    ui::Label* m_hostnameValue = nullptr;
    ui::Label* m_kernelLabel = nullptr;
    ui::Label* m_kernelValue = nullptr;
    ui::Label* m_uptimeLabel = nullptr;
    ui::Label* m_uptimeValue = nullptr;
    
    // CPU Section
    ui::Label* m_cpuLabel = nullptr;
    ui::Label* m_cpuUsageValue = nullptr;
    ui::ProgressBar* m_cpuProgress = nullptr;
    
    // Memory Section
    ui::Label* m_memoryLabel = nullptr;
    ui::Label* m_memoryUsageValue = nullptr;
    ui::ProgressBar* m_memoryProgress = nullptr;
    
    // Disk Section
    ui::Label* m_diskLabel = nullptr;
    ui::Label* m_diskUsageValue = nullptr;
    ui::ProgressBar* m_diskProgress = nullptr;
    
    // Get system information
    std::string getHostname();
    std::string getKernelVersion();
    std::string getUptime();
    float getCpuUsage();
    float getMemoryUsage();
    float getDiskUsage();
    
    void updateSystemInfo();
    void applyModernTheme();
};

} // namespace lshell::control_center
