// L-Shell Design System - Spacing
// Consistent spacing system based on 8px grid
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include <cstdint>

namespace lshell::design {

// ============================================================================
// Base Grid Unit (8px)
// ============================================================================

static constexpr int BASE_GRID_UNIT = 8;

// ============================================================================
// Spacing Scale (Multiples of base unit)
// ============================================================================

/**
 * Spacing value in pixels
 */
struct Spacing {
    int value;  // Value in pixels

    constexpr Spacing(int v = 0) : value(v) {}

    // Implicit conversion to int
    constexpr operator int() const { return value; }

    // Arithmetic operations
    constexpr Spacing operator+(const Spacing& other) const { return Spacing(value + other.value); }
    constexpr Spacing operator-(const Spacing& other) const { return Spacing(value - other.value); }
    constexpr Spacing operator*(int multiplier) const { return Spacing(value * multiplier); }
    constexpr Spacing operator/(int divisor) const { return Spacing(value / divisor); }

    // CSS string conversion
    std::string to_css() const;
};

// ============================================================================
// Spacing Scale (8px base grid)
// ============================================================================

// 0 - No spacing
static constexpr Spacing SPACING_NONE = Spacing(0);

// 4px - Extra small (0.5 * base)
static constexpr Spacing SPACING_XS = Spacing(4);

// 8px - Small (1 * base) - PRIMARY GRID UNIT
static constexpr Spacing SPACING_SM = Spacing(8);

// 12px - Small medium (1.5 * base)
static constexpr Spacing SPACING_SM_MD = Spacing(12);

// 16px - Medium (2 * base)
static constexpr Spacing SPACING_MD = Spacing(16);

// 20px - Medium large (2.5 * base)
static constexpr Spacing SPACING_MD_LG = Spacing(20);

// 24px - Large (3 * base)
static constexpr Spacing SPACING_LG = Spacing(24);

// 28px - Large medium (3.5 * base)
static constexpr Spacing SPACING_LG_MD = Spacing(28);

// 32px - Extra large (4 * base)
static constexpr Spacing SPACING_XL = Spacing(32);

// 36px - Extra large medium (4.5 * base)
static constexpr Spacing SPACING_XL_MD = Spacing(36);

// 40px - XX Large (5 * base)
static constexpr Spacing SPACING_XXL = Spacing(40);

// 48px - XXX Large (6 * base)
static constexpr Spacing SPACING_XXXL = Spacing(48);

// 56px - 7 * base
static constexpr Spacing SPACING_4XL = Spacing(56);

// 64px - 8 * base
static constexpr Spacing SPACING_5XL = Spacing(64);

// ============================================================================
// Component-specific Spacing
// ============================================================================

// Bar spacing
static constexpr Spacing BAR_HEIGHT_SM = Spacing(40);    // Small bar height
static constexpr Spacing BAR_HEIGHT_MD = Spacing(48);    // Medium bar height (default)
static constexpr Spacing BAR_HEIGHT_LG = Spacing(56);    // Large bar height
static constexpr Spacing BAR_HEIGHT_XL = Spacing(64);    // Extra large bar height

static constexpr Spacing BAR_PADDING_X = Spacing(16);    // Horizontal padding
static constexpr Spacing BAR_PADDING_Y = Spacing(8);     // Vertical padding
static constexpr Spacing BAR_WIDGET_SPACING = Spacing(8); // Space between widgets
static constexpr Spacing BAR_BORDER_WIDTH = Spacing(1);  // Border width

// Control Center spacing
static constexpr Spacing CC_WIDTH_SM = Spacing(320);    // Small CC width
static constexpr Spacing CC_WIDTH_MD = Spacing(400);    // Medium CC width (default)
static constexpr Spacing CC_WIDTH_LG = Spacing(500);    // Large CC width
static constexpr Spacing CC_WIDTH_XL = Spacing(600);    // Extra large CC width

static constexpr Spacing CC_HEIGHT_MIN = Spacing(400);  // Minimum height
static constexpr Spacing CC_HEIGHT_MAX = Spacing(700);  // Maximum height

static constexpr Spacing CC_PADDING = Spacing(16);       // General padding
static constexpr Spacing CC_SECTION_SPACING = Spacing(24); // Space between sections
static constexpr Spacing CC_ITEM_SPACING = Spacing(12);  // Space between items
static constexpr Spacing CC_TAB_SPACING = Spacing(8);    // Space between tabs
static constexpr Spacing CC_DIVIDER_HEIGHT = Spacing(1); // Divider height

// Widget spacing
static constexpr Spacing WIDGET_MIN_WIDTH = Spacing(24); // Minimum widget width
static constexpr Spacing WIDGET_HEIGHT = Spacing(24);   // Widget height in bar
static constexpr Spacing WIDGET_PADDING_X = Spacing(8);  // Horizontal padding
static constexpr Spacing WIDGET_PADDING_Y = Spacing(4);  // Vertical padding
static constexpr Spacing WIDGET_ICON_SIZE = Spacing(16); // Icon size
static constexpr Spacing WIDGET_TEXT_SPACING = Spacing(4); // Space between icon and text

// Button spacing
static constexpr Spacing BUTTON_HEIGHT_SM = Spacing(24); // Small button
static constexpr Spacing BUTTON_HEIGHT_MD = Spacing(32); // Medium button (default)
static constexpr Spacing BUTTON_HEIGHT_LG = Spacing(40); // Large button

static constexpr Spacing BUTTON_PADDING_X_SM = Spacing(12); // Small button horizontal padding
static constexpr Spacing BUTTON_PADDING_X_MD = Spacing(16); // Medium button horizontal padding
static constexpr Spacing BUTTON_PADDING_X_LG = Spacing(24); // Large button horizontal padding
static constexpr Spacing BUTTON_PADDING_Y = Spacing(8);    // Vertical padding

static constexpr Spacing BUTTON_ICON_SIZE_SM = Spacing(16); // Small icon
static constexpr Spacing BUTTON_ICON_SIZE_MD = Spacing(20); // Medium icon
static constexpr Spacing BUTTON_ICON_SIZE_LG = Spacing(24); // Large icon
static constexpr Spacing BUTTON_ICON_SPACING = Spacing(8);  // Space between icon and text

// Input spacing
static constexpr Spacing INPUT_HEIGHT_SM = Spacing(24); // Small input
static constexpr Spacing INPUT_HEIGHT_MD = Spacing(32); // Medium input (default)
static constexpr Spacing INPUT_HEIGHT_LG = Spacing(40); // Large input

static constexpr Spacing INPUT_PADDING_X = Spacing(12); // Horizontal padding
static constexpr Spacing INPUT_PADDING_Y = Spacing(8);  // Vertical padding

// Card spacing
static constexpr Spacing CARD_PADDING = Spacing(16);     // General padding
static constexpr Spacing CARD_BORDER_WIDTH = Spacing(1); // Border width
static constexpr Spacing CARD_RADIUS = Spacing(12);      // Border radius

// Modal spacing
static constexpr Spacing MODAL_MAX_WIDTH = Spacing(500); // Maximum width
static constexpr Spacing MODAL_PADDING = Spacing(24);    // General padding
static constexpr Spacing MODAL_RADIUS = Spacing(12);     // Border radius

// Tooltip spacing
static constexpr Spacing TOOLTIP_PADDING_X = Spacing(12); // Horizontal padding
static constexpr Spacing TOOLTIP_PADDING_Y = Spacing(8);  // Vertical padding
static constexpr Spacing TOOLTIP_RADIUS = Spacing(6);    // Border radius

// Context menu spacing
static constexpr Spacing CONTEXT_MENU_MIN_WIDTH = Spacing(180); // Minimum width
static constexpr Spacing CONTEXT_MENU_PADDING = Spacing(4);   // General padding
static constexpr Spacing CONTEXT_MENU_ITEM_PADDING_X = Spacing(12); // Item horizontal padding
static constexpr Spacing CONTEXT_MENU_ITEM_PADDING_Y = Spacing(8);  // Item vertical padding
static constexpr Spacing CONTEXT_MENU_RADIUS = Spacing(6);    // Border radius

// ============================================================================
// Border Radius
// ============================================================================

/**
 * Border radius with support for elliptical corners
 */
struct BorderRadius {
    Spacing top_left;
    Spacing top_right;
    Spacing bottom_right;
    Spacing bottom_left;

    constexpr BorderRadius(Spacing r) 
        : top_left(r), top_right(r), bottom_right(r), bottom_left(r) {}

    constexpr BorderRadius(Spacing tl, Spacing tr, Spacing br, Spacing bl)
        : top_left(tl), top_right(tr), bottom_right(br), bottom_left(bl) {}

    // Uniform radius
    static constexpr BorderRadius uniform(Spacing r) { return BorderRadius(r); }

    // Circular (for pills, avatars)
    static constexpr BorderRadius circular() { return BorderRadius(SPACING_5XL); }

    // CSS string conversion
    std::string to_css() const;
};

// None
static constexpr BorderRadius RADIUS_NONE = BorderRadius::uniform(SPACING_NONE);

// Small (4px)
static constexpr BorderRadius RADIUS_SM = BorderRadius::uniform(SPACING_XS);

// Medium (8px) - DEFAULT
static constexpr BorderRadius RADIUS_MD = BorderRadius::uniform(SPACING_SM);

// Large (12px)
static constexpr BorderRadius RADIUS_LG = BorderRadius::uniform(SPACING_LG);

// Extra Large (16px)
static constexpr BorderRadius RADIUS_XL = BorderRadius::uniform(SPACING_XL);

// Full (9999px - for circles)
static constexpr BorderRadius RADIUS_FULL = BorderRadius::uniform(Spacing(9999));

// Component-specific radii
static constexpr BorderRadius RADIUS_BAR = BorderRadius::uniform(SPACING_MD);
static constexpr BorderRadius RADIUS_CC = BorderRadius::uniform(SPACING_LG);
static constexpr BorderRadius RADIUS_WIDGET = BorderRadius::uniform(SPACING_SM);
static constexpr BorderRadius RADIUS_BUTTON = BorderRadius::uniform(SPACING_SM);
static constexpr BorderRadius RADIUS_BUTTON_ROUNDED = BorderRadius::uniform(SPACING_LG);
static constexpr BorderRadius RADIUS_BUTTON_PILL = BorderRadius::circular();
static constexpr BorderRadius RADIUS_INPUT = BorderRadius::uniform(SPACING_SM);
static constexpr BorderRadius RADIUS_CARD = BorderRadius::uniform(SPACING_LG);
static constexpr BorderRadius RADIUS_MODAL = BorderRadius::uniform(SPACING_LG);
static constexpr BorderRadius RADIUS_TOOLTIP = BorderRadius::uniform(SPACING_SM);
static constexpr BorderRadius RADIUS_CONTEXT_MENU = BorderRadius::uniform(SPACING_SM);

// ============================================================================
// Shadow Offset
// ============================================================================

/**
 * Shadow offset (x, y)
 */
struct ShadowOffset {
    int x;
    int y;

    constexpr ShadowOffset(int x, int y) : x(x), y(y) {}
    constexpr operator std::pair<int, int>() const { return {x, y}; }
};

// Common shadow offsets
static constexpr ShadowOffset SHADOW_OFFSET_SM = ShadowOffset(0, 2);
static constexpr ShadowOffset SHADOW_OFFSET_MD = ShadowOffset(0, 4);
static constexpr ShadowOffset SHADOW_OFFSET_LG = ShadowOffset(0, 8);
static constexpr ShadowOffset SHADOW_OFFSET_XL = ShadowOffset(0, 16);

} // namespace lshell::design
