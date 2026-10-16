# L-Shell Design System - Usage Examples

## Table of Contents
1. [Basic Usage](#basic-usage)
2. [Creating a Bar Widget](#creating-a-bar-widget)
3. [Creating a Control Center Tab](#creating-a-control-center-tab)
4. [Using Animations](#using-animations)
5. [Theme Switching](#theme-switching)
6. [Custom Theming](#custom-theming)
7. [Best Practices](#best-practices)

---

## Basic Usage

### Including Headers

```cpp
// Include only what you need
#include <shell/design/colors.h>
#include <shell/design/typography.h>
#include <shell/design/spacing.h>
#include <shell/design/animations.h>

// Or include the theme for full access
#include <shell/design/theme.h>
```

### Using Colors

```cpp
using namespace lshell::design;

// Predefined colors
Color background = NEUTRAL_BACKGROUND;  // #0a0a14
Color surface = NEUTRAL_SURFACE;        // #161b26
Color text = TEXT_PRIMARY;              // #e8e8e8
Color accent = PRIMARY_SECONDARY;       // #0f3460

// Create custom colors
Color custom_red = Color(255, 0, 0, 255);
Color semi_transparent = Color(255, 255, 255, 128);  // 50% opacity

// From hex
Color from_hex = Color::from_hex(0x1a1a2eff);  // #1a1a2e with full opacity
Color from_hex_alpha = Color::from_hex(0x1a1a2e80);  // #1a1a2e with 50% opacity

// Color operations
Color lighter = background.lighten(0.2f);      // 20% lighter
Color darker = background.darken(0.2f);        // 20% darker
Color blended = color1.blend(color2, 0.5f);    // 50/50 mix
Color with_alpha = text.with_alpha(128);        // 50% opacity

// Using semantic colors
Color bar_bg = BAR_BACKGROUND;
Color widget_hover = WIDGET_HOVER;
Color button_primary = BUTTON_PRIMARY_BG;
```

### Using Typography

```cpp
using namespace lshell::design;

// Font families
FontFamily primary_font = FONT_FAMILY_PRIMARY;  // "Inter", fallbacks
FontFamily mono_font = FONT_FAMILY_MONO;        // "JetBrains Mono", fallbacks

// Font sizes
FontSize small_text = FONT_SIZE_SM;            // 12px
FontSize body_text = FONT_SIZE_BASE;           // 14px
FontSize heading = FONT_SIZE_LG;                // 16px

// Font weights
FontWeight regular = FontWeight::REGULAR;      // 400
FontWeight bold = FontWeight::BOLD;            // 700
FontWeight semi_bold = FontWeight::SEMI_BOLD;  // 600

// Predefined text styles
TextStyle body_style = TEXT_STYLE_BODY;
TextStyle heading1 = TEXT_STYLE_H1;
TextStyle button_text = TEXT_STYLE_BUTTON_PRIMARY;

// Custom text style
TextStyle custom_style = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    TEXT_SECONDARY,
    false,  // italic
    true,   // underline
    false   // strike_through
};

// Convert to CSS
std::string css = body_style.to_css();
// Result: "font-family: Inter, Noto Sans, DejaVu Sans, sans-serif; font-weight: 400; font-size: 14px/20px; color: rgba(232, 232, 232, 1);"
```

### Using Spacing

```cpp
using namespace lshell::design;

// Spacing values
Spacing none = SPACING_NONE;      // 0px
Spacing small = SPACING_SM;        // 8px
Spacing medium = SPACING_MD;       // 16px
Spacing large = SPACING_LG;        // 24px

// Use as int
int padding = SPACING_MD;           // 16
int margin = SPACING_LG;            // 24

// Arithmetic operations
Spacing total_padding = SPACING_MD + SPACING_SM;  // 16 + 8 = 24px
Spacing half = SPACING_XL / 2;                    // 32 / 2 = 16px
Spacing double = SPACING_SM * 2;                  // 8 * 2 = 16px

// Component-specific spacing
int bar_height = BAR_HEIGHT_MD;    // 48px
int widget_height = WIDGET_HEIGHT; // 24px
int button_padding_x = BUTTON_PADDING_X_MD; // 16px

// Border radius
BorderRadius none = RADIUS_NONE;       // 0px
BorderRadius small = RADIUS_SM;        // 4px
BorderRadius medium = RADIUS_MD;       // 8px
BorderRadius large = RADIUS_LG;        // 12px
BorderRadius full = RADIUS_FULL;       // 9999px (circle)

// Custom border radius (different corners)
BorderRadius custom = BorderRadius(
    SPACING_SM,   // top-left
    SPACING_MD,   // top-right
    SPACING_LG,   // bottom-right
    SPACING_XS    // bottom-left
);

// Convert to CSS
std::string css_radius = RADIUS_MD.to_css();  // "8px"
```

---

## Creating a Bar Widget

### Simple Icon Widget

```cpp
#include <shell/design/theme.h>
#include <shell/bar/widget.h>

class MyIconWidget : public lshell::bar::Widget {
public:
    MyIconWidget() {
        // Get theme
        const auto& theme = lshell::design::get_theme();
        
        // Set dimensions using design system
        set_min_size({theme.widget_height, theme.widget_height});
        set_padding({theme.widget_padding_x, theme.widget_padding_y});
        
        // Set colors
        set_background_color(theme.widget_background);
        set_hover_color(theme.widget_hover);
        set_active_color(theme.widget_active);
    }
    
    void paint(cairo_t* cr) override {
        const auto& theme = lshell::design::get_theme();
        
        // Draw icon
        cairo_set_source_rgba(cr, 
            theme.text_primary.r / 255.0,
            theme.text_primary.g / 255.0,
            theme.text_primary.b / 255.0,
            theme.text_primary.a / 255.0);
        
        // Draw icon at center
        cairo_move_to(cr, 8, 8);
        // ... draw icon ...
    }
    
    bool on_click() override {
        // Handle click
        return true;
    }
};
```

### Text Widget with Animation

```cpp
#include <shell/design/theme.h>
#include <shell/design/animations.h>
#include <shell/bar/widget.h>

class MyTextWidget : public lshell::bar::Widget {
    std::string text;
    float hover_scale = 1.0f;
    bool is_hovered = false;
    lshell::design::Animation scale_anim;

public:
    MyTextWidget(const std::string& text) : text(text) {
        const auto& theme = lshell::design::get_theme();
        
        // Set dimensions
        set_min_size({100, theme.widget_height});
        set_padding({theme.widget_padding_x, theme.widget_padding_y});
        
        // Set text style
        set_font(theme.font_family_primary.primary);
        set_font_size(theme.font_size_sm.size);
        set_text_color(theme.text_primary);
        
        // Setup hover animation
        scale_anim = lshell::design::ANIM_SCALE_PULSE;
    }
    
    void set_hovered(bool hovered) override {
        is_hovered = hovered;
        
        if (hovered) {
            // Animate scale up
            // In a real implementation, you'd start the animation here
            hover_scale = 1.05f;
        } else {
            // Animate scale down
            hover_scale = 1.0f;
        }
        
        queue_draw();
    }
    
    void paint(cairo_t* cr) override {
        const auto& theme = lshell::design::get_theme();
        
        // Apply scale
        cairo_scale(cr, hover_scale, hover_scale);
        
        // Draw background (hover state)
        cairo_set_source_rgba(cr,
            theme.widget_hover.r / 255.0,
            theme.widget_hover.g / 255.0,
            theme.widget_hover.b / 255.0,
            theme.widget_hover.a / 255.0 * (is_hovered ? 1.0 : 0.0));
        
        // Draw text
        cairo_set_source_rgba(cr,
            theme.text_primary.r / 255.0,
            theme.text_primary.g / 255.0,
            theme.text_primary.b / 255.0,
            theme.text_primary.a / 255.0);
        
        cairo_move_to(cr, 0, theme.widget_height / 2);
        // ... draw text ...
    }
};
```

---

## Creating a Control Center Tab

### Basic Tab Structure

```cpp
#include <shell/design/theme.h>
#include <shell/control_center/tab.h>

class MySettingsTab : public lshell::control_center::Tab {
public:
    MySettingsTab() {
        const auto& theme = lshell::design::get_theme();
        
        // Set tab properties
        set_name("My Settings");
        set_icon("settings");
        
        // Set layout
        set_padding({theme.cc_padding, theme.cc_padding});
        set_spacing(theme.cc_section_spacing);
        
        // Set colors
        set_background_color(theme.cc_background);
    }
    
    void build_ui() override {
        const auto& theme = lshell::design::get_theme();
        
        // Create title
        auto* title = new lshell::ui::Label("My Settings");
        title->set_font(theme.font_family_primary.primary);
        title->set_font_size(theme.font_size_xl.size);
        title->set_font_weight(lshell::design::FontWeight::BOLD);
        title->set_text_color(theme.text_primary);
        add_child(title);
        
        // Create section header
        auto* section_title = new lshell::ui::Label("General");
        section_title->set_font(theme.font_family_primary.primary);
        section_title->set_font_size(theme.font_size_lg.size);
        section_title->set_font_weight(lshell::design::FontWeight::SEMI_BOLD);
        section_title->set_text_color(theme.text_secondary);
        section_title->set_margin_top(theme.cc_section_spacing);
        add_child(section_title);
        
        // Create setting items
        // ...
    }
};
```

### Settings Item

```cpp
#include <shell/design/theme.h>
#include <shell/ui/button.h>
#include <shell/ui/label.h>

class SettingItem : public lshell::ui::Widget {
    std::string name;
    std::string value;
    std::function<void()> on_activate;
    
    lshell::ui::Label* name_label;
    lshell::ui::Label* value_label;

public:
    SettingItem(const std::string& name, const std::string& value, 
                std::function<void()> on_activate)
        : name(name), value(value), on_activate(on_activate) {
        
        const auto& theme = lshell::design::get_theme();
        
        // Setup layout
        set_layout(lshell::ui::Layout::HORIZONTAL);
        set_padding({theme.cc_padding, theme.cc_item_spacing / 2});
        set_spacing(theme.cc_item_spacing);
        
        // Set colors
        set_background_color(theme.cc_background);
        set_hover_color(theme.cc_hover);
        set_active_color(theme.cc_active);
        
        // Create name label
        name_label = new lshell::ui::Label(name);
        name_label->set_font(theme.font_family_primary.primary);
        name_label->set_font_size(theme.font_size_base.size);
        name_label->set_text_color(theme.text_primary);
        add_child(name_label);
        
        // Create value label (right-aligned)
        value_label = new lshell::ui::Label(value);
        value_label->set_font(theme.font_family_primary.primary);
        value_label->set_font_size(theme.font_size_base.size);
        value_label->set_text_color(theme.text_secondary);
        value_label->set_halign(lshell::ui::Align::END);
        add_child(value_label);
        
        // Set click handler
        set_click_handler([this]() {
            if (on_activate) on_activate();
        });
    }
    
    void set_value(const std::string& new_value) {
        value = new_value;
        value_label->set_text(new_value);
    }
};
```

---

## Using Animations

### Simple Fade Animation

```cpp
#include <shell/design/animations.h>

// Create a widget with fade animation
class FadeWidget : public lshell::ui::Widget {
    float opacity = 1.0f;
    lshell::design::Animation fade_anim;
    bool animating = false;

public:
    FadeWidget() {
        // Setup fade animation
        fade_anim = lshell::design::ANIM_FADE_IN;
    }
    
    void start_fade_in() {
        opacity = 0.0f;
        animating = true;
        // Start animation timer
        // In a real implementation, you'd use a timer or animation framework
    }
    
    void update_animation(float delta_time) {
        if (!animating) return;
        
        // Simple linear animation
        opacity = std::min(1.0f, opacity + delta_time * 3.0f); // 3.0 = speed
        
        if (opacity >= 1.0f) {
            animating = false;
            opacity = 1.0f;
        }
        
        queue_draw();
    }
    
    void paint(cairo_t* cr) override {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, opacity);
        // ... draw widget ...
    }
};
```

### Using Easing Functions

```cpp
#include <shell/design/animations.h>

float animate_value(float start, float end, float progress, 
                   lshell::design::EasingFunction easing) {
    // Get easing value
    float eased = lshell::design::easing_function_value(easing, progress);
    
    // Interpolate
    return start + (end - start) * eased;
}

// Usage
float start = 0.0f;
float end = 1.0f;
float progress = 0.5f;  // 50%

// Linear easing (constant speed)
float value1 = animate_value(start, end, progress, 
                           lshell::design::EasingFunction::LINEAR);

// Ease-out quadratic (fast start, slow end)
float value2 = animate_value(start, end, progress, 
                           lshell::design::EasingFunction::EASE_OUT_QUAD);

// Ease-in-out cubic (smooth start and end)
float value3 = animate_value(start, end, progress, 
                           lshell::design::EasingFunction::EASE_IN_OUT_CUBIC);

// Spring (bouncy)
float value4 = animate_value(start, end, progress, 
                           lshell::design::EasingFunction::SPRING);
```

### Button Hover Animation

```cpp
#include <shell/design/animations.h>
#include <shell/ui/button.h>

class AnimatedButton : public lshell::ui::Button {
    float hover_progress = 0.0f;
    lshell::design::Animation anim;
    bool is_hovered = false;

public:
    AnimatedButton(const std::string& text) 
        : lshell::ui::Button(text) {
        
        // Setup animation
        anim = lshell::design::ANIM_BUTTON_HOVER;
        
        // Set initial state
        set_scale(1.0f);
    }
    
    void set_hovered(bool hovered) override {
        is_hovered = hovered;
        
        if (hovered) {
            // Start animation towards 1.05x scale
            // In a real implementation, start a timer
        }
        
        lshell::ui::Button::set_hovered(hovered);
    }
    
    void update(float delta_time) {
        float target = is_hovered ? 1.05f : 1.0f;
        
        // Simple approach: move towards target
        const float speed = 10.0f;  // Adjust for desired speed
        hover_progress += (target - get_scale()) * speed * delta_time;
        
        // Clamp to avoid overshooting
        hover_progress = std::clamp(hover_progress, 0.0f, 1.0f);
        
        // Apply easing
        float eased = lshell::design::easing_function_value(
            lshell::design::EasingFunction::EASE_OUT_QUAD, hover_progress);
        
        set_scale(1.0f + eased * 0.05f);
    }
};
```

---

## Theme Switching

### Basic Theme Switching

```cpp
#include <shell/design/theme.h>

// Switch to dark theme
lshell::design::set_theme_type(lshell::design::ThemeType::DARK);

// Switch to light theme
lshell::design::set_theme_type(lshell::design::ThemeType::LIGHT);

// Follow system preference
lshell::design::set_theme_type(lshell::design::ThemeType::SYSTEM);
```

### Advanced Theme Management

```cpp
#include <shell/design/theme.h>

// Get theme manager
auto& theme_manager = lshell::design::ThemeManager::get_instance();

// Get current theme
const auto& current_theme = theme_manager.get_theme();

// Check if dark theme
if (lshell::design::is_dark_theme()) {
    // Use dark theme colors
} else {
    // Use light theme colors
}

// Listen for theme changes
auto listener_id = theme_manager.add_theme_change_listener(
    [](const lshell::design::Theme& new_theme) {
        // Update all UI elements with new theme
        update_ui_colors(new_theme);
        update_ui_layout(new_theme);
    }
);

// To remove listener later
// theme_manager.remove_theme_change_listener(listener);
```

### Widget that Adapts to Theme

```cpp
#include <shell/design/theme.h>
#include <shell/ui/widget.h>

class ThemeAdaptiveWidget : public lshell::ui::Widget {
public:
    ThemeAdaptiveWidget() {
        // Listen for theme changes
        lshell::design::ThemeManager::get_instance().add_theme_change_listener(
            [this](const auto& theme) {
                on_theme_change(theme);
            }
        );
        
        // Initialize with current theme
        on_theme_change(lshell::design::get_theme());
    }
    
    ~ThemeAdaptiveWidget() {
        // Remove listener (in a real implementation, you'd need to track the callback)
    }
    
    void on_theme_change(const lshell::design::Theme& theme) {
        // Update colors based on theme
        set_background_color(theme.neutral_surface);
        set_text_color(theme.text_primary);
        set_border_color(theme.neutral_overlay);
        
        // Update font
        set_font(theme.font_family_primary.primary);
        set_font_size(theme.font_size_base.size);
        
        queue_draw();
    }
    
    void paint(cairo_t* cr) override {
        const auto& theme = lshell::design::get_theme();
        
        // Use current theme colors
        cairo_set_source_rgba(cr,
            theme.text_primary.r / 255.0,
            theme.text_primary.g / 255.0,
            theme.text_primary.b / 255.0,
            theme.text_primary.a / 255.0);
        
        // ... draw ...
    }
};
```

---

## Custom Theming

### Creating a Custom Theme

```cpp
#include <shell/design/theme.h>

// Create a custom theme
auto custom_theme = std::make_unique<lshell::design::Theme>(
    lshell::design::ThemeType::CUSTOM);

// Customize colors
custom_theme->name = "My Custom Theme";
custom_theme->description = "A custom theme with purple accents";

// Primary colors (purple palette)
custom_theme->primary_main = lshell::design::Color::from_hex(0x1a1a2e);
custom_theme->primary_accent = lshell::design::Color::from_hex(0x2d1b69);
custom_theme->primary_secondary = lshell::design::Color::from_hex(0x8b5cf6);  // Purple

// Background colors
custom_theme->neutral_background = lshell::design::Color::from_hex(0x0f0f1a);
custom_theme->neutral_surface = lshell::design::Color::from_hex(0x1a1a2e);

// Text colors
custom_theme->text_primary = lshell::design::Color::from_hex(0xe8e8e8);
custom_theme->text_secondary = lshell::design::Color::from_hex(0xa0a0a0);

// Status colors
custom_theme->status_success = lshell::design::Color::from_hex(0x10b981);  // Emerald
custom_theme->status_warning = lshell::design::Color::from_hex(0xf59e0b);  // Amber
custom_theme->status_error = lshell::design::Color::from_hex(0xef4444);   // Red
custom_theme->status_info = lshell::design::Color::from_hex(0x3b82f6);   // Blue

// Apply custom theme
lshell::design::ThemeManager::get_instance().set_custom_theme(
    std::move(custom_theme));
```

### Theme Overrides for Specific Components

```cpp
#include <shell/design/theme.h>

// Create a theme override for a specific component
lshell::design::ThemeOverride override;

// Override colors for a custom widget
override.colors["my_widget_bg"] = lshell::design::Color::from_hex(0xff0000ff);
override.colors["my_widget_fg"] = lshell::design::Color::from_hex(0xffffff);

// Override spacing
override.spacing["my_widget_padding"] = lshell::design::Spacing(24);

// Apply override to theme
// Note: This would need integration with your theme system
```

---

## Best Practices

### 1. Always Use Theme Colors

**Bad:**
```cpp
// Hardcoded colors
set_background_color(Color(255, 255, 255, 255));
```

**Good:**
```cpp
const auto& theme = lshell::design::get_theme();
set_background_color(theme.neutral_surface);
```

### 2. Use Spacing Constants

**Bad:**
```cpp
// Magic numbers
set_padding(16);  // What does 16 mean?
set_margin(24);   // Why 24?
```

**Good:**
```cpp
const auto& theme = lshell::design::get_theme();
set_padding(theme.spacing_md);     // 16px - medium spacing
set_margin(theme.spacing_lg);      // 24px - large spacing
```

### 3. Respect Theme Changes

**Bad:**
```cpp
// Cache colors in constructor
Color my_color = get_theme().text_primary;
// ... later ...
// Color won't update when theme changes
```

**Good:**
```cpp
// Get colors on demand or listen for theme changes
const auto& theme = get_theme();
Color my_color = theme.text_primary;

// OR listen for theme changes
add_theme_change_listener([this](const auto& theme) {
    this->my_color = theme.text_primary;
    queue_draw();
});
```

### 4. Use Appropriate Typography

**Bad:**
```cpp
// No hierarchy
label1->set_font_size(12);
label2->set_font_size(12);
```

**Good:**
```cpp
const auto& theme = get_theme();

// Clear hierarchy
title_label->set_font(theme.font_family_primary.primary);
title_label->set_font_size(theme.font_size_xl.size);
title_label->set_font_weight(lshell::design::FontWeight::BOLD);

body_label->set_font(theme.font_family_primary.primary);
body_label->set_font_size(theme.font_size_base.size);
body_label->set_font_weight(lshell::design::FontWeight::REGULAR);
```

### 5. Use Animations Wisely

**Bad:**
```cpp
// Over-animating everything
set_animation(ANIM_SPRING);  // Bouncy button?
```

**Good:**
```cpp
// Use appropriate animations for each purpose
// Hover - fast and subtle
set_hover_animation(lshell::design::component_animations::BUTTON_HOVER);

// Modal open - smooth and noticeable
set_open_animation(lshell::design::component_animations::MODAL_OPEN);

// Notification - attention-grabbing
set_show_animation(lshell::design::component_animations::NOTIFICATION_SHOW);
```

### 6. Ensure Accessibility

**Bad:**
```cpp
// Low contrast
set_text_color(Color(100, 100, 100));
set_background_color(Color(50, 50, 50));
```

**Good:**
```cpp
// Use theme colors which have good contrast
const auto& theme = get_theme();
set_text_color(theme.text_primary);
set_background_color(theme.neutral_surface);

// OR check contrast programmatically
Color bg = ...;
Color fg = theme.get_contrast_text(bg);  // Automatically chooses high contrast
```

### 7. Support Both Dark and Light Themes

**Bad:**
```cpp
// Only works with dark theme
set_background_color(Color(0, 0, 0));
set_text_color(Color(255, 255, 255));
```

**Good:**
```cpp
// Works with any theme
const auto& theme = get_theme();
set_background_color(theme.neutral_surface);
set_text_color(theme.text_primary);
```

### 8. Use Semantic Colors

**Bad:**
```cpp
// What does this color mean?
set_color(Color(255, 0, 0));
```

**Good:**
```cpp
// Clear meaning
set_color(theme.status_error);  // Red for errors
set_color(theme.status_success);  // Green for success
```

---

## Common Patterns

### Creating a Card

```cpp
#include <shell/design/theme.h>
#include <shell/ui/widget.h>

class Card : public lshell::ui::Widget {
public:
    Card() {
        const auto& theme = lshell::design::get_theme();
        
        // Set card properties
        set_background_color(theme.card_background);
        set_border_color(theme.card_border);
        set_border_width(theme.card_border_width);
        set_border_radius(theme.radius_card);
        set_padding({theme.card_padding, theme.card_padding});
        
        // Add hover effect
        set_hover_color(theme.card_hover);
    }
    
    void add_content(lshell::ui::Widget* content) {
        add_child(content);
    }
};
```

### Creating a Form

```cpp
#include <shell/design/theme.h>
#include <shell/ui/input.h>
#include <shell/ui/button.h>

class Form : public lshell::ui::Widget {
public:
    Form() {
        const auto& theme = lshell::design::get_theme();
        
        set_layout(lshell::ui::Layout::VERTICAL);
        set_spacing(theme.spacing_lg);
        set_padding({theme.spacing_lg, theme.spacing_lg});
    }
    
    lshell::ui::Input* add_input(const std::string& label, const std::string& placeholder) {
        const auto& theme = lshell::design::get_theme();
        
        // Create label
        auto* label_widget = new lshell::ui::Label(label);
        label_widget->set_font(theme.font_family_primary.primary);
        label_widget->set_font_size(theme.font_size_sm.size);
        label_widget->set_font_weight(lshell::design::FontWeight::MEDIUM);
        label_widget->set_text_color(theme.text_secondary);
        add_child(label_widget);
        
        // Create input
        auto* input = new lshell::ui::Input();
        input->set_placeholder(placeholder);
        input->set_background_color(theme.input_background);
        input->set_border_color(theme.input_border);
        input->set_text_color(theme.input_text);
        input->set_font(theme.font_family_primary.primary);
        input->set_font_size(theme.font_size_base.size);
        add_child(input);
        
        return input;
    }
    
    lshell::ui::Button* add_button(const std::string& text, 
                                    lshell::ui::Button::Type type) {
        const auto& theme = lshell::design::get_theme();
        
        auto* button = new lshell::ui::Button(text);
        
        switch (type) {
            case lshell::ui::Button::PRIMARY:
                button->set_background_color(theme.button_primary_bg);
                button->set_text_color(theme.button_primary_text);
                break;
            case lshell::ui::Button::SECONDARY:
                button->set_background_color(theme.button_secondary_bg);
                button->set_border_color(theme.button_secondary_border);
                button->set_text_color(theme.button_secondary_text);
                break;
            case lshell::ui::Button::TERTIARY:
                button->set_background_color(theme.button_tertiary_bg);
                button->set_text_color(theme.button_tertiary_text);
                break;
        }
        
        button->set_font(theme.font_family_primary.primary);
        button->set_font_size(theme.font_size_base.size);
        button->set_font_weight(lshell::design::FontWeight::SEMI_BOLD);
        button->set_padding({theme.button_padding_x_md, theme.button_padding_y});
        button->set_border_radius(theme.radius_button);
        
        add_child(button);
        return button;
    }
};
```

---

## Troubleshooting

### Colors Don't Update on Theme Change

**Problem:** Custom widget doesn't update colors when theme changes.

**Solution:** Listen for theme changes and update colors:

```cpp
// In constructor
lshell::design::ThemeManager::get_instance().add_theme_change_listener(
    [this](const auto& theme) {
        update_theme_colors(theme);
        queue_draw();
    });

void CustomWidget::update_theme_colors(const lshell::design::Theme& theme) {
    set_background_color(theme.neutral_surface);
    set_text_color(theme.text_primary);
    // Update all child colors
}
```

### Animation is Too Fast/Slow

**Problem:** Animation doesn't feel right.

**Solution:** Adjust duration and easing:

```cpp
// Try different durations
Animation fast_anim = {DURATION_FAST, EasingFunction::EASE_OUT_QUAD};
Animation normal_anim = {DURATION_NORMAL, EasingFunction::EASE_OUT_QUAD};
Animation slow_anim = {DURATION_SLOW, EasingFunction::EASE_OUT_CUBIC};

// Try different easing functions
Animation ease_in = {DURATION_NORMAL, EasingFunction::EASE_IN_QUAD};
Animation ease_out = {DURATION_NORMAL, EasingFunction::EASE_OUT_QUAD};
Animation ease_in_out = {DURATION_NORMAL, EasingFunction::EASE_IN_OUT_QUAD};
```

### Colors Don't Look Good Together

**Problem:** Color combination has poor contrast or looks unprofessional.

**Solution:** Use the theme's contrast text function or predefined combinations:

```cpp
const auto& theme = get_theme();

// Get text color that contrasts with background
Color bg = ...;
Color fg = theme.get_contrast_text(bg);

// Use semantic colors
Color success_bg = theme.status_success;
Color success_fg = theme.get_contrast_text(success_bg);
```

---

## Additional Resources

- [DESIGN-PLAN-2026.md](../../DESIGN-PLAN-2026.md) - Complete design specification
- [colors.h](colors.h) - Color definitions and utilities
- [typography.h](typography.h) - Typography system
- [spacing.h](spacing.h) - Spacing and layout
- [animations.h](animations.h) - Animation definitions
- [theme.h](theme.h) - Theme system

---

**Note:** These examples assume a certain widget framework structure. You may need to adapt them to your specific framework and requirements.
