// L-Shell Design System - Spacing Implementation
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "spacing.h"
#include <sstream>

namespace lshell::design {

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

} // namespace lshell::design
