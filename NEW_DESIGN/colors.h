// L-Shell Design System - Colors
// Modern, Professional, Dark Theme
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include <cstdint>

namespace lshell::design {

// ============================================================================
// Color Representation
// ============================================================================

/**
 * RGBA Color with 8-bit channels (0-255)
 */
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    // Constructors
    constexpr Color() : r(0), g(0), b(0), a(255) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    // Hex constructor (0xRRGGBBAA or 0xRRGGBB)
    static Color from_hex(uint32_t hex, uint8_t alpha = 255);

    // Convert to hex string
    std::string to_hex() const;

    // Convert to CSS rgba string
    std::string to_css() const;

    // Blend with another color
    Color blend(const Color& other, float ratio) const;

    // Apply opacity
    Color with_alpha(uint8_t alpha) const;

    // Lighten/darken
    Color lighten(float factor) const;
    Color darken(float factor) const;

    // Check if equal
    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const;
};

// ============================================================================
// Primary Colors
// ============================================================================

static constexpr Color PRIMARY_MAIN =      Color(0x1a, 0x1a, 0x2e, 0xff);  // #1a1a2e - Deep blue-black
static constexpr Color PRIMARY_ACCENT =   Color(0x16, 0x21, 0x3e, 0xff);  // #16213e - Deep blue
static constexpr Color PRIMARY_SECONDARY = Color(0x0f, 0x34, 0x60, 0xff);  // #0f3460 - Royal blue

// ============================================================================
// Neutral Colors
// ============================================================================

static constexpr Color NEUTRAL_BACKGROUND = Color(0x0a, 0x0a, 0x14, 0xff);  // #0a0a14 - Almost black
static constexpr Color NEUTRAL_SURFACE =   Color(0x16, 0x1b, 0x26, 0xff);  // #161b26 - Dark gray
static constexpr Color NEUTRAL_OVERLAY =   Color(0x00, 0x00, 0x00, 0xcc);  // 80% black

// ============================================================================
// Text Colors
// ============================================================================

static constexpr Color TEXT_PRIMARY =     Color(0xe8, 0xe8, 0xe8, 0xff);  // #e8e8e8 - Light gray
static constexpr Color TEXT_SECONDARY =   Color(0xa0, 0xa0, 0xa0, 0xff);  // #a0a0a0 - Gray
static constexpr Color TEXT_DISABLED =    Color(0x6c, 0x6c, 0x6c, 0xff);  // #6c6c6c - Dark gray
static constexpr Color TEXT_INVERSE =     Color(0x16, 0x1b, 0x26, 0xff);  // For light backgrounds

// ============================================================================
// Status Colors
// ============================================================================

static constexpr Color STATUS_SUCCESS =   Color(0x4c, 0xaf, 0x50, 0xff);  // #4caf50 - Green
static constexpr Color STATUS_WARNING =   Color(0xff, 0x98, 0x00, 0xff);  // #ff9800 - Orange
static constexpr Color STATUS_ERROR =     Color(0xf4, 0x43, 0x36, 0xff);  // #f44336 - Red
static constexpr Color STATUS_INFO =      Color(0x21, 0x96, 0xf3, 0xff);  // #2196f3 - Blue

// ============================================================================
// Semantic Colors
// ============================================================================

// Bar colors
static constexpr Color BAR_BACKGROUND =   Color(0x16, 0x1b, 0x26, 0xd9);  // 85% opacity
static constexpr Color BAR_BORDER =       Color(0xe8, 0xe8, 0xe8, 0x33);  // 20% opacity
static constexpr Color BAR_HOVER =        Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% opacity
static constexpr Color BAR_ACTIVE =       PRIMARY_SECONDARY;

// Control Center colors
static constexpr Color CC_BACKGROUND =    Color(0x16, 0x1b, 0x26, 0xe6);  // 90% opacity
static constexpr Color CC_BORDER =        Color(0xe8, 0xe8, 0xe8, 0x33);  // 20% opacity
static constexpr Color CC_DIVIDER =       Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% opacity

// Widget colors
static constexpr Color WIDGET_BACKGROUND = Color(0x16, 0x1b, 0x26, 0x00);  // Transparent
static constexpr Color WIDGET_HOVER =     Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% opacity
static constexpr Color WIDGET_ACTIVE =    Color(0x0f, 0x34, 0x60, 0x33);  // 20% accent
static constexpr Color WIDGET_BORDER =    Color(0xe8, 0xe8, 0xe8, 0x00);  // Transparent

// Button colors
static constexpr Color BUTTON_PRIMARY_BG =        PRIMARY_SECONDARY;
static constexpr Color BUTTON_PRIMARY_TEXT =      TEXT_INVERSE;
static constexpr Color BUTTON_PRIMARY_HOVER =     Color(0x1a, 0x40, 0x72, 0xff);  // Lightened accent
static constexpr Color BUTTON_PRIMARY_ACTIVE =   Color(0x0a, 0x2a, 0x48, 0xff);  // Darkened accent

static constexpr Color BUTTON_SECONDARY_BG =      Color(0x00, 0x00, 0x00, 0x00);  // Transparent
static constexpr Color BUTTON_SECONDARY_BORDER = Color(0xe8, 0xe8, 0xe8, 0x80);  // 50% opacity
static constexpr Color BUTTON_SECONDARY_TEXT =    TEXT_PRIMARY;
static constexpr Color BUTTON_SECONDARY_HOVER =  Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% bg
static constexpr Color BUTTON_SECONDARY_ACTIVE = Color(0xe8, 0xe8, 0xe8, 0x33);  // 20% bg

static constexpr Color BUTTON_TERTIARY_BG =       Color(0x00, 0x00, 0x00, 0x00);  // Transparent
static constexpr Color BUTTON_TERTIARY_TEXT =     TEXT_SECONDARY;
static constexpr Color BUTTON_TERTIARY_HOVER =   Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% bg
static constexpr Color BUTTON_TERTIARY_ACTIVE =  Color(0xe8, 0xe8, 0xe8, 0x33);  // 20% bg

// Input colors
static constexpr Color INPUT_BACKGROUND =        Color(0x16, 0x1b, 0x26, 0xff);
static constexpr Color INPUT_BORDER =             Color(0xe8, 0xe8, 0xe8, 0x33);  // 20% opacity
static constexpr Color INPUT_TEXT =               TEXT_PRIMARY;
static constexpr Color INPUT_PLACEHOLDER =        Color(0xa0, 0xa0, 0xa0, 0x80);  // 50% secondary
static constexpr Color INPUT_FOCUS_BORDER =      PRIMARY_SECONDARY;

// Card colors
static constexpr Color CARD_BACKGROUND =         Color(0x16, 0x1b, 0x26, 0xff);
static constexpr Color CARD_BORDER =             Color(0xe8, 0xe8, 0xe8, 0x1a);  // 10% opacity
static constexpr Color CARD_HOVER =              Color(0xe8, 0xe8, 0xe8, 0x0f);  // 5% bg

// ============================================================================
// Gradients
// ============================================================================

/**
 * Linear gradient with two colors
 */
struct Gradient {
    Color start;
    Color end;
    float angle; // in degrees (0 = left to right, 90 = bottom to top)

    Gradient(Color start, Color end, float angle = 0)
        : start(start), end(end), angle(angle) {}
};

static constexpr Gradient GRADIENT_PRIMARY = {
    Color(0x0f, 0x34, 0x60, 0xff),
    Color(0x16, 0x21, 0x3e, 0xff),
    90.0f // Bottom to top
};

static constexpr Gradient GRADIENT_SURFACE = {
    Color(0x1a, 0x1a, 0x2e, 0xff),
    Color(0x16, 0x1b, 0x26, 0xff),
    90.0f // Bottom to top
};

// ============================================================================
// Opacity Levels
// ============================================================================

static constexpr float OPACITY_FULL =     1.00f;  // 100%
static constexpr float OPACITY_HIGH =     0.90f;  // 90%
static constexpr float OPACITY_MEDIUM =   0.85f;  // 85%
static constexpr float OPACITY_LOW =      0.50f;  // 50%
static constexpr float OPACITY_SUBTLE =   0.20f;  // 20%
static constexpr float OPACITY_MINIMAL =  0.10f;  // 10%
static constexpr float OPACITY_NONE =     0.00f;  // 0%

// ============================================================================
// Blur Levels (Gaussian blur radius in pixels)
// ============================================================================

static constexpr int BLUR_NONE =      0;
static constexpr int BLUR_SUBTLE =   4;
static constexpr int BLUR_SMALL =    8;
static constexpr int BLUR_MEDIUM =   12;
static constexpr int BLUR_LARGE =    16;
static constexpr int BLUR_XL =       24;

// ============================================================================
// Shadow Levels
// ============================================================================

/**
 * Shadow with color, offset, blur, and spread
 */
struct Shadow {
    Color color;
    int offset_x;
    int offset_y;
    int blur_radius;
    int spread_radius;

    Shadow(Color color, int offset_x, int offset_y, int blur_radius, int spread_radius = 0)
        : color(color), offset_x(offset_x), offset_y(offset_y),
          blur_radius(blur_radius), spread_radius(spread_radius) {}

    std::string to_css() const;
};

static constexpr Shadow SHADOW_NONE = {
    Color(0x00, 0x00, 0x00, 0x00),
    0, 0, 0, 0
};

static constexpr Shadow SHADOW_SMALL = {
    Color(0x00, 0x00, 0x00, 0x1a),  // 10% opacity
    0, 2, 4, 0
};

static constexpr Shadow SHADOW_MEDIUM = {
    Color(0x00, 0x00, 0x00, 0x26),  // 15% opacity
    0, 4, 8, 0
};

static constexpr Shadow SHADOW_LARGE = {
    Color(0x00, 0x00, 0x00, 0x33),  // 20% opacity
    0, 8, 16, 0
};

static constexpr Shadow SHADOW_XL = {
    Color(0x00, 0x00, 0x00, 0x40),  // 25% opacity
    0, 16, 32, 0
};

// ============================================================================
// Utility Functions
// ============================================================================

namespace utils {

/**
 * Get color with adjusted opacity
 */
inline Color with_opacity(Color color, float opacity) {
    return Color(
        color.r,
        color.g,
        color.b,
        static_cast<uint8_t>(color.a * opacity)
    );
}

/**
 * Get color with added opacity (multiplicative)
 */
inline Color multiply_opacity(Color color, float factor) {
    return Color(
        color.r,
        color.g,
        color.b,
        static_cast<uint8_t>(color.a * factor)
    );
}

/**
 * Mix two colors
 */
inline Color mix_colors(Color a, Color b, float ratio) {
    return Color(
        static_cast<uint8_t>(a.r + (b.r - a.r) * ratio),
        static_cast<uint8_t>(a.g + (b.g - a.g) * ratio),
        static_cast<uint8_t>(a.b + (b.b - a.b) * ratio),
        static_cast<uint8_t>(a.a + (b.a - a.a) * ratio)
    );
}

} // namespace utils

} // namespace lshell::design
