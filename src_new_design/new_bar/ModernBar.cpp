// L-Shell Modern Bar - New Implementation
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/design/new_bar/ModernBar.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/design/spacing.h"
#include "shell/design/animations.h"

#include <cmath>

namespace lshell::design::bar {

// ============================================================================
// ModernBar Implementation
// ============================================================================

ModernBar::ModernBar(wl_output* output, const ModernBarConfig& config)
    : ::Bar(output), m_config(config), m_theme(lshell::design::Theme::get()) {
    
    // Apply initial theme
    apply_theme();
    
    // Calculate initial height
    m_calculated_height = calculate_height();
    
    // Set up animations
    m_show_animation = lshell::design::Animation{
        .duration = std::chrono::milliseconds(200),
        .easing = lshell::design::Easing::OutQuad
    };
    
    m_hide_animation = lshell::design::Animation{
        .duration = std::chrono::milliseconds(200),
        .easing = lshell::design::Easing::InQuad
    };
    
    // Register theme listener
    register_theme_listener();
}

ModernBar::~ModernBar() {
    unregister_theme_listener();
}

void ModernBar::set_config(const ModernBarConfig& config) {
    m_config = config;
    apply_theme();
    queue_redraw();
}

void ModernBar::apply_theme() {
    const auto& theme = lshell::design::Theme::get();
    const auto& colors = lshell::design::ColorPalette::get();
    
    // Use theme colors if not overridden
    m_background_color = m_config.background_color.value_or(colors.background_primary);
    m_border_color = m_config.border_color.value_or(colors.border_primary);
    m_opacity = m_config.opacity;
    m_blur_radius = m_config.blur_radius;
    
    // Apply to bar
    set_background_color(m_background_color);
    set_border_color(m_border_color);
    set_border_width(0);  // Modern design has no border by default
    
    // Apply transparency
    set_opacity(m_opacity);
    
    // Apply blur (if supported)
    set_blur(m_blur_radius);
}

void ModernBar::on_theme_changed(const lshell::design::Theme& theme) {
    apply_theme();
}

float ModernBar::calculate_height() const {
    switch (m_config.size) {
        case BarSize::XS: return 40.0f;
        case BarSize::SM: return 44.0f;
        case BarSize::MD: return 48.0f;
        case BarSize::LG: return 56.0f;
        case BarSize::XL: return 64.0f;
        default: return 48.0f;
    }
}

void ModernBar::set_position(BarPosition position) {
    m_config.position = position;
    queue_redraw();
}

void ModernBar::set_size(BarSize size) {
    m_config.size = size;
    m_calculated_height = calculate_height();
    queue_redraw();
}

void ModernBar::set_custom_height(int height) {
    // Override size preset
    m_config.size = BarSize::XL;  // Custom
    m_calculated_height = static_cast<float>(height);
    queue_redraw();
}

void ModernBar::set_opacity(float opacity) {
    m_config.opacity = opacity;
    m_opacity = opacity;
    set_opacity(opacity);
    queue_redraw();
}

void ModernBar::set_blur_radius(int radius) {
    m_config.blur_radius = radius;
    m_blur_radius = radius;
    set_blur(radius);
    queue_redraw();
}

void ModernBar::set_transparent(bool transparent) {
    m_config.transparent = transparent;
    set_transparent(transparent);
    queue_redraw();
}

void ModernBar::set_animation_enabled(bool enabled) {
    // Animation support would be implemented here
}

void ModernBar::animate_show() {
    if (m_visible || m_animating) return;
    
    m_animating = true;
    m_show_animation.start();
}

void ModernBar::animate_hide() {
    if (!m_visible || m_animating) return;
    
    m_animating = true;
    m_hide_animation.start();
}

void ModernBar::create() {
    ::Bar::create();
    
    // Set initial size
    set_height(m_calculated_height);
    
    // Set position
    switch (m_config.position) {
        case BarPosition::BOTTOM:
            set_position(Position::Bottom);
            break;
        case BarPosition::TOP:
            set_position(Position::Top);
            break;
        case BarPosition::LEFT:
            set_position(Position::Left);
            break;
        case BarPosition::RIGHT:
            set_position(Position::Right);
            break;
    }
    
    // Set layer
    set_layer(m_config.layer);
    
    // Set exclusive zone
    set_exclusive_zone(m_config.exclusive_zone);
}

void ModernBar::doLayout() {
    const auto& theme = lshell::design::Theme::get();
    const auto& spacing = lshell::design::Spacing::get();
    
    // Calculate available width
    float output_width = get_output_width();
    float padding_x = spacing.get(m_config.padding_x);
    float padding_y = spacing.get(m_config.padding_y);
    
    float available_width = output_width - 2 * padding_x;
    
    // Calculate section widths
    calculate_section_layouts(available_width);
    
    // Layout sections
    float x = padding_x;
    float widget_x = x;
    
    for (auto& widget : m_widgets) {
        float widget_width = widget->min_width();
        float widget_height = m_calculated_height - 2 * padding_y;
        
        layout_widget(widget.get(), widget_x, padding_y, widget_width, widget_height);
        widget_x += widget_width + spacing.get(m_config.widget_spacing);
    }
}

void ModernBar::calculate_section_layouts(float container_width) {
    // Calculate layout for each section
    // This is a simplified version - would be more complex in reality
}

void ModernBar::layout_widget(Widget* widget, float x, float y, float width, float height) {
    widget->set_position(x, y);
    widget->set_size(width, height);
    widget->layout();
}

void ModernBar::doUpdate() {
    // Update all widgets
    for (auto& widget : m_widgets) {
        widget->update();
    }
    
    // Update animations
    if (m_animating) {
        // Animation logic would go here
    }
}

void ModernBar::add_widget_to_section(std::unique_ptr<Widget> widget, BarSection section) {
    widget->set_parent(this);
    m_widgets.push_back(std::move(widget));
    
    // Sort widgets by section
    reorder_widgets();
    queue_redraw();
}

void ModernBar::remove_widget(Widget* widget) {
    auto it = std::remove_if(m_widgets.begin(), m_widgets.end(),
        [widget](const std::unique_ptr<Widget>& w) { return w.get() == widget; });
    
    if (it != m_widgets.end()) {
        m_widgets.erase(it);
        queue_redraw();
    }
}

void ModernBar::reorder_widgets() {
    // Sort widgets by section order: START -> APP_LAUNCHER -> WINDOWS -> WIDGETS -> SYSTEM
    std::sort(m_widgets.begin(), m_widgets.end(),
        [this](const std::unique_ptr<Widget>& a, const std::unique_ptr<Widget>& b) {
            // In a real implementation, we'd have section information stored with each widget
            return true; // Simplified
        });
    
    queue_redraw();
}

void ModernBar::handle_pointer_event(const PointerEvent& event) {
    // Handle pointer events for the bar
    ::Bar::handle_pointer_event(event);
}

void ModernBar::handle_frame() {
    // Handle frame updates
    ::Bar::handle_frame();
}

void ModernBar::register_theme_listener() {
    // Register to receive theme change notifications
    // Implementation would depend on the theme system
}

void ModernBar::unregister_theme_listener() {
    // Unregister from theme change notifications
}

} // namespace lshell::design::bar
