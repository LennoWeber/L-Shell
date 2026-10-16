// L-Shell Design System - Typography Implementation
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "typography.h"
#include <sstream>

namespace lshell::design {

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

} // namespace lshell::design
