#include "shell/surface/shadow.h"

#include "render/core/color.h"
#include "ui/palette.h"

#include <algorithm>

namespace shell::surface_shadow {

  bool enabled(bool componentShadow, const ShellConfig::ShadowConfig& /*shadow*/) noexcept { return componentShadow; }

  Bleed bleed(bool componentShadow, const ShellConfig::ShadowConfig& shadow) noexcept {
    if (!enabled(componentShadow, shadow)) {
      return {};
    }
    const auto offset = shadowDirectionOffset(shadow.direction);
    return {
        .left = kBlurRadius + std::max(0, -offset.x),
        .right = kBlurRadius + std::max(0, offset.x),
        .up = kBlurRadius + std::max(0, -offset.y),
        .down = kBlurRadius + std::max(0, offset.y),
    };
  }

  RoundedRectStyle
  style(const ShellConfig::ShadowConfig& shadow, float backgroundOpacity, const Shape& shape) noexcept {
    const auto offset = shadowDirectionOffset(shadow.direction);
    const float shadowAlpha = std::clamp(shadow.alpha, 0.0F, 1.0F) * std::clamp(backgroundOpacity, 0.0F, 1.0F);
    const Color hue = resolveColorSpec(shadow.color);
    return RoundedRectStyle{
        .fill = withAlpha(hue, hue.a * shadowAlpha),
        .border = Color{},
        .fillMode = FillMode::Solid,
        .corners = shape.corners,
        .logicalInset = shape.logicalInset,
        .radius = shape.radius,
        .softness = static_cast<float>(kBlurRadius),
        .borderWidth = 0.0F,
        .outerShadow = true,
        .shadowCutoutOffsetX = static_cast<float>(offset.x),
        .shadowCutoutOffsetY = static_cast<float>(offset.y),
    };
  }

  bool sameSurfaceMetrics(const ShellConfig::ShadowConfig& previous, const ShellConfig::ShadowConfig& next) noexcept {
    return previous.direction == next.direction;
  }

} // namespace shell::surface_shadow
