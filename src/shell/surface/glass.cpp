#include "shell/surface/glass.h"

#include "ui/controls/box.h"

#include <algorithm>
#include <stdexcept>

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

  void applyRim(Box& box, const GlassStyle& style, bool lightTheme, float scale) {
    box.setBorder(rimColor(style, lightTheme), style.rimWidth * scale);
    box.setRimLight(lightTheme ? 0.0F : style.rimLight);
  }

  GlassNodes buildGlass(Node& parent, const GlassStyle& style) {
    (void)parent;
    (void)style;
    throw std::logic_error("buildGlass: not implemented");
  }

  void layoutGlass(GlassNodes& nodes, float x, float y, float width, float height, float scale) {
    (void)nodes;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)scale;
    throw std::logic_error("layoutGlass: not implemented");
  }

  void applyGlass(GlassNodes& nodes, const GlassStyle& style, bool lightTheme, float scale) {
    (void)nodes;
    (void)style;
    (void)lightTheme;
    (void)scale;
    throw std::logic_error("applyGlass: not implemented");
  }

} // namespace shell::glass
