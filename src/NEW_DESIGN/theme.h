// L-Shell Design System - Theme
// Complete theme definition
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include "colors.h"
#include "typography.h"
#include "spacing.h"
#include "animations.h"

#include <string>
#include <vector>
#include <map>

namespace lshell::design {

// ============================================================================
// Theme Type
// ============================================================================

enum class ThemeType {
    DARK,      // Dark theme (default)
    LIGHT,     // Light theme
    SYSTEM,    // Follow system theme
    CUSTOM     // Custom user-defined theme
};

// ============================================================================
// Theme Definition
// ============================================================================

/**
 * Complete theme definition containing all design tokens
 */
class Theme {
public:
    ThemeType type;
    std::string name;
    std::string description;

    // Colors
    Color primary_main;
    Color primary_accent;
    Color primary_secondary;
    Color neutral_background;
    Color neutral_surface;
    Color neutral_overlay;
    Color text_primary;
    Color text_secondary;
    Color text_disabled;
    Color text_inverse;
    Color status_success;
    Color status_warning;
    Color status_error;
    Color status_info;

    // Semantic colors
    Color bar_background;
    Color bar_border;
    Color bar_hover;
    Color bar_active;
    Color cc_background;
    Color cc_border;
    Color cc_divider;
    Color widget_background;
    Color widget_hover;
    Color widget_active;
    Color widget_border;

    // Button colors
    Color button_primary_bg;
    Color button_primary_text;
    Color button_primary_hover;
    Color button_primary_active;
    Color button_secondary_bg;
    Color button_secondary_border;
    Color button_secondary_text;
    Color button_secondary_hover;
    Color button_secondary_active;
    Color button_tertiary_bg;
    Color button_tertiary_text;
    Color button_tertiary_hover;
    Color button_tertiary_active;

    // Input colors
    Color input_background;
    Color input_border;
    Color input_text;
    Color input_placeholder;
    Color input_focus_border;

    // Card colors
    Color card_background;
    Color card_border;
    Color card_hover;

    // Opacity levels
    float opacity_full;
    float opacity_high;
    float opacity_medium;
    float opacity_low;
    float opacity_subtle;
    float opacity_minimal;

    // Blur levels
    int blur_none;
    int blur_subtle;
    int blur_small;
    int blur_medium;
    int blur_large;
    int blur_xl;

    // Shadows
    Shadow shadow_none;
    Shadow shadow_small;
    Shadow shadow_medium;
    Shadow shadow_large;
    Shadow shadow_xl;

    // Typography
    FontFamily font_family_primary;
    FontFamily font_family_mono;
    FontFamily font_family_system;

    // Font sizes
    FontSize font_size_tiny;
    FontSize font_size_xs;
    FontSize font_size_sm;
    FontSize font_size_base;
    FontSize font_size_lg;
    FontSize font_size_xl;
    FontSize font_size_xxl;
    FontSize font_size_xxxl;

    // Spacing
    Spacing spacing_none;
    Spacing spacing_xs;
    Spacing spacing_sm;
    Spacing spacing_md;
    Spacing spacing_lg;
    Spacing spacing_xl;
    Spacing spacing_xxl;
    Spacing spacing_xxxl;

    // Bar spacing
    Spacing bar_height;
    Spacing bar_padding_x;
    Spacing bar_padding_y;
    Spacing bar_widget_spacing;
    Spacing bar_border_width;

    // Control Center spacing
    Spacing cc_width;
    Spacing cc_padding;
    Spacing cc_section_spacing;
    Spacing cc_item_spacing;
    Spacing cc_tab_spacing;
    Spacing cc_divider_height;

    // Widget spacing
    Spacing widget_min_width;
    Spacing widget_height;
    Spacing widget_padding_x;
    Spacing widget_padding_y;
    Spacing widget_icon_size;
    Spacing widget_text_spacing;

    // Border radii
    BorderRadius radius_none;
    BorderRadius radius_sm;
    BorderRadius radius_md;
    BorderRadius radius_lg;
    BorderRadius radius_xl;
    BorderRadius radius_full;
    BorderRadius radius_bar;
    BorderRadius radius_cc;
    BorderRadius radius_widget;
    BorderRadius radius_button;
    BorderRadius radius_input;
    BorderRadius radius_card;

    // Animation durations
    Duration duration_fastest;
    Duration duration_faster;
    Duration duration_fast;
    Duration duration_normal;
    Duration duration_slow;
    Duration duration_slower;
    Duration duration_long;

    // Common animations
    Animation anim_fade_in;
    Animation anim_fade_out;
    Animation anim_scale_in;
    Animation anim_scale_out;
    Animation anim_slide_in_from_right;
    Animation anim_slide_out_to_right;

public:
    Theme(ThemeType type = ThemeType::DARK);
    virtual ~Theme() = default;

    // Create dark theme
    static Theme create_dark_theme();

    // Create light theme
    static Theme create_light_theme();

    // Create system theme (detects system preference)
    static Theme create_system_theme();

    // Load theme from configuration
    static Theme load_from_config(const std::string& config_path);

    // Save theme to configuration
    void save_to_config(const std::string& config_path) const;

    // Apply theme to application
    void apply() const;

    // Check if dark theme
    bool is_dark() const { return type == ThemeType::DARK || (type == ThemeType::SYSTEM && is_system_dark()); }

    // Get text color based on background (for accessibility)
    Color get_contrast_text(Color background) const;

    // Blend two colors with theme opacity
    Color blend_colors(Color a, Color b, float ratio) const;

    // Get color with theme opacity applied
    Color with_theme_opacity(Color color, float opacity) const;

private:
    // Detect if system is using dark theme
    static bool is_system_dark();

    // Initialize with dark theme defaults
    void init_dark();

    // Initialize with light theme defaults
    void init_light();
};

// ============================================================================
// Theme Manager (Singleton)
// ============================================================================

class ThemeManager {
public:
    static ThemeManager& get_instance() {
        static ThemeManager instance;
        return instance;
    }

    // Get current theme
    const Theme& get_theme() const { return *current_theme; }
    Theme& get_theme() { return *current_theme; }

    // Set current theme
    void set_theme(std::unique_ptr<Theme> theme);

    // Set theme by type
    void set_theme_type(ThemeType type);

    // Set custom theme
    void set_custom_theme(std::unique_ptr<Theme> theme);

    // Reload theme from configuration
    void reload_from_config();

    // Check if dark theme
    bool is_dark_theme() const;

    // Listen for theme changes
    using ThemeChangeCallback = std::function<void(const Theme&)>;
    void add_theme_change_listener(ThemeChangeCallback callback);
    void remove_theme_change_listener(ThemeChangeCallback callback);

private:
    ThemeManager();
    ~ThemeManager();

    std::unique_ptr<Theme> current_theme;
    std::vector<ThemeChangeCallback> change_listeners;

    void notify_theme_change();
};

// ============================================================================
// Theme Helper Functions
// ============================================================================

// Get current theme
const Theme& get_theme();
Theme& get_theme_mutable();

// Set theme type
void set_theme_type(ThemeType type);

// Check if dark theme
bool is_dark_theme();

// ============================================================================
// Theme Overrides (for component-specific styling)
// ============================================================================

/**
 * Component-specific theme overrides
 */
struct ThemeOverride {
    std::map<std::string, Color> colors;
    std::map<std::string, Spacing> spacing;
    std::map<std::string, BorderRadius> radii;
    std::map<std::string, FontSize> font_sizes;

    // Apply overrides to theme
    void apply_to(Theme& theme) const;
};

// ============================================================================
// Theme Constants for Easy Access
// ============================================================================

// Current theme access
#define THEME lshell::design::get_theme()
#define THEME_COLOR(name) THEME.name
#define THEME_SPACING(name) THEME.name

} // namespace lshell::design
