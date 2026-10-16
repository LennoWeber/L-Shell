// L-Shell Modern Bar - New Implementation
// Based on DESIGN-PLAN-2026
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include "shell/design/theme.h"
#include "shell/bar/bar.h"

#include <vector>
#include <memory>

namespace lshell::design::bar {

// ============================================================================
// Bar Position
// ============================================================================

enum class BarPosition {
    BOTTOM,    // Default for Hyprland
    TOP,
    LEFT,
    RIGHT
};

// ============================================================================
// Bar Size Presets
// ============================================================================

enum class BarSize {
    XS,        // 40px
    SM,        // 44px
    MD,        // 48px (default)
    LG,        // 56px
    XL         // 64px
};

// ============================================================================
// Modern Bar Configuration
// ============================================================================

struct ModernBarConfig {
    BarPosition position = BarPosition::BOTTOM;
    BarSize size = BarSize::MD;
    bool auto_hide = false;
    bool hide_on_fullscreen = true;
    int layer = 0;  // Wayland layer
    bool exclusive_zone = true;  // Reserve space
    
    // Design options
    bool transparent = true;
    float opacity = 0.85f;  // 85%
    int blur_radius = 8;   // 8px blur
    
    // Colors (override theme)
    std::optional<lshell::design::Color> background_color;
    std::optional<lshell::design::Color> border_color;
    
    // Spacing
    lshell::design::Spacing padding_x = lshell::design::SPACING_MD;  // 16px
    lshell::design::Spacing padding_y = lshell::design::SPACING_SM;  // 8px
    lshell::design::Spacing widget_spacing = lshell::design::SPACING_SM; // 8px
};

// ============================================================================
// Modern Bar Section
// ============================================================================

enum class BarSection {
    START,       // Left/Top-Start
    APP_LAUNCHER,
    WINDOWS,     // Center
    WIDGETS,     // Right/Bottom-End
    SYSTEM       // Far right/bottom
};

struct BarSectionConfig {
    BarSection section;
    float min_width;  // Minimum width
    float max_width;  // Maximum width
    bool expand;      // Should this section expand to fill space?
    lshell::design::Align alignment = lshell::design::Align::START;
};

// ============================================================================
// Modern Bar - Main Class
// ============================================================================

class ModernBar : public ::Bar {
public:
    explicit ModernBar(wl_output* output, const ModernBarConfig& config = {});
    ~ModernBar() override;

    // Configuration
    void set_config(const ModernBarConfig& config);
    const ModernBarConfig& get_config() const { return m_config; }

    // Theme
    void apply_theme();
    void on_theme_changed(const lshell::design::Theme& theme);

    // Layout
    void doLayout() override;
    void doUpdate() override;

    // Widget management
    void add_widget_to_section(std::unique_ptr<Widget> widget, BarSection section);
    void remove_widget(Widget* widget);
    void reorder_widgets();

    // Position and size
    void set_position(BarPosition position);
    void set_size(BarSize size);
    void set_custom_height(int height);

    // Visual properties
    void set_opacity(float opacity);
    void set_blur_radius(int radius);
    void set_transparent(bool transparent);

    // Animation
    void set_animation_enabled(bool enabled);
    void animate_show();
    void animate_hide();

protected:
    void create() override;
    void handle_pointer_event(const PointerEvent& event) override;
    void handle_frame() override;

private:
    // Configuration
    ModernBarConfig m_config;
    
    // Theme
    const lshell::design::Theme& m_theme;
    lshell::design::Color m_background_color;
    lshell::design::Color m_border_color;
    float m_opacity;
    int m_blur_radius;
    
    // Layout
    std::vector<std::unique_ptr<Widget>> m_widgets;
    std::vector<BarSectionConfig> m_sections;
    
    // State
    bool m_visible = true;
    bool m_animating = false;
    float m_animation_progress = 0.0f;
    lshell::design::Animation m_show_animation;
    lshell::design::Animation m_hide_animation;
    
    // Calculated values
    float m_calculated_height;
    
    // Theme change listener
    void register_theme_listener();
    void unregister_theme_listener();
    
    // Layout calculations
    float calculate_height() const;
    void calculate_section_layouts(float container_width);
    
    // Rendering
    void render_background(Renderer& renderer);
    void render_border(Renderer& renderer);
    
    // Widget layout
    void layout_widget(Widget* widget, float x, float y, float width, float height);
};

// ============================================================================
// Bar Builder (Fluent API for creating bars)
// ============================================================================

class ModernBarBuilder {
public:
    ModernBarBuilder(wl_output* output) : m_output(output) {}

    ModernBarBuilder& with_position(BarPosition position) {
        m_config.position = position;
        return *this;
    }

    ModernBarBuilder& with_size(BarSize size) {
        m_config.size = size;
        return *this;
    }

    ModernBarBuilder& with_height(int height) {
        m_config.size = BarSize::XL;  // Custom
        m_custom_height = height;
        return *this;
    }

    ModernBarBuilder& transparent(bool transparent = true) {
        m_config.transparent = transparent;
        return *this;
    }

    ModernBarBuilder& opacity(float opacity) {
        m_config.opacity = opacity;
        return *this;
    }

    ModernBarBuilder& blur(int radius) {
        m_config.blur_radius = radius;
        return *this;
    }

    ModernBarBuilder& padding(lshell::design::Spacing x, lshell::design::Spacing y) {
        m_config.padding_x = x;
        m_config.padding_y = y;
        return *this;
    }

    ModernBarBuilder& widget_spacing(lshell::design::Spacing spacing) {
        m_config.widget_spacing = spacing;
        return *this;
    }

    std::unique_ptr<ModernBar> build() {
        if (m_custom_height) {
            // Handle custom height
        }
        return std::make_unique<ModernBar>(m_output, m_config);
    }

private:
    wl_output* m_output;
    ModernBarConfig m_config;
    std::optional<int> m_custom_height;
};

} // namespace lshell::design::bar
