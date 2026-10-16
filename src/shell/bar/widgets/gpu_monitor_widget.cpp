// L-Shell GPU Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/gpu_monitor_widget.h"
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
// GPUMonitorWidget Implementation
// ============================================================================

GPUMonitorWidget::GPUMonitorWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_usage(0),
      m_vramUsed(0),
      m_vramTotal(8),
      m_temperature(0),
      m_fanSpeed(0),
      m_gpuName("NVIDIA RTX 4090"),
      m_icon(nullptr),
      m_usageLabel(nullptr),
      m_vramLabel(nullptr),
      m_tempLabel(nullptr),
      m_fanLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
}

GPUMonitorWidget::~GPUMonitorWidget() {
    delete m_icon;
    delete m_usageLabel;
    delete m_vramLabel;
    delete m_tempLabel;
    delete m_fanLabel;
    delete m_innerRoot;
}

void GPUMonitorWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create usage label
    if (m_options.showPercentage) {
        m_usageLabel = new Label("");
        m_usageLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_usageLabel->setFontSize(theme.font_size_sm.size);
        m_usageLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_usageLabel->setLetterSpacing(m_options.letterSpacing);
        m_usageLabel->setLineHeight(m_options.lineHeight);
        m_usageLabel->setColor(theme.text_primary);
    }
    
    // Create VRAM label
    if (m_options.showVRAM) {
        m_vramLabel = new Label("");
        m_vramLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_vramLabel->setFontSize(theme.font_size_tiny.size);
        m_vramLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_vramLabel->setLetterSpacing(m_options.letterSpacing);
        m_vramLabel->setLineHeight(m_options.lineHeight);
        m_vramLabel->setColor(theme.text_secondary);
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
    
    // Create fan speed label
    if (m_options.showFanSpeed) {
        m_fanLabel = new Label("");
        m_fanLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_fanLabel->setFontSize(theme.font_size_tiny.size);
        m_fanLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_fanLabel->setLetterSpacing(m_options.letterSpacing);
        m_fanLabel->setLineHeight(m_options.lineHeight);
        m_fanLabel->setColor(theme.text_secondary);
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
    if (m_vramLabel) m_innerRoot->addChild(m_vramLabel->root());
    if (m_tempLabel) m_innerRoot->addChild(m_tempLabel->root());
    if (m_fanLabel) m_innerRoot->addChild(m_fanLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void GPUMonitorWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update GPU status
    updateGPUStatus();
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(getUsageColor(m_usage));
    }
    
    // Update usage label
    if (m_usageLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << m_usage << "%";
        m_usageLabel->setText(oss.str());
        m_usageLabel->setColor(getUsageColor(m_usage));
    }
    
    // Update VRAM label
    if (m_vramLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << m_vramUsed << "GB/" << m_vramTotal << "GB";
        m_vramLabel->setText(oss.str());
        m_vramLabel->setColor(theme.text_secondary);
    }
    
    // Update temperature label
    if (m_tempLabel) {
        std::ostringstream oss;
        oss << static_cast<int>(m_temperature) << "°C";
        m_tempLabel->setText(oss.str());
        m_tempLabel->setColor(m_temperature > 70 ? theme.color_danger : theme.color_warning);
    }
    
    // Update fan speed label
    if (m_fanLabel) {
        std::ostringstream oss;
        oss << m_fanSpeed << "RPM";
        m_fanLabel->setText(oss.str());
        m_fanLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void GPUMonitorWidget::updateGPUStatus() {
    // In a real implementation, this would read from NVIDIA XNVML or AMD ADL
    // For now, we simulate GPU metrics
    
    static int counter = 0;
    counter = (counter + 1) % 100;
    
    // Simulate GPU usage (0-100%)
    m_usage = 10.0f + 70.0f * std::sin(counter * 0.1f);
    m_usage = std::clamp(m_usage, 0.0f, 100.0f);
    
    // Simulate VRAM usage
    m_vramUsed = 1.0f + 6.0f * std::sin(counter * 0.05f);
    m_vramUsed = std::clamp(m_vramUsed, 0.1f, m_vramTotal - 0.1f);
    
    // Simulate temperature
    m_temperature = 30.0f + 50.0f * std::sin(counter * 0.08f);
    m_temperature = std::clamp(m_temperature, 20.0f, 95.0f);
    
    // Simulate fan speed
    m_fanSpeed = 1000 + static_cast<int>(4000 * std::sin(counter * 0.1f));
    m_fanSpeed = std::clamp(m_fanSpeed, 1000, 6000);
}

std::string GPUMonitorWidget::formatGPUText() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << m_usage << "%";
    return oss.str();
}

std::string GPUMonitorWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "GPU: " << m_gpuName << "\n";
    oss << "Usage: " << std::fixed << std::setprecision(1) << m_usage << "%\n";
    oss << "VRAM: " << std::fixed << std::setprecision(1) << m_vramUsed << "GB / " << m_vramTotal << "GB\n";
    oss << "Temperature: " << static_cast<int>(m_temperature) << "°C\n";
    oss << "Fan Speed: " << m_fanSpeed << " RPM";
    return oss.str();
}

lshell::design::Color GPUMonitorWidget::getUsageColor(float percentage) const {
    const auto& theme = getDesignTheme();
    
    if (percentage >= 80) return theme.color_danger;
    if (percentage >= 60) return theme.color_warning;
    if (percentage >= 40) return theme.color_info;
    return theme.color_success;
}

void GPUMonitorWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
        float usageWidth = availableWidth * 0.35f;
        m_usageLabel->setSize(usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->layout(renderer, usageWidth, containerHeight - 2 * paddingY);
        m_usageLabel->root()->setPosition({x, paddingY});
        x += usageWidth + spacing;
        availableWidth -= usageWidth + spacing;
    }
    
    // Layout VRAM
    if (m_vramLabel) {
        float vramWidth = availableWidth * 0.35f;
        m_vramLabel->setSize(vramWidth, containerHeight - 2 * paddingY);
        m_vramLabel->layout(renderer, vramWidth, containerHeight - 2 * paddingY);
        m_vramLabel->root()->setPosition({x, paddingY});
        x += vramWidth + spacing;
        availableWidth -= vramWidth + spacing;
    }
    
    // Layout temperature
    if (m_tempLabel) {
        float tempWidth = availableWidth * 0.3f;
        m_tempLabel->setSize(tempWidth, containerHeight - 2 * paddingY);
        m_tempLabel->layout(renderer, tempWidth, containerHeight - 2 * paddingY);
        m_tempLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void GPUMonitorWidget::doUpdate(Renderer& renderer) {
    // Update GPU status
    updateGPUStatus();
    
    // Update display
    updateDisplay();
}

void GPUMonitorWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_usageLabel) m_usageLabel->queueRedraw();
    if (m_vramLabel) m_vramLabel->queueRedraw();
    if (m_tempLabel) m_tempLabel->queueRedraw();
    if (m_fanLabel) m_fanLabel->queueRedraw();
}

void GPUMonitorWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// GPUMonitorWidget::Options Builder
// ============================================================================

GPUMonitorWidget::Options GPUMonitorWidget::OptionsBuilder::build() const {
    return options;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::showPercentage(bool show) {
    options.showPercentage = show;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::showVRAM(bool show) {
    options.showVRAM = show;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::showTemperature(bool show) {
    options.showTemperature = show;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::showFanSpeed(bool show) {
    options.showFanSpeed = show;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::compactMode(bool compact) {
    options.compactMode = compact;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

GPUMonitorWidget::OptionsBuilder& GPUMonitorWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
