#include "shell/surface/glass.h"

#include <algorithm>

namespace shell::glass {

  RoundedRectStyle sheenStyle(const GlassStyle& style) {
    const float extent = std::clamp(style.sheenExtent, 0.0F, 1.0F);
    const Color top = rgba(1.0F, 1.0F, 1.0F, style.sheenOpacity);
    const Color clear = rgba(1.0F, 1.0F, 1.0F, 0.0F);
    return RoundedRectStyle{
        .fill = top,
        .border = rgba(0.0F, 0.0F, 0.0F, 0.0F),
        .fillMode = FillMode::LinearGradient,
        .gradientDirection = GradientDirection::Vertical,
        .gradientStops = {GradientStop{0.0F, top}, GradientStop{extent, clear}, GradientStop{1.0F, clear},
                          GradientStop{1.0F, clear}},
        .radius = style.radius,
        .softness = 1.0F,
    };
  }

  Color rimColor(const GlassStyle& style, bool lightTheme) {
    return lightTheme ? rgba(0.0F, 0.0F, 0.0F, style.rimOpacity * 0.6F) : rgba(1.0F, 1.0F, 1.0F, style.rimOpacity);
  }

} // namespace shell::glass
