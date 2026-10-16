// System Tab - Modern Control Center
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/tabs/SystemTab.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <sys/utsname.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace lshell::control_center {

// ============================================================================
// SystemTab Implementation
// ============================================================================

SystemTab::SystemTab() 
    : ControlCenterTab(Options{
        .name = "system",
        .icon = "computer-symbolic",
        .title = "System"
    }) {
}

void SystemTab::create() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& typography = getTypography();
    const auto& spacing = getSpacing();
    
    // Create title
    m_titleLabel = new ui::Label("System Information");
    m_titleLabel->setFontFamily(typography.heading.font_family);
    m_titleLabel->setFontSize(typography.heading.font_size);
    m_titleLabel->setFontWeight(static_cast<::FontWeight>(typography.heading.font_weight));
    m_titleLabel->setColor(colors.text_primary);
    addChild(m_titleLabel->root());
    
    // System Info Section
    float sectionSpacing = static_cast<float>(spacing.lg);
    float itemSpacing = static_cast<float>(spacing.md);
    float labelWidth = 120.0f;
    
    // Hostname
    m_hostnameLabel = new ui::Label("Hostname:");
    m_hostnameLabel->setFontFamily(typography.body.font_family);
    m_hostnameLabel->setFontSize(typography.body.font_size);
    m_hostnameLabel->setColor(colors.text_secondary);
    m_hostnameLabel->setWidth(labelWidth);
    m_hostnameLabel->setTextAlign(::TextAlign::Right);
    addChild(m_hostnameLabel->root());
    
    m_hostnameValue = new ui::Label(getHostname());
    m_hostnameValue->setFontFamily(typography.body.font_family);
    m_hostnameValue->setFontSize(typography.body.font_size);
    m_hostnameValue->setColor(colors.text_primary);
    addChild(m_hostnameValue->root());
    
    // Kernel
    m_kernelLabel = new ui::Label("Kernel:");
    m_kernelLabel->setFontFamily(typography.body.font_family);
    m_kernelLabel->setFontSize(typography.body.font_size);
    m_kernelLabel->setColor(colors.text_secondary);
    m_kernelLabel->setWidth(labelWidth);
    m_kernelLabel->setTextAlign(::TextAlign::Right);
    addChild(m_kernelLabel->root());
    
    m_kernelValue = new ui::Label(getKernelVersion());
    m_kernelValue->setFontFamily(typography.body.font_family);
    m_kernelValue->setFontSize(typography.body.font_size);
    m_kernelValue->setColor(colors.text_primary);
    addChild(m_kernelValue->root());
    
    // Uptime
    m_uptimeLabel = new ui::Label("Uptime:");
    m_uptimeLabel->setFontFamily(typography.body.font_family);
    m_uptimeLabel->setFontSize(typography.body.font_size);
    m_uptimeLabel->setColor(colors.text_secondary);
    m_uptimeLabel->setWidth(labelWidth);
    m_uptimeLabel->setTextAlign(::TextAlign::Right);
    addChild(m_uptimeLabel->root());
    
    m_uptimeValue = new ui::Label(getUptime());
    m_uptimeValue->setFontFamily(typography.body.font_family);
    m_uptimeValue->setFontSize(typography.body.font_size);
    m_uptimeValue->setColor(colors.text_primary);
    addChild(m_uptimeValue->root());
    
    // CPU Section
    auto* cpuSectionLabel = new ui::Label("CPU Usage");
    cpuSectionLabel->setFontFamily(typography.subheading.font_family);
    cpuSectionLabel->setFontSize(typography.subheading.font_size);
    cpuSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    cpuSectionLabel->setColor(colors.text_primary);
    addChild(cpuSectionLabel->root());
    
    m_cpuLabel = new ui::Label("Usage:");
    m_cpuLabel->setFontFamily(typography.body.font_family);
    m_cpuLabel->setFontSize(typography.body.font_size);
    m_cpuLabel->setColor(colors.text_secondary);
    m_cpuLabel->setWidth(labelWidth);
    m_cpuLabel->setTextAlign(::TextAlign::Right);
    addChild(m_cpuLabel->root());
    
    m_cpuUsageValue = new ui::Label("0%");
    m_cpuUsageValue->setFontFamily(typography.body.font_family);
    m_cpuUsageValue->setFontSize(typography.body.font_size);
    m_cpuUsageValue->setColor(colors.text_primary);
    addChild(m_cpuUsageValue->root());
    
    m_cpuProgress = new ui::ProgressBar;
    m_cpuProgress->setHeight(8);
    m_cpuProgress->setCornerRadius(4);
    addChild(m_cpuProgress->root());
    
    // Memory Section
    auto* memorySectionLabel = new ui::Label("Memory Usage");
    memorySectionLabel->setFontFamily(typography.subheading.font_family);
    memorySectionLabel->setFontSize(typography.subheading.font_size);
    memorySectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    memorySectionLabel->setColor(colors.text_primary);
    addChild(memorySectionLabel->root());
    
    m_memoryLabel = new ui::Label("Usage:");
    m_memoryLabel->setFontFamily(typography.body.font_family);
    m_memoryLabel->setFontSize(typography.body.font_size);
    m_memoryLabel->setColor(colors.text_secondary);
    m_memoryLabel->setWidth(labelWidth);
    m_memoryLabel->setTextAlign(::TextAlign::Right);
    addChild(m_memoryLabel->root());
    
    m_memoryUsageValue = new ui::Label("0%");
    m_memoryUsageValue->setFontFamily(typography.body.font_family);
    m_memoryUsageValue->setFontSize(typography.body.font_size);
    m_memoryUsageValue->setColor(colors.text_primary);
    addChild(m_memoryUsageValue->root());
    
    m_memoryProgress = new ui::ProgressBar;
    m_memoryProgress->setHeight(8);
    m_memoryProgress->setCornerRadius(4);
    addChild(m_memoryProgress->root());
    
    // Disk Section
    auto* diskSectionLabel = new ui::Label("Disk Usage");
    diskSectionLabel->setFontFamily(typography.subheading.font_family);
    diskSectionLabel->setFontSize(typography.subheading.font_size);
    diskSectionLabel->setFontWeight(static_cast<::FontWeight>(typography.subheading.font_weight));
    diskSectionLabel->setColor(colors.text_primary);
    addChild(diskSectionLabel->root());
    
    m_diskLabel = new ui::Label("Usage:");
    m_diskLabel->setFontFamily(typography.body.font_family);
    m_diskLabel->setFontSize(typography.body.font_size);
    m_diskLabel->setColor(colors.text_secondary);
    m_diskLabel->setWidth(labelWidth);
    m_diskLabel->setTextAlign(::TextAlign::Right);
    addChild(m_diskLabel->root());
    
    m_diskUsageValue = new ui::Label("0%");
    m_diskUsageValue->setFontFamily(typography.body.font_family);
    m_diskUsageValue->setFontSize(typography.body.font_size);
    m_diskUsageValue->setColor(colors.text_primary);
    addChild(m_diskUsageValue->root());
    
    m_diskProgress = new ui::ProgressBar;
    m_diskProgress->setHeight(8);
    m_diskProgress->setCornerRadius(4);
    addChild(m_diskProgress->root());
    
    applyModernTheme();
    updateSystemInfo();
}

void SystemTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float padding = static_cast<float>(spacing.lg);
    float sectionSpacing = static_cast<float>(spacing.xl);
    float itemSpacing = static_cast<float>(spacing.md);
    float labelWidth = 120.0f;
    float progressWidth = 200.0f;
    
    float y = padding;
    
    // Title
    m_titleLabel->setSize(width - 2 * padding, 30);
    m_titleLabel->layout(renderer, width - 2 * padding, 30);
    m_titleLabel->root()->setPosition({padding, y});
    y += 30 + sectionSpacing;
    
    // System Info Section
    auto layoutRow = [&](ui::Label* label, ui::Label* value, float& y_pos) {
        label->setSize(labelWidth, 24);
        label->layout(renderer, labelWidth, 24);
        label->root()->setPosition({padding, y_pos});
        
        value->setSize(width - padding - labelWidth - itemSpacing, 24);
        value->layout(renderer, width - padding - labelWidth - itemSpacing, 24);
        value->root()->setPosition({padding + labelWidth + itemSpacing, y_pos});
        
        y_pos += 24 + itemSpacing;
    };
    
    // Hostname
    layoutRow(m_hostnameLabel, m_hostnameValue, y);
    // Kernel
    layoutRow(m_kernelLabel, m_kernelValue, y);
    // Uptime
    layoutRow(m_uptimeLabel, m_uptimeValue, y);
    
    y += sectionSpacing;
    
    // CPU Section
    auto* cpuSectionLabel = static_cast<ui::Label*>(getChildren()[4]);
    cpuSectionLabel->setSize(width - 2 * padding, 24);
    cpuSectionLabel->layout(renderer, width - 2 * padding, 24);
    cpuSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // CPU Usage
    layoutRow(m_cpuLabel, m_cpuUsageValue, y);
    y += 8;
    
    m_cpuProgress->setSize(progressWidth, 8);
    m_cpuProgress->layout(renderer, progressWidth, 8);
    m_cpuProgress->root()->setPosition({padding + labelWidth + itemSpacing, y});
    y += 8 + sectionSpacing;
    
    // Memory Section
    auto* memorySectionLabel = static_cast<ui::Label*>(getChildren()[9]);
    memorySectionLabel->setSize(width - 2 * padding, 24);
    memorySectionLabel->layout(renderer, width - 2 * padding, 24);
    memorySectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Memory Usage
    layoutRow(m_memoryLabel, m_memoryUsageValue, y);
    y += 8;
    
    m_memoryProgress->setSize(progressWidth, 8);
    m_memoryProgress->layout(renderer, progressWidth, 8);
    m_memoryProgress->root()->setPosition({padding + labelWidth + itemSpacing, y});
    y += 8 + sectionSpacing;
    
    // Disk Section
    auto* diskSectionLabel = static_cast<ui::Label*>(getChildren()[14]);
    diskSectionLabel->setSize(width - 2 * padding, 24);
    diskSectionLabel->layout(renderer, width - 2 * padding, 24);
    diskSectionLabel->root()->setPosition({padding, y});
    y += 24 + itemSpacing;
    
    // Disk Usage
    layoutRow(m_diskLabel, m_diskUsageValue, y);
    y += 8;
    
    m_diskProgress->setSize(progressWidth, 8);
    m_diskProgress->layout(renderer, progressWidth, 8);
    m_diskProgress->root()->setPosition({padding + labelWidth + itemSpacing, y});
}

void SystemTab::doUpdate(Renderer& renderer) {
    // Update system information periodically
    static int counter = 0;
    if (counter % 30 == 0) { // Update every ~30 frames (~0.5 seconds)
        updateSystemInfo();
    }
    counter++;
}

void SystemTab::updateSystemInfo() {
    m_hostnameValue->setText(getHostname());
    m_kernelValue->setText(getKernelVersion());
    m_uptimeValue->setText(getUptime());
    
    float cpuUsage = getCpuUsage();
    m_cpuUsageValue->setText(std::to_string(static_cast<int>(cpuUsage)) + "%");
    m_cpuProgress->setValue(cpuUsage / 100.0f);
    
    float memoryUsage = getMemoryUsage();
    m_memoryUsageValue->setText(std::to_string(static_cast<int>(memoryUsage)) + "%");
    m_memoryProgress->setValue(memoryUsage / 100.0f);
    
    float diskUsage = getDiskUsage();
    m_diskUsageValue->setText(std::to_string(static_cast<int>(diskUsage)) + "%");
    m_diskProgress->setValue(diskUsage / 100.0f);
}

void SystemTab::applyModernTheme() {
    const auto& colors = getColorPalette();
    
    setBackgroundColor(colors.background_primary);
    
    if (m_cpuProgress) m_cpuProgress->setForegroundColor(colors.accent_primary);
    if (m_memoryProgress) m_memoryProgress->setForegroundColor(colors.accent_secondary);
    if (m_diskProgress) m_diskProgress->setForegroundColor(colors.accent_tertiary);
}

std::string SystemTab::getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown";
}

std::string SystemTab::getKernelVersion() {
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        return std::string(unameData.release);
    }
    return "Unknown";
}

std::string SystemTab::getUptime() {
    // Read uptime from /proc/uptime
    std::ifstream file("/proc/uptime");
    if (file.is_open()) {
        double uptime;
        file >> uptime;
        file.close();
        
        int hours = static_cast<int>(uptime / 3600);
        int minutes = static_cast<int>((uptime - hours * 3600) / 60);
        int seconds = static_cast<int>(uptime - hours * 3600 - minutes * 60);
        
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%d:%02d:%02d", hours, minutes, seconds);
        return std::string(buffer);
    }
    return "Unknown";
}

float SystemTab::getCpuUsage() {
    // Read CPU usage from /proc/stat
    std::ifstream file("/proc/stat");
    if (file.is_open()) {
        std::string line;
        std::getline(file, line);
        file.close();
        
        // Parse CPU line
        std::istringstream iss(line);
        std::string cpu;
        long user, nice, system, idle, iowait, irq, softirq;
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
        
        long total = user + nice + system + idle + iowait + irq + softirq;
        long used = total - idle - iowait;
        
        if (total > 0) {
            return static_cast<float>(used) / static_cast<float>(total) * 100.0f;
        }
    }
    return 0.0f;
}

float SystemTab::getMemoryUsage() {
    // Read memory info from /proc/meminfo
    std::ifstream file("/proc/meminfo");
    if (file.is_open()) {
        long total = 0, free = 0, available = 0;
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.find("MemTotal:") == 0) {
                sscanf(line.c_str(), "MemTotal: %ld kB", &total);
            } else if (line.find("MemAvailable:") == 0) {
                sscanf(line.c_str(), "MemAvailable: %ld kB", &available);
            }
        }
        file.close();
        
        if (total > 0) {
            long used = total - available;
            return static_cast<float>(used) / static_cast<float>(total) * 100.0f;
        }
    }
    return 0.0f;
}

float SystemTab::getDiskUsage() {
    // Read disk usage from / (root partition)
    struct statvfs fs;
    if (statvfs("/", &fs) == 0) {
        unsigned long long total = fs.f_blocks * fs.f_frsize;
        unsigned long long free = fs.f_bfree * fs.f_frsize;
        unsigned long long used = total - free;
        
        if (total > 0) {
            return static_cast<float>(used) / static_cast<float>(total) * 100.0f;
        }
    }
    return 0.0f;
}

} // namespace lshell::control_center
