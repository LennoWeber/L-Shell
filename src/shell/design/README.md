# L-Shell Design System

## Overview

The L-Shell Design System provides a **comprehensive, modern, and professional** UI foundation for the L-Shell project. This system was created as part of a complete redesign initiative to transform L-Shell into a cutting-edge desktop environment for Fedora with Hyprland.

## 🎯 Goals

1. **Complete Reset**: Remove all existing UI designs and start fresh
2. **Modern Design**: Create an extremely modern, professional, and elegant interface
3. **Full Functionality**: Preserve all existing functionality while improving UX
4. **Consistency**: Maintain a unified design language across all components
5. **Performance**: Ensure optimal performance on all hardware
6. **Accessibility**: Meet WCAG 2.1 AA standards

## 📁 Structure

```
src/shell/design/
├── colors.h/.cpp      # Color definitions and utilities
├── typography.h      # Font families, sizes, and text styles
├── spacing.h         # Spacing system based on 8px grid
├── animations.h/.cpp # Animation definitions and easing functions
├── theme.h/.cpp      # Complete theme system with dark/light modes
└── README.md         # This file
```

## 🎨 Core Components

### Colors

The color system includes:
- **Primary Colors**: Deep blue-black palette (#1a1a2e, #16213e, #0f3460)
- **Neutral Colors**: Dark backgrounds with subtle contrasts
- **Status Colors**: Success (green), Warning (orange), Error (red), Info (blue)
- **Semantic Colors**: Pre-defined colors for bars, control center, widgets, buttons, etc.
- **Opacity Levels**: 0%, 10%, 20%, 50%, 85%, 90%, 100%
- **Blur Effects**: Gaussian blur levels for glassmorphism
- **Shadows**: Multiple shadow presets for depth
- **Gradients**: Linear gradients for backgrounds and accents

### Typography

- **Font Families**:
  - Primary: `Inter` (with Noto Sans, DejaVu Sans fallbacks)
  - Monospace: `JetBrains Mono` (with Fira Code, DejaVu Sans Mono fallbacks)
  - System: System font stack

- **Font Sizes**: Tiny (10px), XS (11px), SM (12px), Base (14px), LG (16px), XL (18px), XXL (20px), XXXL (24px)

- **Font Weights**: Thin (100), Extra Light (200), Light (300), Regular (400), Medium (500), SemiBold (600), Bold (700), Extra Bold (800), Black (900)

- **Text Styles**: Pre-defined styles for headings, body text, buttons, inputs, etc.

### Spacing

Based on an **8px grid system**:
- **Spacing Scale**: None (0px), XS (4px), SM (8px), MD (16px), LG (24px), XL (32px), XXL (40px), XXXL (48px)
- **Border Radius**: None (0px), SM (4px), MD (8px), LG (12px), XL (16px), Full (9999px)
- **Component-specific Spacing**: Pre-defined spacing for bars, control center, widgets, buttons, inputs, cards, modals, etc.

### Animations

- **Timing Functions**: Linear, Quadratic, Cubic, Quartic, Quintic, Sine, Exponential, Circular, Elastic, Back, Bounce, Spring
- **Durations**: Instant (0ms), Fastest (50ms), Faster (100ms), Fast (150ms), Normal (200ms), Slow (300ms), Slower (400ms), Long (1000ms)
- **Pre-defined Animations**: Fade in/out, Scale in/out, Slide in/out (all directions), Combined animations
- **Component Animations**: Specific animations for buttons, inputs, tooltips, modals, dropdowns, notifications, etc.

### Theme System

- **Theme Types**: Dark, Light, System (follows OS preference), Custom
- **Theme Manager**: Singleton for managing and switching themes
- **Theme Overrides**: Component-specific theme customizations
- **Dynamic Theme Detection**: Automatic detection of system theme preference

## 🚀 Usage

### Basic Usage

```cpp
#include <shell/design/colors.h>
#include <shell/design/typography.h>
#include <shell/design/spacing.h>
#include <shell/design/theme.h>

// Use predefined colors
Color bg_color = lshell::design::NEUTRAL_BACKGROUND;
Color text_color = lshell::design::TEXT_PRIMARY;
Color accent = lshell::design::PRIMARY_SECONDARY;

// Use spacing
int padding = lshell::design::SPACING_MD;  // 16px
int margin = lshell::design::SPACING_LG;   // 24px

// Use typography
FontSize font_size = lshell::design::FONT_SIZE_BASE;  // 14px
FontWeight font_weight = lshell::design::FontWeight::SEMI_BOLD;

// Access current theme
const auto& theme = lshell::design::get_theme();
bool dark_mode = lshell::design::is_dark_theme();

// Set theme
lshell::design::set_theme_type(lshell::design::ThemeType::DARK);
```

### With Theme Manager

```cpp
// Get theme manager instance
auto& theme_manager = lshell::design::ThemeManager::get_instance();

// Get current theme
const auto& theme = theme_manager.get_theme();

// Change theme
theme_manager.set_theme_type(lshell::design::ThemeType::LIGHT);

// Listen for theme changes
theme_manager.add_theme_change_listener([](const auto& new_theme) {
    // Update UI when theme changes
});
```

### Custom Theme

```cpp
auto custom_theme = std::make_unique<lshell::design::Theme>(lshell::design::ThemeType::CUSTOM);

// Customize colors
custom_theme->primary_main = lshell::design::Color::from_hex(0xff0000ff);
custom_theme->text_primary = lshell::design::Color::from_hex(0x00ff00ff);

// Apply custom theme
lshell::design::ThemeManager::get_instance().set_custom_theme(std::move(custom_theme));
```

## 🎨 Design Principles

### Color Theory

- **60-30-10 Rule**: 60% dominant (backgrounds), 30% secondary (surfaces), 10% accent (highlights)
- **Accessibility**: Minimum contrast ratio of 4.5:1 for normal text, 3:1 for large text
- **Semantic Colors**: Use consistent colors for specific purposes (success=green, error=red, etc.)

### Typography

- **Hierarchy**: Use different font sizes and weights to create clear visual hierarchy
- **Readability**: Ensure sufficient line height (1.2x to 1.5x font size)
- **Consistency**: Use the same font family throughout the UI

### Spacing

- **8px Grid**: All spacing should be multiples of 8px for consistency
- **Grouping**: Use spacing to group related elements and separate unrelated ones
- **Whitespace**: Don't be afraid of whitespace - it improves readability

### Animation

- **Purpose**: Every animation should serve a purpose (feedback, attention, transition)
- **Timing**: Keep animations fast (100-300ms) for responsiveness
- **Easing**: Use appropriate easing functions for natural feel
- **Performance**: Prefer opacity and transform animations (GPU-accelerated)

## 📐 Component Specifications

### Bar (Taskbar/Panel)

- **Position**: Bottom (default), Top, Left, or Right
- **Height**: 40px (XS), 48px (SM/Medium), 56px (LG), 64px (XL)
- **Background**: Semi-transparent with blur effect (85% opacity, 8px blur)
- **Border**: Subtle 1px border with 20% opacity
- **Padding**: 16px horizontal, 8px vertical
- **Widget Spacing**: 8px between widgets

### Control Center

- **Width**: 320px (XS), 400px (SM), 500px (LG), 600px (XL)
- **Position**: Right-aligned or centered
- **Background**: Semi-transparent (90% opacity) with blur
- **Border**: Subtle 1px border
- **Animation**: Slide-in from right (300ms) with fade

### Widgets

- **Size**: 24px height, minimum 24px width
- **Spacing**: 8px between widgets in bar
- **Hover**: Scale to 1.05x with background lightening
- **Active**: Subtle accent color indicator

### Buttons

- **Primary**: Filled with accent color, white text
- **Secondary**: Transparent with border, primary text
- **Tertiary**: Transparent, secondary text (icon-only)
- **Sizes**: Small (24px), Medium (32px), Large (40px)
- **Radius**: 8px (standard), 12px (rounded), 9999px (pill)

### Inputs

- **Height**: 24px (SM), 32px (MD), 40px (LG)
- **Background**: Surface color
- **Border**: 1px with 20% opacity, accent color on focus
- **Padding**: 12px horizontal, 8px vertical

## 🔄 Theme Switching

The design system supports dynamic theme switching between dark and light modes:

```cpp
// Switch to dark theme
lshell::design::set_theme_type(lshell::design::ThemeType::DARK);

// Switch to light theme
lshell::design::set_theme_type(lshell::design::ThemeType::LIGHT);

// Follow system preference
lshell::design::set_theme_type(lshell::design::ThemeType::SYSTEM);
```

## 🎯 Future Enhancements

1. **CSS Export**: Generate CSS files from theme definitions
2. **Live Preview**: Visual editor for theme customization
3. **Theme Marketplace**: Download and share custom themes
4. **Dynamic Themes**: Time-based or sensor-based theme switching
5. **High Contrast Mode**: Enhanced accessibility theme
6. **Color Blind Modes**: Themes optimized for different types of color blindness

## 📚 Documentation

- [DESIGN-PLAN-2026.md](../../DESIGN-PLAN-2026.md) - Complete design plan with wireframes
- [colors.h](colors.h) - Color definitions and utilities
- [typography.h](typography.h) - Typography system
- [spacing.h](spacing.h) - Spacing and layout system
- [animations.h](animations.h) - Animation definitions
- [theme.h](theme.h) - Theme system

## 🤝 Contributing

1. Follow the existing design patterns and principles
2. Use the provided color, typography, and spacing constants
3. Maintain consistency across all components
4. Ensure accessibility (contrast, keyboard navigation, etc.)
5. Test with both dark and light themes
6. Document new components and patterns

## 📄 License

This design system is part of the L-Shell project and follows the same license terms.

---

**Created**: October 15, 2026  
**Version**: 1.0  
**Status**: Active Development  
**Author**: L-Shell Design Team
