// L-Shell Disk Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/disk_monitor_widget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/design/typography.h"
#include "shell/design/spacing.h"
#include "shell/design/animations.h"
#include "shell/render/core/renderer.h"
#include "shell/bar/ModernDesignedWidget.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include "render/backend/cairo/cairo_wrapper.h"

// ============================================================================
// DiskMonitorWidget Implementation
// ============================================================================

DiskMonitorWidget::DiskMonitorWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_activeDiskIndex(0),
      m_readSpeed(0),
      m_writeSpeed(0),
      m_icon(nullptr),
      m_usageLabel(nullptr),
      m_speedLabel(nullptr),
      m_tempLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    // Initialize with default disks
    m_disks.push_back({"nvme0n1", "/", 30.0f, 120.0f, 500.0f, 0.0f, 0.0f, 45, true}); // SSD
    m_disks.push_back({"sda1", "/home", 65.0f, 800.0f, 1500.0f, 0.0f, 0.0f, 0, false}); // HDD
    
    applyDesignTheme();
}

DiskMonitorWidget::~DiskMonitorWidget() {
    delete m_icon;
    delete m_usageLabel;
    delete m_speedLabel;
    delete m_tempLabel;
    delete m_innerRoot;
}

void DiskMonitorWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create usage label
    if (m_options.showUsage) {
        m_usageLabel = new Label("");
        m_usageLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_usageLabel->setFontSize(theme.font_size_sm.size);
        m_usageLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_usageLabel->setLetterSpacing(m_options.letterSpacing);
        m_usageLabel->setLineHeight(m_options.lineHeight);
        m_usageLabel->setColor(theme.text_primary);
    }
    
    // Create speed label
    if (m_options.showSpeed) {
        m_speedLabel = new Label("");
        m_speedLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_speedLabel->setFontSize(theme.font_size_tiny.size);
        m_speedLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_speedLabel->setLetterSpacing(m_options.letterSpacing);
        m_speedLabel->setLineHeight(m_options.lineHeight);
        m_speedLabel->setColor(theme.text_secondary);
    }
    
    // Create temperature label
    if (m_options.showTemperature) {
        m_tempLabel = new Label("");
        m_tempLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_tempLabel->setFontSize(theme.font_size_tiny.size);
        m_tempLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_tempLabel->setLetterSpacing(m_options.letterSpacing);
        m_tempLabel->setLineHeight(m_options.lineHeight);
        m_tempLabel->setColor(theme.color_warning);
    }
    
    // Update display
    updateDisplay();
    
    // Set tooltip
    setTooltip(m_lastTooltip);
    
    // Create container node
    m_innerRoot = new Node;
    m_innerRoot->setData(this);
    
    // Add children to container
    if (m_icon) m_innerRoot->addChild(m_icon->root());
    if (m_usageLabel) m_innerRoot->addChild(m_usageLabel->root());
    if (m_speedLabel) m_innerRoot->addChild(m_speedLabel->root());
    if (m_tempLabel) m_innerRoot->addChild(m_tempLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void DiskMonitorWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update disk status
    updateDiskStatus();
    
    // Get active disk
    const auto& disk = m_disks[m_activeDiskIndex];
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(getUsageColor(disk.usagePercentage));
    }
    
    // Update usage label
    if (m_usageLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << disk.usagePercentage << "% ";
        if (m_options.showMultipleDisks && m_disks.size() > 1) {
            oss << "(" << disk.mountPoint << ")";
        }
        m_usageLabel->setText(oss.str());
        m_usageLabel->setColor(getUsageColor(disk.usagePercentage));
    }
    
    // Update speed label
    if (m_speedLabel) {
        std::ostringstream oss;
        bool hasSpeed = m_readSpeed > 0.1f || m_writeSpeed > 0.1f;
        if (hasSpeed) {
            if (m_readSpeed > 0.1f) {
                oss << "↓" << std::fixed << std::setprecision(1) << m_readSpeed << "MB/s";
            }
            if (m_writeSpeed > 0.1f) {
                if (m_readSpeed > 0.1f) oss << " ";
                oss << "↑" << std::fixed << std::setprecision(1) << m_writeSpeed << "MB/s";
            }
        } else {
            oss << "Idle";
        }
        m_speedLabel->setText(oss.str());
        m_speedLabel->setColor(theme.text_secondary);
    }
    
    // Update temperature label
    if (m_tempLabel) {
        std::ostringstream oss;
        if (disk.temperature > 0) {
            oss << disk.temperature << "°C";
        } else {
            oss << "N/A";
        }
        m_tempLabel->setText(oss.str());
        m_tempLabel->setColor(disk.temperature > 50 ? theme.color_warning : theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void DiskMonitorWidget::updateDiskStatus() {
    // In a real implementation, this would read from /proc/diskstats or iostat
    // For now, we simulate disk metrics
    
    static int counter = 0;
    counter = (counter + 1) % 100;
    
    // Update all disks
    for (auto& disk : m_disks) {
        // Simulate usage changes
        float delta = 5.0f * std::sin(counter * 0.05f + &disk - &m_disks[0]);
        disk.usagePercentage = std::clamp(disk.usagePercentage + delta, 5.0f, 98.0f);
        disk.usedGB = disk.totalGB * (disk.usagePercentage / 100.0f);
        
        // Simulate temperature for SSD
        if (disk.isSSD) {
            disk.temperature = 30 + static_cast<int>(30 * std::sin(counter * 0.03f));
            disk.temperature = std::clamp(disk.temperature, 25, 70);
        }
    }
    
    // Simulate combined read/write speeds
    m_readSpeed = 10.0f + 90.0f * std::sin(counter * 0.1f);
    m_readSpeed = std::max(m_readSpeed, 0.0f);
    m_writeSpeed = 5.0f + 45.0f * std::sin(counter * 0.15f);
    m_writeSpeed = std::max(m_writeSpeed, 0.0f);
}

std::string DiskMonitorWidget::formatDiskText() const {
    if (m_disks.empty()) return "N/A";
    
    const auto& disk = m_disks[m_activeDiskIndex];
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << disk.usagePercentage << "%";
    return oss.str();
}

std::string DiskMonitorWidget::formatTooltip() const {
    std::ostringstream oss;
    
    for (size_t i = 0; i < m_disks.size(); ++i) {
        const auto& disk = m_disks[i];
        if (i > 0) oss << "\n\n";
        oss << disk.name << " (" << disk.mountPoint << "):\n";
        oss << "  Usage: " << std::fixed << std::setprecision(1) << disk.usedGB << "GB / " << disk.totalGB << "GB (" << std::fixed << std::setprecision(0) << disk.usagePercentage << "%)\n";
        if (disk.temperature > 0) {
            oss << "  Temperature: " << disk.temperature << "°C\n";
        }
        oss << "  Type: " << (disk.isSSD ? "SSD" : "HDD");
    }
    
    if (m_readSpeed > 0.1f || m_writeSpeed > 0.1f) {
        oss << "\n\nI/O:\n";
        oss << "  Read: " << std::fixed << std::setprecision(1) << m_readSpeed << "MB/s\n";
        oss << "  Write: " << std::fixed << std::setprecision(1) << m_writeSpeed << "MB/s";
    }
    
    return oss.str();
}

lshell::design::Color DiskMonitorWidget::getUsageColor(float percentage) const {
    const auto& theme = getDesignTheme();
    
    if (percentage >= 90) return theme.color_danger;
    if (percentage >= 75) return theme.color_warning;
    if (percentage >= 50) return theme.color_info;
    return theme.color_success;
}

std::string DiskMonitorWidget::formatBytes(float bytes) const {
    if (bytes >= 1024 * 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / (1024 * 1024 * 1024)) << "GB";
        return oss.str();
    } else if (bytes >= 1024 * 1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / (1024 * 1024)) << "MB";
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bytes / 1024) << "KB";
        return oss.str();
    }
}

void DiskMonitorWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
    const auto& theme = getDesignTheme();
    
    float paddingX = static_cast<float>(theme.widget_padding_x);
    float paddingY = static_cast<float>(theme.widget_padding_y);
    float spacing = static_cast<float>(theme.widget_spacing);
    float availableWidth = containerWidth - 2 * paddingX;
    
    float x = paddingX;
    
    // Layout icon
    if (m_icon) {
        float iconWidth = theme.font_size_sm.size * 1.5f;
        m_icon->setSize(iconWidth, containerHeight - 2 * paddingY);
        m_icon->layout(renderer, iconWidth, containerHeight - 2 * paddingY);
        m_icon->root()->setPosition({x, paddingY});
        x += iconWidth + spacing;
        availableWidth -= iconWidth + spacing;
    }
    
    // Layout usage
    if (m_usageLabel) {
        float usageWidth = availableWidth * 0.4f;
        m_usageLabel->setSize(usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->layout(renderer, usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->root()->setPosition({x, paddingY});
        x += usageWidth + spacing;
        availableWidth -= usageWidth + spacing;
    }
    
    // Layout speed
    if (m_speedLabel) {
        float speedWidth = availableWidth * 0.4f;
        m_speedLabel->setSize(speedWidth, containerHeight - 2 * paddingY);
        m_speedLabel->layout(renderer, speedWidth, containerHeight - 2 * paddingY);
        m_speedLabel->root()->setPosition({x, paddingY});
        x += speedWidth + spacing;
        availableWidth -= speedWidth + spacing;
    }
    
    // Layout temperature
    if (m_tempLabel) {
        m_tempLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_tempLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_tempLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void DiskMonitorWidget::doUpdate(Renderer& renderer) {
    // Update disk status
    updateDiskStatus();
    
    // Update display
    updateDisplay();
}

void DiskMonitorWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_usageLabel) m_usageLabel->queueRedraw();
    if (m_speedLabel) m_speedLabel->queueRedraw();
    if (m_tempLabel) m_tempLabel->queueRedraw();
}

void DiskMonitorWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// DiskMonitorWidget::Options Builder
// ============================================================================

DiskMonitorWidget::Options DiskMonitorWidget::OptionsBuilder::build() const {
    return options;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::showUsage(bool show) {
    options.showUsage = show;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::showSpeed(bool show) {
    options.showSpeed = show;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::showTemperature(bool show) {
    options.showTemperature = show;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::showMultipleDisks(bool show) {
    options.showMultipleDisks = show;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

DiskMonitorWidget::OptionsBuilder& DiskMonitorWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
