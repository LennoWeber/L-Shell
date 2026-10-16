// L-Shell Design System - Theme Implementation
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "theme.h"
#include "colors.h"
#include "typography.h"
#include "spacing.h"
#include "animations.h"

#include <memory>
#include <stdexcept>

// For system theme detection
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace lshell::design {

// ============================================================================
// Theme Implementation
// ============================================================================

Theme::Theme(ThemeType type) : type(type) {
    if (type == ThemeType::DARK || type == ThemeType::SYSTEM) {
        init_dark();
    } else if (type == ThemeType::LIGHT) {
        init_light();
    }
}

Theme Theme::create_dark_theme() {
    Theme theme(ThemeType::DARK);
    theme.name = "Dark";
    theme.description = "Modern dark theme with blue accents";
    theme.init_dark();
    return theme;
}

Theme Theme::create_light_theme() {
    Theme theme(ThemeType::LIGHT);
    theme.name = "Light";
    theme.description = "Clean light theme with blue accents";
    theme.init_light();
    return theme;
}

Theme Theme::create_system_theme() {
    Theme theme(ThemeType::SYSTEM);
    theme.name = "System";
    theme.description = "Follows system theme preference";
    
    if (is_system_dark()) {
        theme.init_dark();
    } else {
        theme.init_light();
    }
    
    return theme;
}

void Theme::init_dark() {
    type = ThemeType::DARK;
    
    // Primary colors
    primary_main = PRIMARY_MAIN;
    primary_accent = PRIMARY_ACCENT;
    primary_secondary = PRIMARY_SECONDARY;
    
    // Neutral colors
    neutral_background = NEUTRAL_BACKGROUND;
    neutral_surface = NEUTRAL_SURFACE;
    neutral_overlay = NEUTRAL_OVERLAY;
    
    // Text colors
    text_primary = TEXT_PRIMARY;
    text_secondary = TEXT_SECONDARY;
    text_disabled = TEXT_DISABLED;
    text_inverse = TEXT_INVERSE;
    
    // Status colors
    status_success = STATUS_SUCCESS;
    status_warning = STATUS_WARNING;
    status_error = STATUS_ERROR;
    status_info = STATUS_INFO;
    
    // Semantic colors
    bar_background = BAR_BACKGROUND;
    bar_border = BAR_BORDER;
    bar_hover = BAR_HOVER;
    bar_active = BAR_ACTIVE;
    
    cc_background = CC_BACKGROUND;
    cc_border = CC_BORDER;
    cc_divider = CC_DIVIDER;
    
    widget_background = WIDGET_BACKGROUND;
    widget_hover = WIDGET_HOVER;
    widget_active = WIDGET_ACTIVE;
    widget_border = WIDGET_BORDER;
    
    // Button colors
    button_primary_bg = BUTTON_PRIMARY_BG;
    button_primary_text = BUTTON_PRIMARY_TEXT;
    button_primary_hover = BUTTON_PRIMARY_HOVER;
    button_primary_active = BUTTON_PRIMARY_ACTIVE;
    
    button_secondary_bg = BUTTON_SECONDARY_BG;
    button_secondary_border = BUTTON_SECONDARY_BORDER;
    button_secondary_text = BUTTON_SECONDARY_TEXT;
    button_secondary_hover = BUTTON_SECONDARY_HOVER;
    button_secondary_active = BUTTON_SECONDARY_ACTIVE;
    
    button_tertiary_bg = BUTTON_TERTIARY_BG;
    button_tertiary_text = BUTTON_TERTIARY_TEXT;
    button_tertiary_hover = BUTTON_TERTIARY_HOVER;
    button_tertiary_active = BUTTON_TERTIARY_ACTIVE;
    
    // Input colors
    input_background = INPUT_BACKGROUND;
    input_border = INPUT_BORDER;
    input_text = INPUT_TEXT;
    input_placeholder = INPUT_PLACEHOLDER;
    input_focus_border = INPUT_FOCUS_BORDER;
    
    // Card colors
    card_background = CARD_BACKGROUND;
    card_border = CARD_BORDER;
    card_hover = CARD_HOVER;
    
    // Opacity levels
    opacity_full = OPACITY_FULL;
    opacity_high = OPACITY_HIGH;
    opacity_medium = OPACITY_MEDIUM;
    opacity_low = OPACITY_LOW;
    opacity_subtle = OPACITY_SUBTLE;
    opacity_minimal = OPACITY_MINIMAL;
    
    // Blur levels
    blur_none = BLUR_NONE;
    blur_subtle = BLUR_SUBTLE;
    blur_small = BLUR_SMALL;
    blur_medium = BLUR_MEDIUM;
    blur_large = BLUR_LARGE;
    blur_xl = BLUR_XL;
    
    // Shadows
    shadow_none = SHADOW_NONE;
    shadow_small = SHADOW_SMALL;
    shadow_medium = SHADOW_MEDIUM;
    shadow_large = SHADOW_LARGE;
    shadow_xl = SHADOW_XL;
    
    // Typography
    font_family_primary = FONT_FAMILY_PRIMARY;
    font_family_mono = FONT_FAMILY_MONO;
    font_family_system = FONT_FAMILY_SYSTEM;
    
    font_size_tiny = FONT_SIZE_TINY;
    font_size_xs = FONT_SIZE_XS;
    font_size_sm = FONT_SIZE_SM;
    font_size_base = FONT_SIZE_BASE;
    font_size_lg = FONT_SIZE_LG;
    font_size_xl = FONT_SIZE_XL;
    font_size_xxl = FONT_SIZE_XXL;
    font_size_xxxl = FONT_SIZE_XXXL;
    
    // Spacing
    spacing_none = SPACING_NONE;
    spacing_xs = SPACING_XS;
    spacing_sm = SPACING_SM;
    spacing_md = SPACING_MD;
    spacing_lg = SPACING_LG;
    spacing_xl = SPACING_XL;
    spacing_xxl = SPACING_XXL;
    spacing_xxxl = SPACING_XXXL;
    
    // Bar spacing
    bar_height = BAR_HEIGHT_MD;
    bar_padding_x = BAR_PADDING_X;
    bar_padding_y = BAR_PADDING_Y;
    bar_widget_spacing = BAR_WIDGET_SPACING;
    bar_border_width = BAR_BORDER_WIDTH;
    
    // Control Center spacing
    cc_width = CC_WIDTH_MD;
    cc_padding = CC_PADDING;
    cc_section_spacing = CC_SECTION_SPACING;
    cc_item_spacing = CC_ITEM_SPACING;
    cc_tab_spacing = CC_TAB_SPACING;
    cc_divider_height = CC_DIVIDER_HEIGHT;
    
    // Widget spacing
    widget_min_width = WIDGET_MIN_WIDTH;
    widget_height = WIDGET_HEIGHT;
    widget_padding_x = WIDGET_PADDING_X;
    widget_padding_y = WIDGET_PADDING_Y;
    widget_icon_size = WIDGET_ICON_SIZE;
    widget_text_spacing = WIDGET_TEXT_SPACING;
    
    // Border radii
    radius_none = RADIUS_NONE;
    radius_sm = RADIUS_SM;
    radius_md = RADIUS_MD;
    radius_lg = RADIUS_LG;
    radius_xl = RADIUS_XL;
    radius_full = RADIUS_FULL;
    radius_bar = RADIUS_BAR;
    radius_cc = RADIUS_CC;
    radius_widget = RADIUS_WIDGET;
    radius_button = RADIUS_BUTTON;
    radius_input = RADIUS_INPUT;
    radius_card = RADIUS_CARD;
    
    // Animation durations
    duration_fastest = DURATION_FASTEST;
    duration_faster = DURATION_FASTER;
    duration_fast = DURATION_FAST;
    duration_normal = DURATION_NORMAL;
    duration_slow = DURATION_SLOW;
    duration_slower = DURATION_SLOWER;
    duration_long = DURATION_LONG;
    
    // Common animations
    anim_fade_in = ANIM_FADE_IN;
    anim_fade_out = ANIM_FADE_OUT;
    anim_scale_in = ANIM_SCALE_IN;
    anim_scale_out = ANIM_SCALE_OUT;
    anim_slide_in_from_right = ANIM_SLIDE_IN_FROM_RIGHT;
    anim_slide_out_to_right = ANIM_SLIDE_OUT_TO_RIGHT;
}

void Theme::init_light() {
    type = ThemeType::LIGHT;
    
    // Primary colors - keep the same but adjust for light theme
    primary_main = Color(0x19, 0x76, 0xd2, 0xff);  // #1976d2 - Blue
    primary_accent = Color(0x21, 0x96, 0xf3, 0xff);  // #2196f3 - Lighter blue
    primary_secondary = Color(0x42, 0xa5, 0xf5, 0xff);  // #42a5f5 - Even lighter blue
    
    // Neutral colors - light variants
    neutral_background = Color(0xf5, 0xf5, 0xf5, 0xff);  // #f5f5f5 - Light gray
    neutral_surface = Color(0xff, 0xff, 0xff, 0xff);  // #ffffff - White
    neutral_overlay = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black
    
    // Text colors - dark variants
    text_primary = Color(0x21, 0x21, 0x21, 0xff);  // #212121 - Dark gray
    text_secondary = Color(0x75, 0x75, 0x75, 0xff);  // #757575 - Gray
    text_disabled = Color(0xb0, 0xb0, 0xb0, 0xff);  // #b0b0b0 - Light gray
    text_inverse = Color(0xff, 0xff, 0xff, 0xff);  // White
    
    // Status colors - slightly adjusted for light theme
    status_success = Color(0x2e, 0x7d, 0x32, 0xff);  // #2e7d32 - Darker green
    status_warning = Color(0xef, 0x6c, 0x00, 0xff);  // #ef6c00 - Darker orange
    status_error = Color(0xc6, 0x28, 0x28, 0xff);  // #c62828 - Darker red
    status_info = Color(0x0d, 0x47, 0xa1, 0xff);  // #0d47a1 - Darker blue
    
    // Semantic colors
    bar_background = Color(0xff, 0xff, 0xff, 0xe6);  // 90% white
    bar_border = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black
    bar_hover = Color(0x00, 0x00, 0x00, 0x0f);  // 5% black
    bar_active = primary_secondary;
    
    cc_background = Color(0xff, 0xff, 0xff, 0xff);  // White
    cc_border = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black
    cc_divider = Color(0x00, 0x00, 0x00, 0x0f);  // 5% black
    
    widget_background = Color(0x00, 0x00, 0x00, 0x00);  // Transparent
    widget_hover = Color(0x00, 0x00, 0x00, 0x0f);  // 5% black
    widget_active = Color(0x19, 0x76, 0xd2, 0x26);  // 15% primary
    widget_border = Color(0x00, 0x00, 0x00, 0x00);  // Transparent
    
    // Button colors
    button_primary_bg = primary_secondary;
    button_primary_text = Color(0xff, 0xff, 0xff, 0xff);  // White
    button_primary_hover = Color(0x15, 0x65, 0xc0, 0xff);  // Darker blue
    button_primary_active = Color(0x0d, 0x47, 0xa1, 0xff);  // Even darker blue
    
    button_secondary_bg = Color(0x00, 0x00, 0x00, 0x00);  // Transparent
    button_secondary_border = Color(0x00, 0x00, 0x00, 0x80);  // 50% black
    button_secondary_text = text_primary;
    button_secondary_hover = Color(0x00, 0x00, 0x00, 0x0f);  // 5% black bg
    button_secondary_active = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black bg
    
    button_tertiary_bg = Color(0x00, 0x00, 0x00, 0x00);  // Transparent
    button_tertiary_text = text_secondary;
    button_tertiary_hover = Color(0x00, 0x00, 0x00, 0x0f);  // 5% black bg
    button_tertiary_active = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black bg
    
    // Input colors
    input_background = Color(0xff, 0xff, 0xff, 0xff);  // White
    input_border = Color(0x00, 0x00, 0x00, 0x33);  // 20% black
    input_text = text_primary;
    input_placeholder = Color(0x75, 0x75, 0x75, 0x80);  // 50% gray
    input_focus_border = primary_secondary;
    
    // Card colors
    card_background = Color(0xff, 0xff, 0xff, 0xff);  // White
    card_border = Color(0x00, 0x00, 0x00, 0x1a);  // 10% black
    card_hover = Color(0x00, 0x00, 0x00, 0x05);  // 2% black
    
    // Opacity levels (same as dark)
    opacity_full = OPACITY_FULL;
    opacity_high = OPACITY_HIGH;
    opacity_medium = OPACITY_MEDIUM;
    opacity_low = OPACITY_LOW;
    opacity_subtle = OPACITY_SUBTLE;
    opacity_minimal = OPACITY_MINIMAL;
    
    // Blur levels (same as dark)
    blur_none = BLUR_NONE;
    blur_subtle = BLUR_SUBTLE;
    blur_small = BLUR_SMALL;
    blur_medium = BLUR_MEDIUM;
    blur_large = BLUR_LARGE;
    blur_xl = BLUR_XL;
    
    // Shadows - adjusted for light theme
    shadow_none = Shadow(Color(0x00, 0x00, 0x00, 0x00), 0, 0, 0, 0);
    shadow_small = Shadow(Color(0x00, 0x00, 0x00, 0x1a), 0, 2, 4, 0);  // 10%
    shadow_medium = Shadow(Color(0x00, 0x00, 0x00, 0x1a), 0, 4, 8, 0);  // 10%
    shadow_large = Shadow(Color(0x00, 0x00, 0x00, 0x26), 0, 8, 16, 0);  // 15%
    shadow_xl = Shadow(Color(0x00, 0x00, 0x00, 0x26), 0, 16, 32, 0);  // 15%
    
    // Typography (same as dark)
    font_family_primary = FONT_FAMILY_PRIMARY;
    font_family_mono = FONT_FAMILY_MONO;
    font_family_system = FONT_FAMILY_SYSTEM;
    
    font_size_tiny = FONT_SIZE_TINY;
    font_size_xs = FONT_SIZE_XS;
    font_size_sm = FONT_SIZE_SM;
    font_size_base = FONT_SIZE_BASE;
    font_size_lg = FONT_SIZE_LG;
    font_size_xl = FONT_SIZE_XL;
    font_size_xxl = FONT_SIZE_XXL;
    font_size_xxxl = FONT_SIZE_XXXL;
    
    // Spacing (same as dark)
    spacing_none = SPACING_NONE;
    spacing_xs = SPACING_XS;
    spacing_sm = SPACING_SM;
    spacing_md = SPACING_MD;
    spacing_lg = SPACING_LG;
    spacing_xl = SPACING_XL;
    spacing_xxl = SPACING_XXL;
    spacing_xxxl = SPACING_XXXL;
    
    // Bar spacing (same as dark)
    bar_height = BAR_HEIGHT_MD;
    bar_padding_x = BAR_PADDING_X;
    bar_padding_y = BAR_PADDING_Y;
    bar_widget_spacing = BAR_WIDGET_SPACING;
    bar_border_width = BAR_BORDER_WIDTH;
    
    // Control Center spacing (same as dark)
    cc_width = CC_WIDTH_MD;
    cc_padding = CC_PADDING;
    cc_section_spacing = CC_SECTION_SPACING;
    cc_item_spacing = CC_ITEM_SPACING;
    cc_tab_spacing = CC_TAB_SPACING;
    cc_divider_height = CC_DIVIDER_HEIGHT;
    
    // Widget spacing (same as dark)
    widget_min_width = WIDGET_MIN_WIDTH;
    widget_height = WIDGET_HEIGHT;
    widget_padding_x = WIDGET_PADDING_X;
    widget_padding_y = WIDGET_PADDING_Y;
    widget_icon_size = WIDGET_ICON_SIZE;
    widget_text_spacing = WIDGET_TEXT_SPACING;
    
    // Border radii (same as dark)
    radius_none = RADIUS_NONE;
    radius_sm = RADIUS_SM;
    radius_md = RADIUS_MD;
    radius_lg = RADIUS_LG;
    radius_xl = RADIUS_XL;
    radius_full = RADIUS_FULL;
    radius_bar = RADIUS_BAR;
    radius_cc = RADIUS_CC;
    radius_widget = RADIUS_WIDGET;
    radius_button = RADIUS_BUTTON;
    radius_input = RADIUS_INPUT;
    radius_card = RADIUS_CARD;
    
    // Animation durations (same as dark)
    duration_fastest = DURATION_FASTEST;
    duration_faster = DURATION_FASTER;
    duration_fast = DURATION_FAST;
    duration_normal = DURATION_NORMAL;
    duration_slow = DURATION_SLOW;
    duration_slower = DURATION_SLOWER;
    duration_long = DURATION_LONG;
    
    // Common animations (same as dark)
    anim_fade_in = ANIM_FADE_IN;
    anim_fade_out = ANIM_FADE_OUT;
    anim_scale_in = ANIM_SCALE_IN;
    anim_scale_out = ANIM_SCALE_OUT;
    anim_slide_in_from_right = ANIM_SLIDE_IN_FROM_RIGHT;
    anim_slide_out_to_right = ANIM_SLIDE_OUT_TO_RIGHT;
}

Theme Theme::load_from_config(const std::string& config_path) {
    // In a real implementation, this would load from a configuration file
    // For now, just return the default dark theme
    return create_dark_theme();
}

void Theme::save_to_config(const std::string& config_path) const {
    // In a real implementation, this would save to a configuration file
    // For now, do nothing
}

void Theme::apply() const {
    // In a real implementation, this would apply the theme to the application
    // by updating UI elements, colors, etc.
}

Color Theme::get_contrast_text(Color background) const {
    // Calculate luminance using relative luminance formula
    float r = static_cast<float>(background.r) / 255.0f;
    float g = static_cast<float>(background.g) / 255.0f;
    float b = static_cast<float>(background.b) / 255.0f;
    
    // Apply gamma correction
    r = (r <= 0.03928f) ? r / 12.92f : std::pow((r + 0.055f) / 1.055f, 2.4f);
    g = (g <= 0.03928f) ? g / 12.92f : std::pow((g + 0.055f) / 1.055f, 2.4f);
    b = (b <= 0.03928f) ? b / 12.92f : std::pow((b + 0.055f) / 1.055f, 2.4f);
    
    float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    
    // Use dark text for light backgrounds, light text for dark backgrounds
    return (luminance > 0.179f) ? text_primary : TEXT_PRIMARY;
}

Color Theme::blend_colors(Color a, Color b, float ratio) const {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    return Color(
        static_cast<uint8_t>(a.r + (b.r - a.r) * ratio),
        static_cast<uint8_t>(a.g + (b.g - a.g) * ratio),
        static_cast<uint8_t>(a.b + (b.b - a.b) * ratio),
        static_cast<uint8_t>(a.a + (b.a - a.a) * ratio)
    );
}

Color Theme::with_theme_opacity(Color color, float opacity) const {
    return Color(
        color.r,
        color.g,
        color.b,
        static_cast<uint8_t>(color.a * opacity)
    );
}

bool Theme::is_system_dark() {
    // Try to detect system theme on Linux
    #ifdef __linux__
    
    // Method 1: Check for gsettings (GNOME)
    if (std::getenv("XDG_CURRENT_DESKTOP") != nullptr) {
        const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
        if (desktop && std::string(desktop).find("GNOME") != std::string::npos) {
            // Try to read gsettings
            FILE* pipe = popen("gsettings get org.gnome.desktop.interface gtk-theme-name", "r");
            if (pipe) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string theme(buffer);
                    pclose(pipe);
                    if (theme.find("dark") != std::string::npos || 
                        theme.find("Dark") != std::string::npos) {
                        return true;
                    }
                }
                pclose(pipe);
            }
        }
    }
    
    // Method 2: Check for KDE
    if (std::getenv("XDG_CURRENT_DESKTOP") != nullptr) {
        const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
        if (desktop && std::string(desktop).find("KDE") != std::string::npos) {
            // Try to read KDE config
            FILE* pipe = popen("lookAndFeelTheme", "r");
            if (pipe) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string theme(buffer);
                    pclose(pipe);
                    if (theme.find("dark") != std::string::npos || 
                        theme.find("Dark") != std::string::npos) {
                        return true;
                    }
                }
                pclose(pipe);
            }
        }
    }
    
    // Method 3: Check for environment variable
    const char* theme_env = std::getenv("GTK_THEME");
    if (theme_env && std::string(theme_env).find("dark") != std::string::npos) {
        return true;
    }
    
    // Method 4: Check for common dark theme files
    // This is a fallback - in practice, you'd want a more reliable method
    
    #endif
    
    // Default to dark theme if we can't detect
    return true;
}

// ============================================================================
// Theme Manager Implementation
// ============================================================================

ThemeManager::ThemeManager() {
    // Initialize with dark theme by default
    current_theme = std::make_unique<Theme>(ThemeType::DARK);
}

ThemeManager::~ThemeManager() {
    // Clean up
}

void ThemeManager::set_theme(std::unique_ptr<Theme> theme) {
    current_theme = std::move(theme);
    notify_theme_change();
}

void ThemeManager::set_theme_type(ThemeType type) {
    switch (type) {
        case ThemeType::DARK:
            current_theme = std::make_unique<Theme>(Theme::create_dark_theme());
            break;
        case ThemeType::LIGHT:
            current_theme = std::make_unique<Theme>(Theme::create_light_theme());
            break;
        case ThemeType::SYSTEM:
            current_theme = std::make_unique<Theme>(Theme::create_system_theme());
            break;
        default:
            current_theme = std::make_unique<Theme>(Theme::create_dark_theme());
            break;
    }
    notify_theme_change();
}

void ThemeManager::set_custom_theme(std::unique_ptr<Theme> theme) {
    theme->type = ThemeType::CUSTOM;
    current_theme = std::move(theme);
    notify_theme_change();
}

void ThemeManager::reload_from_config() {
    current_theme = std::make_unique<Theme>(Theme::load_from_config("theme.conf"));
    notify_theme_change();
}

bool ThemeManager::is_dark_theme() const {
    return current_theme->is_dark();
}

void ThemeManager::add_theme_change_listener(ThemeChangeCallback callback) {
    change_listeners.push_back(callback);
}

void ThemeManager::remove_theme_change_listener(ThemeChangeCallback callback) {
    change_listeners.erase(
        std::remove(change_listeners.begin(), change_listeners.end(), callback),
        change_listeners.end()
    );
}

void ThemeManager::notify_theme_change() {
    for (auto& callback : change_listeners) {
        if (callback) {
            callback(*current_theme);
        }
    }
}

// ============================================================================
// Global Theme Functions
// ============================================================================

const Theme& get_theme() {
    return ThemeManager::get_instance().get_theme();
}

Theme& get_theme_mutable() {
    return ThemeManager::get_instance().get_theme();
}

void set_theme_type(ThemeType type) {
    ThemeManager::get_instance().set_theme_type(type);
}

bool is_dark_theme() {
    return ThemeManager::get_instance().is_dark_theme();
}

// ============================================================================
// Theme Override Implementation
// ============================================================================

void ThemeOverride::apply_to(Theme& theme) const {
    // Apply color overrides
    for (const auto& [key, color] : colors) {
        // In a real implementation, you'd have a mapping from
        // override keys to theme properties
    }
    
    // Apply spacing overrides
    for (const auto& [key, spacing] : spacing) {
        // Similar for spacing
    }
    
    // Apply radius overrides
    for (const auto& [key, radius] : radii) {
        // Similar for radii
    }
    
    // Apply font size overrides
    for (const auto& [key, font_size] : font_sizes) {
        // Similar for font sizes
    }
}

} // namespace lshell::design
