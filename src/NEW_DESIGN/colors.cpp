// L-Shell Design System - Colors Implementation
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "colors.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace lshell::design {

// ============================================================================
// Color Implementation
// ============================================================================

Color Color::from_hex(uint32_t hex, uint8_t alpha) {
    uint8_t r = static_cast<uint8_t>((hex >> 24) & 0xFF);
    uint8_t g = static_cast<uint8_t>((hex >> 16) & 0xFF);
    uint8_t b = static_cast<uint8_t>((hex >> 8) & 0xFF);
    uint8_t a = alpha;

    // If alpha is in the hex (0xRRGGBBAA format)
    if (alpha == 255 && (hex & 0xFF000000) != 0) {
        a = static_cast<uint8_t>((hex) & 0xFF);
    }

    return Color(r, g, b, a);
}

std::string Color::to_hex() const {
    std::ostringstream oss;
    oss << "#" 
        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(r)
        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(g)
        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    
    if (a != 255) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(a);
    }
    
    return oss.str();
}

std::string Color::to_css() const {
    if (a == 255) {
        std::ostringstream oss;
        oss << "rgb(" 
            << static_cast<int>(r) << ", "
            << static_cast<int>(g) << ", "
            << static_cast<int>(b) << ")";
        return oss.str();
    } else {
        float alpha_f = static_cast<float>(a) / 255.0f;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "rgba(" 
            << static_cast<int>(r) << ", "
            << static_cast<int>(g) << ", "
            << static_cast<int>(b) << ", "
            << alpha_f << ")";
        return oss.str();
    }
}

Color Color::blend(const Color& other, float ratio) const {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    return Color(
        static_cast<uint8_t>(r + (other.r - r) * ratio),
        static_cast<uint8_t>(g + (other.g - g) * ratio),
        static_cast<uint8_t>(b + (other.b - b) * ratio),
        static_cast<uint8_t>(a + (other.a - a) * ratio)
    );
}

Color Color::with_alpha(uint8_t new_alpha) const {
    return Color(r, g, b, new_alpha);
}

Color Color::lighten(float factor) const {
    factor = std::clamp(factor, 0.0f, 1.0f);
    float multiplier = 1.0f + (1.0f - factor);
    return Color(
        static_cast<uint8_t>(std::min(255.0f, static_cast<float>(r) * multiplier)),
        static_cast<uint8_t>(std::min(255.0f, static_cast<float>(g) * multiplier)),
        static_cast<uint8_t>(std::min(255.0f, static_cast<float>(b) * multiplier)),
        a
    );
}

Color Color::darken(float factor) const {
    factor = std::clamp(factor, 0.0f, 1.0f);
    return Color(
        static_cast<uint8_t>(static_cast<float>(r) * factor),
        static_cast<uint8_t>(static_cast<float>(g) * factor),
        static_cast<uint8_t>(static_cast<float>(b) * factor),
        a
    );
}

bool Color::operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(const Color& other) const {
    return !(*this == other);
}

// ============================================================================
// FontFamily Implementation
// ============================================================================

std::string FontFamily::to_css() const {
    if (fallbacks.empty()) {
        return primary;
    }
    
    std::string result = primary;
    for (const auto& fallback : fallbacks) {
        result += ", " + fallback;
    }
    return result;
}

std::string FontFamily::get_first_available() const {
    // In a real implementation, this would check which fonts are available
    // on the system. For now, just return the primary.
    return primary;
}

// ============================================================================
// FontSize Implementation
// ============================================================================

std::string FontSize::to_css() const {
    std::ostringstream oss;
    oss << size << "px";
    if (line_height != 1.0f) {
        oss << "/" << get_line_height_px() << "px";
    }
    return oss.str();
}

// ============================================================================
// TextStyle Implementation
// ============================================================================

std::string TextStyle::to_css() const {
    std::ostringstream oss;
    
    oss << "font-family: " << family.to_css() << "; ";
    oss << "font-weight: " << static_cast<int>(weight) << "; ";
    oss << "font-size: " << size.to_css() << "; ";
    
    // Convert color to CSS
    std::ostringstream color_oss;
    color_oss << "rgba(" 
              << static_cast<int>(color.r) << ", "
              << static_cast<int>(color.g) << ", "
              << static_cast<int>(color.b) << ", "
              << (static_cast<float>(color.a) / 255.0f) << ")";
    oss << "color: " << color_oss.str() << "; ";
    
    if (italic) {
        oss << "font-style: italic; ";
    }
    if (underline) {
        oss << "text-decoration: underline; ";
    }
    if (strike_through) {
        oss << "text-decoration: line-through; ";
    }
    
    return oss.str();
}

// ============================================================================
// Spacing Implementation
// ============================================================================

std::string Spacing::to_css() const {
    return std::to_string(value) + "px";
}

// ============================================================================
// BorderRadius Implementation
// ============================================================================

std::string BorderRadius::to_css() const {
    if (top_left == top_right && top_right == bottom_right && bottom_right == bottom_left) {
        return top_left.to_css();
    }
    
    std::ostringstream oss;
    oss << top_left.to_css() << " " 
        << top_right.to_css() << " " 
        << bottom_right.to_css() << " " 
        << bottom_left.to_css();
    return oss.str();
}

// ============================================================================
// Shadow Implementation
// ============================================================================

Shadow::Shadow(Color color, int offset_x, int offset_y, int blur_radius, int spread_radius)
    : color(color), offset_x(offset_x), offset_y(offset_y),
      blur_radius(blur_radius), spread_radius(spread_radius) {}

// ============================================================================
// Gradient Implementation
// ============================================================================

std::string Gradient::to_css() const {
    std::ostringstream oss;
    oss << "linear-gradient(" << angle << "deg, "
        << start.to_css() << ", " << end.to_css() << ")";
    return oss.str();
}

// ============================================================================
// Duration Implementation
// ============================================================================

std::string Duration::to_css() const {
    return std::to_string(ms) + "ms";
}

// ============================================================================
// Easing Function Implementation
// ============================================================================

float easing_function_value(EasingFunction easing, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    
    switch (easing) {
        // Linear
        case EasingFunction::LINEAR:
            return t;

        // Quadratic
        case EasingFunction::EASE_IN_QUAD:
            return t * t;
        case EasingFunction::EASE_OUT_QUAD:
            return t * (2.0f - t);
        case EasingFunction::EASE_IN_OUT_QUAD:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

        // Cubic
        case EasingFunction::EASE_IN_CUBIC:
            return t * t * t;
        case EasingFunction::EASE_OUT_CUBIC:
            return (--t) * t * t + 1.0f;
        case EasingFunction::EASE_IN_OUT_CUBIC:
            return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;

        // Quartic
        case EasingFunction::EASE_IN_QUART:
            return t * t * t * t;
        case EasingFunction::EASE_OUT_QUART:
            return 1.0f - (--t) * t * t * t;
        case EasingFunction::EASE_IN_OUT_QUART:
            return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (--t) * t * t * t;

        // Quintic
        case EasingFunction::EASE_IN_QUINT:
            return t * t * t * t * t;
        case EasingFunction::EASE_OUT_QUINT:
            return 1.0f + (--t) * t * t * t * t;
        case EasingFunction::EASE_IN_OUT_QUINT:
            return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f + 16.0f * (--t) * t * t * t * t;

        // Sine
        case EasingFunction::EASE_IN_SINE:
            return -std::cos(t * M_PI / 2.0f) + 1.0f;
        case EasingFunction::EASE_OUT_SINE:
            return std::sin(t * M_PI / 2.0f);
        case EasingFunction::EASE_IN_OUT_SINE:
            return -0.5f * (std::cos(M_PI * t) - 1.0f);

        // Exponential
        case EasingFunction::EASE_IN_EXPO:
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
        case EasingFunction::EASE_OUT_EXPO:
            return t == 1.0f ? 1.0f : -std::pow(2.0f, -10.0f * t) + 1.0f;
        case EasingFunction::EASE_IN_OUT_EXPO:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            if (t < 0.5f) return 0.5f * std::pow(2.0f, 20.0f * t - 10.0f);
            return -0.5f * std::pow(2.0f, -20.0f * t + 10.0f) + 1.0f;

        // Circular
        case EasingFunction::EASE_IN_CIRC:
            return -(std::sqrt(1.0f - t * t) - 1.0f);
        case EasingFunction::EASE_OUT_CIRC:
            return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
        case EasingFunction::EASE_IN_OUT_CIRC:
            if (t < 0.5f) return -(std::sqrt(1.0f - 4.0f * t * t) - 1.0f) / 2.0f;
            return (std::sqrt(-(2.0f * t - 3.0f) * (2.0f * t - 1.0f)) + 1.0f) / 2.0f;

        // Elastic
        case EasingFunction::EASE_IN_ELASTIC: {
            const float c4 = (2.0f * M_PI) / 3.0f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return -std::pow(2.0f, 10.0f * (t -= 1.0f)) * std::sin((t - 0.1f) * c4);
        }
        case EasingFunction::EASE_OUT_ELASTIC: {
            const float c4 = (2.0f * M_PI) / 3.0f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * c4) + 1.0f;
        }
        case EasingFunction::EASE_IN_OUT_ELASTIC: {
            const float c5 = (2.0f * M_PI) / 4.5f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            if (t < 0.5f) {
                return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f;
            }
            return (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
        }

        // Back
        case EasingFunction::EASE_IN_BACK: {
            const float c1 = 1.70158f;
            const float c2 = c1 + 1.0f;
            return c2 * t * t * t - c1 * t * t;
        }
        case EasingFunction::EASE_OUT_BACK: {
            const float c1 = 1.70158f;
            const float c2 = c1 + 1.0f;
            return 1.0f + c2 * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) + c1 * (t - 1.0f) * (t - 1.0f);
        }
        case EasingFunction::EASE_IN_OUT_BACK: {
            const float c1 = 1.70158f;
            const float c2 = c1 * 1.525f;
            if (t < 0.5f) {
                return ((2.0f * t) * (2.0f * t) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f;
            }
            return ((2.0f * t - 2.0f) * (2.0f * t - 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2)) / 2.0f + 1.0f;
        }

        // Bounce
        case EasingFunction::EASE_OUT_BOUNCE: {
            if (t < 1.0f / 2.75f) {
                return 7.5625f * t * t;
            } else if (t < 2.0f / 2.75f) {
                return 7.5625f * (t -= 1.5f / 2.75f) * t + 0.75f;
            } else if (t < 2.5f / 2.75f) {
                return 7.5625f * (t -= 2.25f / 2.75f) * t + 0.9375f;
            } else {
                return 7.5625f * (t -= 2.625f / 2.75f) * t + 0.984375f;
            }
        }
        case EasingFunction::EASE_IN_BOUNCE:
            return 1.0f - easing_function_value(EasingFunction::EASE_OUT_BOUNCE, 1.0f - t);
        case EasingFunction::EASE_IN_OUT_BOUNCE:
            if (t < 0.5f) {
                return (1.0f - easing_function_value(EasingFunction::EASE_OUT_BOUNCE, 1.0f - 2.0f * t)) / 2.0f;
            }
            return (1.0f + easing_function_value(EasingFunction::EASE_OUT_BOUNCE, 2.0f * t - 1.0f)) / 2.0f;

        // Spring (custom implementation)
        case EasingFunction::SPRING: {
            // Damped harmonic oscillator approximation
            const float damping = 0.15f;
            const float stiffness = 0.3f;
            const float mass = 1.0f;
            const float omega = std::sqrt(stiffness / mass);
            const float zeta = damping / (2.0f * std::sqrt(stiffness * mass));
            const float omega_d = omega * std::sqrt(1.0f - zeta * zeta);
            
            if (zeta >= 1.0f) {
                // Overdamped
                const float a = omega * (zeta + std::sqrt(zeta * zeta - 1.0f));
                const float b = omega * (zeta - std::sqrt(zeta * zeta - 1.0f));
                return 1.0f - (a * std::exp(-b * t) - b * std::exp(-a * t)) / (a - b);
            } else {
                // Underdamped
                const float phase = std::atan2(std::sqrt(1.0f - zeta * zeta), zeta);
                const float amplitude = 1.0f / std::sin(phase);
                return 1.0f - amplitude * std::exp(-zeta * omega * t) * std::sin(omega_d * t + phase);
            }
        }

        default:
            return t;
    }
}

// ============================================================================
// Animation Implementation
// ============================================================================

std::string Animation::to_css() const {
    std::ostringstream oss;
    
    if (keyframes.empty()) {
        // Simple animation
        oss << duration.to_css() << " " 
            << "easing-function(" << static_cast<int>(easing) << ") "
            << delay << "ms";
    } else {
        // Keyframe animation
        oss << duration.to_css() << " ";
        // Keyframes would need more complex CSS generation
    }
    
    if (iterations == 0) {
        oss << " infinite";
    } else if (iterations > 1) {
        oss << " " << iterations;
    }
    
    if (alternate) {
        oss << " alternate";
    }
    
    return oss.str();
}

// ============================================================================
// Shadow to_css helper
// ============================================================================

std::string Shadow::to_css() const {
    std::ostringstream oss;
    float alpha = static_cast<float>(color.a) / 255.0f;
    oss << "0 " << offset_x << "px " << offset_y << "px " 
        << blur_radius << "px " << spread_radius << "px "
        << "rgba(" << static_cast<int>(color.r) << ", "
        << static_cast<int>(color.g) << ", "
        << static_cast<int>(color.b) << ", "
        << alpha << ")";
    return oss.str();
}

} // namespace lshell::design
