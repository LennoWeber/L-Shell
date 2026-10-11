#pragma once

#include "render/core/render_styles.h"
#include "ui/style.h"

#include <cstdint>

// Tahoe-style "Liquid Glass" chrome built from plain boxes: a translucent tint over the compositor blur, a
// specular sheen fading down from the top edge, a hairline rim and a soft drop shadow. Accent colors never
// appear here; the material is neutral in both light and dark.
class Box;
class Node;

namespace shell::glass {

  enum class Material : std::uint8_t {
    Glass, // translucent neutral tint, needs compositor blur behind it to read as glass
    Black, // opaque black capsule (iPhone Dynamic Island look)
  };

  struct GlassStyle {
    Material material = Material::Glass;
    Radii radius = Style::radiusXl;
    float tintOpacity = 0.55F;     // surface tint over the blurred backdrop
    float sheenOpacity = 0.10F;    // top specular sheen, fades to 0 at `sheenExtent`
    float sheenExtent = 0.45F;     // fraction of the height the sheen covers
    float rimOpacity = 0.16F;      // hairline rim strength
    float rimWidth = 1.0F;         // logical px
    float rimLight = 0.85F;        // edge light on the rim (RoundedRectStyle::rimLight), dark themes only
    bool shadow = true;
  };

  // The boxes composing one glass shape, bottom to top. Owned by the scene graph.
  struct GlassNodes {
    Box* shadow = nullptr;
    Box* body = nullptr;
    Box* sheen = nullptr;
    Box* rim = nullptr;
  };

  // Specular sheen over a glass body: white fading out from the top edge to `sheenExtent` of the height.
  [[nodiscard]] RoundedRectStyle sheenStyle(const GlassStyle& style);
  // Hairline rim color: a light edge on a dark theme, a dark edge on a light theme.
  [[nodiscard]] Color rimColor(const GlassStyle& style, bool lightTheme);
  // Puts the glass rim on a box's border: the edge-lit light rim on a dark theme, a plain dark hairline on a
  // light one (where a light rim would vanish into the tint).
  void applyRim(Box& box, const GlassStyle& style, bool lightTheme, float scale = 1.0F);

} // namespace shell::glass
