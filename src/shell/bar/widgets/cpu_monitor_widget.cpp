// L-Shell CPU Monitor Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/cpu_monitor_widget.h"
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
// CPUMonitorWidget Implementation
// ============================================================================

CPUMonitorWidget::CPUMonitorWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_usage(0),
      m_coreUsage(8, 0), // 8 cores by default
      m_temperature(0),
      m_frequency(0),
      m_cpuName("AMD Ryzen 9 7950X3D"),
      m_icon(nullptr),
      m_usageLabel(nullptr),
      m_tempLabel(nullptr),
      m_freqLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
}

CPUMonitorWidget::~CPUMonitorWidget() {
    delete m_icon;
    delete m_usageLabel;
    delete m_tempLabel;
    delete m_freqLabel;
    delete m_innerRoot;
}

void CPUMonitorWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create usage label
    if (m_options.showOverall) {
        m_usageLabel = new Label("");
        m_usageLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_usageLabel->setFontSize(theme.font_size_sm.size);
        m_usageLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_usageLabel->setLetterSpacing(m_options.letterSpacing);
        m_usageLabel->setLineHeight(m_options.lineHeight);
        m_usageLabel->setColor(theme.text_primary);
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
    
    // Create frequency label
    if (m_options.showFrequency) {
        m_freqLabel = new Label("");
        m_freqLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_freqLabel->setFontSize(theme.font_size_tiny.size);
        m_freqLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_freqLabel->setLetterSpacing(m_options.letterSpacing);
        m_freqLabel->setLineHeight(m_options.lineHeight);
        m_freqLabel->setColor(theme.text_secondary);
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
    if (m_tempLabel) m_innerRoot->addChild(m_tempLabel->root());
    if (m_freqLabel) m_innerRoot->addChild(m_freqLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void CPUMonitorWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update CPU status
    updateCPUStatus();
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(getUsageColor(m_usage));
    }
    
    // Update usage label
    if (m_usageLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << m_usage << "% ";
        if (m_options.showPerCore) {
            oss << "[" << m_coreUsage.size() << " cores]";
        }
        m_usageLabel->setText(oss.str());
        m_usageLabel->setColor(getUsageColor(m_usage));
    }
    
    // Update temperature label
    if (m_tempLabel) {
        std::ostringstream oss;
        oss << static_cast<int>(m_temperature) << "°C";
        m_tempLabel->setText(oss.str());
        m_tempLabel->setColor(m_temperature > 70 ? theme.color_danger : theme.color_warning);
    }
    
    // Update frequency label
    if (m_freqLabel) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << m_frequency << "GHz";
        m_freqLabel->setText(oss.str());
        m_freqLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void CPUMonitorWidget::updateCPUStatus() {
    // In a real implementation, this would read from /proc/stat or sysfs
    // For now, we simulate CPU metrics
    
    static int counter = 0;
    counter = (counter + 1) % 100;
    
    // Simulate overall CPU usage (0-100%)
    m_usage = 15.0f + 65.0f * std::sin(counter * 0.1f);
    m_usage = std::clamp(m_usage, 0.0f, 100.0f);
    
    // Simulate per-core usage
    for (size_t i = 0; i < m_coreUsage.size(); ++i) {
        m_coreUsage[i] = 5.0f + 80.0f * std::sin(counter * 0.1f + i * 0.2f);
        m_coreUsage[i] = std::clamp(m_coreUsage[i], 0.0f, 100.0f);
    }
    
    // Simulate temperature
    m_temperature = 30.0f + 50.0f * std::sin(counter * 0.05f);
    m_temperature = std::clamp(m_temperature, 20.0f, 95.0f);
    
    // Simulate frequency
    m_frequency = 2.5f + 2.0f * std::sin(counter * 0.1f);
    m_frequency = std::clamp(m_frequency, 1.0f, 5.5f);
}

std::string CPUMonitorWidget::formatCPUText() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << m_usage << "%";
    return oss.str();
}

std::string CPUMonitorWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "CPU: " << m_cpuName << "\n";
    oss << "Usage: " << std::fixed << std::setprecision(1) << m_usage << "%\n";
    
    if (m_options.showPerCore) {
        oss << "Core Usage: ";
        for (size_t i = 0; i < m_coreUsage.size(); ++i) {
            if (i > 0) oss << " ";
            oss << std::fixed << std::setprecision(0) << m_coreUsage[i] << "%";
        }
        oss << "\n";
    }
    
    if (m_options.showTemperature) {
        oss << "Temperature: " << static_cast<int>(m_temperature) << "°C\n";
    }
    if (m_options.showFrequency) {
        oss << "Frequency: " << std::fixed << std::setprecision(2) << m_frequency << "GHz";
    }
    
    return oss.str();
}

lshell::design::Color CPUMonitorWidget::getUsageColor(float percentage) const {
    const auto& theme = getDesignTheme();
    
    if (percentage >= 80) return theme.color_danger;
    if (percentage >= 60) return theme.color_warning;
    if (percentage >= 40) return theme.color_info;
    return theme.color_success;
}

void CPUMonitorWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
    
    // Layout temperature
    if (m_tempLabel) {
        float tempWidth = availableWidth * 0.3f;
        m_tempLabel->setSize(tempWidth, containerHeight - 2 * paddingY);
        m_tempLabel->layout(renderer, tempWidth, containerHeight - 2 * paddingY);
        m_tempLabel->root()->setPosition({x, paddingY});
        x += tempWidth + spacing;
        availableWidth -= tempWidth + spacing;
    }
    
    // Layout frequency
    if (m_freqLabel) {
        m_freqLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_freqLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_freqLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void CPUMonitorWidget::doUpdate(Renderer& renderer) {
    // Update CPU status
    updateCPUStatus();
    
    // Update display
    updateDisplay();
}

void CPUMonitorWidget::onThemeChanged(const lshell::design::Theme& theme) {
    updateDisplay(); // Re-calculate colors
    
    if (m_icon) m_icon->queueRedraw();
    if (m_usageLabel) m_usageLabel->queueRedraw();
    if (m_tempLabel) m_tempLabel->queueRedraw();
    if (m_freqLabel) m_freqLabel->queueRedraw();
}

void CPUMonitorWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// CPUMonitorWidget::Options Builder
// ============================================================================

CPUMonitorWidget::Options CPUMonitorWidget::OptionsBuilder::build() const {
    return options;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::showOverall(bool show) {
    options.showOverall = show;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::showPerCore(bool show) {
    options.showPerCore = show;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::showTemperature(bool show) {
    options.showTemperature = show;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::showFrequency(bool show) {
    options.showFrequency = show;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

CPUMonitorWidget::OptionsBuilder& CPUMonitorWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
