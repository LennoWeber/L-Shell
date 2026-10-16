// L-Shell PowerProfile Widget - NEW IMPLEMENTATION
// Based on DESIGN-PLAN-2026
// Uses the new design system
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "shell/bar/widgets/powerprofile_widget.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

// ============================================================================
// PowerProfileWidget Implementation
// ============================================================================

PowerProfileWidget::PowerProfileWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output), m_options(std::move(options)) {
    // Apply NEW design system
    applyDesignTheme();
}

void PowerProfileWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create label
    m_label = new Label(getDisplayText());
    m_label->setFontFamily(getPrimaryFontFamily().primary);
    m_label->setFontSize(theme.font_size_sm.size);
    m_label->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
    m_label->setColor(theme.text_primary);
    
    // Set tooltip
    setTooltip("PowerProfile widget");
    
    // Create container node
    m_innerRoot = new Node;
    m_innerRoot->setData(this);
    m_innerRoot->addChild(m_label->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width, theme.widget_height);
}

void PowerProfileWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
    const auto& theme = getDesignTheme();
    
    float paddingX = static_cast<float>(theme.widget_padding_x);
    float paddingY = static_cast<float>(theme.widget_padding_y);
    float availableWidth = containerWidth - 2 * paddingX;
    
    m_label->setSize(availableWidth, containerHeight - 2 * paddingY);
    m_label->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
    m_label->root()->setPosition({paddingX, paddingY});
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void PowerProfileWidget::doUpdate(Renderer& renderer) {
    const auto& theme = getDesignTheme();
    
    // Update label text
    std::string newText = getDisplayText();
    if (newText != m_lastText) {
        m_lastText = newText;
        m_label->setText(newText);
    }
    
    // Update colors from theme
    m_label->setColor(theme.text_primary);
}

void PowerProfileWidget::onThemeChanged(const lshell::design::Theme& theme) {
    // Update label colors
    if (m_label) {
        m_label->setColor(theme.text_primary);
        m_label->queueRedraw();
    }
}

std::string PowerProfileWidget::getDisplayText() {
    // Default implementation - override in derived classes
    return "PowerProfile";
}

// ============================================================================
// Helper functions
// ============================================================================
