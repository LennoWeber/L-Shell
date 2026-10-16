// L-Shell RAM Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/ram_monitor_widget.h"
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
// RAMMonitorWidget Implementation
// ============================================================================

RAMMonitorWidget::RAMMonitorWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_usage(0),
      m_usedGB(0),
      m_totalGB(32), // 32GB RAM
      m_swapUsage(0),
      m_swapUsedGB(0),
      m_swapTotalGB(4), // 4GB swap
      m_pressureWarning(false),
      m_ramType("DDR5-6000"),
      m_icon(nullptr),
      m_usageLabel(nullptr),
      m_usageValueLabel(nullptr),
      m_swapLabel(nullptr),
      m_pressureLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
}

RAMMonitorWidget::~RAMMonitorWidget() {
    delete m_icon;
    delete m_usageLabel;
    delete m_usageValueLabel;
    delete m_swapLabel;
    delete m_pressureLabel;
    delete m_innerRoot;
}

void RAMMonitorWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create usage percentage label
    if (m_options.showPercentage) {
        m_usageLabel = new Label("");
        m_usageLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_usageLabel->setFontSize(theme.font_size_sm.size);
        m_usageLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_usageLabel->setLetterSpacing(m_options.letterSpacing);
        m_usageLabel->setLineHeight(m_options.lineHeight);
        m_usageLabel->setColor(theme.text_primary);
    }
    
    // Create usage value label (GB)
    if (m_options.showUsage) {
        m_usageValueLabel = new Label("");
        m_usageValueLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_usageValueLabel->setFontSize(theme.font_size_tiny.size);
        m_usageValueLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_usageValueLabel->setLetterSpacing(m_options.letterSpacing);
        m_usageValueLabel->setLineHeight(m_options.lineHeight);
        m_usageValueLabel->setColor(theme.text_secondary);
    }
    
    // Create swap label
    if (m_options.showSwap) {
        m_swapLabel = new Label("");
        m_swapLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_swapLabel->setFontSize(theme.font_size_tiny.size);
        m_swapLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_swapLabel->setLetterSpacing(m_options.letterSpacing);
        m_swapLabel->setLineHeight(m_options.lineHeight);
        m_swapLabel->setColor(theme.text_secondary);
    }
    
    // Create pressure label
    if (m_options.showPressure) {
        m_pressureLabel = new Label("");
        m_pressureLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_pressureLabel->setFontSize(theme.font_size_tiny.size);
        m_pressureLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_pressureLabel->setLetterSpacing(m_options.letterSpacing);
        m_pressureLabel->setLineHeight(m_options.lineHeight);
        m_pressureLabel->setColor(theme.color_warning);
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
    if (m_usageValueLabel) m_innerRoot->addChild(m_usageValueLabel->root());
    if (m_swapLabel) m_innerRoot->addChild(m_swapLabel->root());
    if (m_pressureLabel) m_innerRoot->addChild(m_pressureLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void RAMMonitorWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update RAM status
    updateRAMStatus();
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(getUsageColor(m_usage));
    }
    
    // Update usage percentage label
    if (m_usageLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << m_usage << "%";
        m_usageLabel->setText(oss.str());
        m_usageLabel->setColor(getUsageColor(m_usage));
    }
    
    // Update usage value label
    if (m_usageValueLabel) {
        std::ostringstream oss;
        oss << formatBytes(m_usedGB * 1024 * 1024 * 1024) << " / " << formatBytes(m_totalGB * 1024 * 1024 * 1024);
        m_usageValueLabel->setText(oss.str());
        m_usageValueLabel->setColor(theme.text_secondary);
    }
    
    // Update swap label
    if (m_swapLabel) {
        std::ostringstream oss;
        if (m_swapUsage > 0) {
            oss << "Swap: " << std::fixed << std::setprecision(0) << m_swapUsage << "%" ;
        } else {
            oss << "Swap: 0% ";
        }
        m_swapLabel->setText(oss.str());
        m_swapLabel->setColor(m_swapUsage > 50 ? theme.color_warning : theme.text_secondary);
    }
    
    // Update pressure label
    if (m_pressureLabel) {
        if (m_pressureWarning) {
            m_pressureLabel->setText("Pressure: HIGH");
            m_pressureLabel->setColor(theme.color_danger);
        } else {
            m_pressureLabel->setText("Pressure: OK");
            m_pressureLabel->setColor(theme.color_success);
        }
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void RAMMonitorWidget::updateRAMStatus() {
    // In a real implementation, this would read from /proc/meminfo
    // For now, we simulate RAM metrics
    
    static int counter = 0;
    counter = (counter + 1) % 100;
    
    // Simulate RAM usage (0-100%)
    m_usage = 30.0f + 50.0f * std::sin(counter * 0.05f);
    m_usage = std::clamp(m_usage, 0.0f, 100.0f);
    m_usedGB = m_totalGB * (m_usage / 100.0f);
    
    // Simulate swap usage (0-100%)
    m_swapUsage = 0.0f + 40.0f * std::sin(counter * 0.08f);
    m_swapUsage = std::clamp(m_swapUsage, 0.0f, 100.0f);
    m_swapUsedGB = m_swapTotalGB * (m_swapUsage / 100.0f);
    
    // Simulate memory pressure
    m_pressureWarning = m_usage > 85 || m_swapUsage > 70;
}

std::string RAMMonitorWidget::formatRAMText() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << m_usage << "%";
    return oss.str();
}

std::string RAMMonitorWidget::formatBytes(float bytes) const {
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

std::string RAMMonitorWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "RAM: " << m_ramType << "\n";
    oss << "Usage: " << std::fixed << std::setprecision(1) << m_usedGB << "GB / " << m_totalGB << "GB (" << std::fixed << std::setprecision(0) << m_usage << "%)\n";
    oss << "Swap: " << std::fixed << std::setprecision(1) << m_swapUsedGB << "GB / " << m_swapTotalGB << "GB (" << std::fixed << std::setprecision(0) << m_swapUsage << "%)\n";
    oss << "Memory Pressure: " << (m_pressureWarning ? "HIGH - Consider closing applications" : "Normal");
    return oss.str();
}

lshell::design::Color RAMMonitorWidget::getUsageColor(float percentage) const {
    const auto& theme = getDesignTheme();
    
    if (percentage >= 85) return theme.color_danger;
    if (percentage >= 70) return theme.color_warning;
    if (percentage >= 50) return theme.color_info;
    return theme.color_success;
}

void RAMMonitorWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
    
    // Layout usage percentage
    if (m_usageLabel) {
        float usageWidth = availableWidth * 0.3f;
        m_usageLabel->setSize(usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->layout(renderer, usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->root()->setPosition({x, paddingY});
        x += usageWidth + spacing;
        availableWidth -= usageWidth + spacing;
    }
    
    // Layout usage value
    if (m_usageValueLabel) {
        float valueWidth = availableWidth * 0.4f;
        m_usageValueLabel->setSize(valueWidth, containerHeight - 2 * paddingY);
        m_usageValueLabel->layout(renderer, valueWidth, containerHeight - 2 * paddingY);
        m_usageValueLabel->root()->setPosition({x, paddingY});
        x += valueWidth + spacing;
        availableWidth -= valueWidth + spacing;
    }
    
    // Layout swap
    if (m_swapLabel) {
        float swapWidth = availableWidth * 0.3f;
        m_swapLabel->setSize(swapWidth, containerHeight - 2 * paddingY);
        m_swapLabel->layout(renderer, swapWidth, containerHeight - 2 * paddingY);
        m_swapLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void RAMMonitorWidget::doUpdate(Renderer& renderer) {
    // Update RAM status
    updateRAMStatus();
    
    // Update display
    updateDisplay();
}

void RAMMonitorWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_usageLabel) m_usageLabel->queueRedraw();
    if (m_usageValueLabel) m_usageValueLabel->queueRedraw();
    if (m_swapLabel) m_swapLabel->queueRedraw();
    if (m_pressureLabel) m_pressureLabel->queueRedraw();
}

void RAMMonitorWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// RAMMonitorWidget::Options Builder
// ============================================================================

RAMMonitorWidget::Options RAMMonitorWidget::OptionsBuilder::build() const {
    return options;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::showPercentage(bool show) {
    options.showPercentage = show;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::showUsage(bool show) {
    options.showUsage = show;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::showSwap(bool show) {
    options.showSwap = show;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::showPressure(bool show) {
    options.showPressure = show;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

RAMMonitorWidget::OptionsBuilder& RAMMonitorWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
