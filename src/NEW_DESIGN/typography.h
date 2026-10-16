// L-Shell Design System - Typography
// Modern, Professional Font System
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include <string>
#include <vector>

namespace lshell::design {

// ============================================================================
// Font Families
// ============================================================================

/**
 * Font family with fallback list
 */
struct FontFamily {
    std::string primary;
    std::vector<std::string> fallbacks;

    FontFamily(const std::string& primary, const std::vector<std::string>& fallbacks = {})
        : primary(primary), fallbacks(fallbacks) {}

    std::string to_css() const;
    std::string get_first_available() const;
};

// Primary font family (Inter with fallbacks)
static constexpr FontFamily FONT_FAMILY_PRIMARY = {
    "Inter",
    {"Noto Sans", "DejaVu Sans", "sans-serif"}
};

// Secondary font family (JetBrains Mono for code)
static constexpr FontFamily FONT_FAMILY_MONO = {
    "JetBrains Mono",
    {"Fira Code", "DejaVu Sans Mono", "monospace"}
};

// System font family
static constexpr FontFamily FONT_FAMILY_SYSTEM = {
    "-apple-system",
    {"BlinkMacSystemFont", "Segoe UI", "Roboto", "Oxygen", "Ubuntu", "Cantarell", "sans-serif"}
};

// ============================================================================
// Font Weights
// ============================================================================

enum class FontWeight {
    THIN = 100,
    EXTRA_LIGHT = 200,
    LIGHT = 300,
    REGULAR = 400,
    MEDIUM = 500,
    SEMI_BOLD = 600,
    BOLD = 700,
    EXTRA_BOLD = 800,
    BLACK = 900
};

// ============================================================================
// Font Sizes (in pixels)
// ============================================================================

/**
 * Font size with line height multiplier
 */
struct FontSize {
    int size;          // Size in pixels
    float line_height; // Line height as multiplier (1.0 = 100%)

    FontSize(int size, float line_height = 1.2f)
        : size(size), line_height(line_height) {}

    int get_line_height_px() const { return static_cast<int>(size * line_height); }
    std::string to_css() const;
};

// Tiny - For helper text, labels (10px)
static constexpr FontSize FONT_SIZE_TINY = FontSize(10, 1.2f);

// Extra Small - For small widget text (11px)
static constexpr FontSize FONT_SIZE_XS = FontSize(11, 1.2f);

// Small - For widget text (12px)
static constexpr FontSize FONT_SIZE_SM = FontSize(12, 1.2f);

// Base - For standard text (14px)
static constexpr FontSize FONT_SIZE_BASE = FontSize(14, 1.4f);

// Large - For headings in panels (16px)
static constexpr FontSize FONT_SIZE_LG = FontSize(16, 1.4f);

// Extra Large - For main headings (18px)
static constexpr FontSize FONT_SIZE_XL = FontSize(18, 1.4f);

// XX Large - For titles in control center (20px)
static constexpr FontSize FONT_SIZE_XXL = FontSize(20, 1.4f);

// XXX Large - For large titles (24px)
static constexpr FontSize FONT_SIZE_XXXL = FontSize(24, 1.4f);

// ============================================================================
// Text Styles
// ============================================================================

/**
 * Complete text style definition
 */
struct TextStyle {
    FontFamily family;
    FontWeight weight;
    FontSize size;
    Color color;  // From colors.h
    bool italic;
    bool underline;
    bool strike_through;

    TextStyle(
        FontFamily family = FONT_FAMILY_PRIMARY,
        FontWeight weight = FontWeight::REGULAR,
        FontSize size = FONT_SIZE_BASE,
        Color color = TEXT_PRIMARY,
        bool italic = false,
        bool underline = false,
        bool strike_through = false
    ) : family(family), weight(weight), size(size), color(color),
        italic(italic), underline(underline), strike_through(strike_through) {}

    std::string to_css() const;
};

// ============================================================================
// Predefined Text Styles
// ============================================================================

// Headings
static constexpr TextStyle TEXT_STYLE_H1 = {
    FONT_FAMILY_PRIMARY,
    FontWeight::BOLD,
    FONT_SIZE_XXXL,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_H2 = {
    FONT_FAMILY_PRIMARY,
    FontWeight::BOLD,
    FONT_SIZE_XXL,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_H3 = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_XL,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_H4 = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_LG,
    TEXT_PRIMARY
};

// Body text
static constexpr TextStyle TEXT_STYLE_BODY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_BASE,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_BODY_SMALL = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_BODY_LARGE = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_LG,
    TEXT_PRIMARY
};

// Secondary text
static constexpr TextStyle TEXT_STYLE_SECONDARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    TEXT_SECONDARY
};

static constexpr TextStyle TEXT_STYLE_SECONDARY_SMALL = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_TINY,
    TEXT_SECONDARY
};

// Disabled text
static constexpr TextStyle TEXT_STYLE_DISABLED = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    TEXT_DISABLED
};

// Accent text
static constexpr TextStyle TEXT_STYLE_ACCENT = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    PRIMARY_SECONDARY
};

// Success/Warning/Error text
static constexpr TextStyle TEXT_STYLE_SUCCESS = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    STATUS_SUCCESS
};

static constexpr TextStyle TEXT_STYLE_WARNING = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    STATUS_WARNING
};

static constexpr TextStyle TEXT_STYLE_ERROR = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    STATUS_ERROR
};

// Code/Monospace text
static constexpr TextStyle TEXT_STYLE_CODE = {
    FONT_FAMILY_MONO,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    TEXT_SECONDARY
};

// ============================================================================
// Component-specific Text Styles
// ============================================================================

// Bar text styles
static constexpr TextStyle TEXT_STYLE_BAR_WIDGET = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_BAR_WIDGET_SECONDARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_TINY,
    TEXT_SECONDARY
};

static constexpr TextStyle TEXT_STYLE_BAR_CLOCK = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    TEXT_PRIMARY
};

// Control Center text styles
static constexpr TextStyle TEXT_STYLE_CC_TITLE = {
    FONT_FAMILY_PRIMARY,
    FontWeight::BOLD,
    FONT_SIZE_XL,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_CC_SECTION_TITLE = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_LG,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_CC_ITEM = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_BASE,
    TEXT_PRIMARY
};

static constexpr TextStyle TEXT_STYLE_CC_ITEM_SECONDARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    TEXT_SECONDARY
};

static constexpr TextStyle TEXT_STYLE_CC_TAB = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_SM,
    TEXT_SECONDARY
};

static constexpr TextStyle TEXT_STYLE_CC_TAB_ACTIVE = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    TEXT_PRIMARY
};

// Widget text styles
static constexpr TextStyle TEXT_STYLE_WIDGET_LABEL = {
    FONT_FAMILY_PRIMARY,
    FontWeight::MEDIUM,
    FONT_SIZE_TINY,
    TEXT_SECONDARY
};

static constexpr TextStyle TEXT_STYLE_WIDGET_VALUE = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    TEXT_PRIMARY
};

// Button text styles
static constexpr TextStyle TEXT_STYLE_BUTTON_PRIMARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    BUTTON_PRIMARY_TEXT
};

static constexpr TextStyle TEXT_STYLE_BUTTON_SECONDARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::SEMI_BOLD,
    FONT_SIZE_SM,
    BUTTON_SECONDARY_TEXT
};

static constexpr TextStyle TEXT_STYLE_BUTTON_TERTIARY = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_SM,
    BUTTON_TERTIARY_TEXT
};

// Input text styles
static constexpr TextStyle TEXT_STYLE_INPUT = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_BASE,
    INPUT_TEXT
};

static constexpr TextStyle TEXT_STYLE_INPUT_PLACEHOLDER = {
    FONT_FAMILY_PRIMARY,
    FontWeight::REGULAR,
    FONT_SIZE_BASE,
    INPUT_PLACEHOLDER
};

// ============================================================================
// Letter Spacing
// ============================================================================

static constexpr float LETTER_SPACING_NORMAL = 0.0f;    // Normal
static constexpr float LETTER_SPACING_TIGHT = -0.02f;   // -2%
static constexpr float LETTER_SPACING_WIDE = 0.05f;     // +5%

// ============================================================================
// Text Transform
// ============================================================================

enum class TextTransform {
    NONE,
    CAPITALIZE,
    UPPERCASE,
    LOWERCASE
};

// ============================================================================
// Text Decoration
// ============================================================================

enum class TextDecoration {
    NONE,
    UNDERLINE,
    OVERLINE,
    LINE_THROUGH
};

} // namespace lshell::design
