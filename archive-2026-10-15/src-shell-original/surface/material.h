#pragma once

#include "render/core/render_styles.h"
#include "ui/style.h"

#include <cstdint>

// Slate, the one material of L-Shell (see DESIGN.md): a graphite tint over the compositor blur, a 1px hairline
// and, on raised surfaces only, a soft drop shadow. No sheen, no lit rim, no gradients. Every shell surface (bar,
// island, dock, panels, OSD, toasts, overlays) is drawn from it, so they read as one system.
class Box;
class Node;

namespace shell::material {

  // Fill opacity of the Slate tint over the blurred backdrop. High enough that text stays legible on any
  // wallpaper and that compositor blur rules with an alpha cut-off (Hyprland `ignore_alpha`) always catch it.
  inline constexpr float kTint = Style::slateTint;
  // Inner wells (tiles, rows, fields) sit on a surface as a lighter step of the same graphite.
  inline constexpr float kWellOpacity = 0.55F;

  enum class Kind : std::uint8_t {
    Slate, // translucent graphite over the compositor blur
    Ink,   // opaque black, for an island that should melt into a notch-like strip
  };

  // Raised surfaces float over content and cast the shadow; flush ones sit on a screen edge and do not.
  enum class Elevation : std::uint8_t {
    Flush,
    Raised,
  };

  struct Spec {
    Kind kind = Kind::Slate;
    Elevation elevation = Elevation::Raised;
    Radii radius = Style::radiusXl;
    float tint = kTint;
  };

  // The boxes of one material shape, bottom to top. Owned by the scene graph.
  struct Nodes {
    Box* shadow = nullptr;
    Box* body = nullptr;
  };

  // The 1px hairline around every surface: light on a dark theme, dark on a light one.
  [[nodiscard]] Color hairlineColor(bool lightTheme, Kind kind = Kind::Slate);
  // Puts the hairline on a box's border (the box keeps its fill).
  void applyHairline(Box& box, bool lightTheme, float scale = 1.0F, Kind kind = Kind::Slate);
  // The drop shadow of raised surfaces, as a style for a box laid out under the surface.
  [[nodiscard]] RoundedRectStyle shadowStyle(const Radii& radius, bool lightTheme);
  // Vertical drop of the shadow below its surface (logical px).
  inline constexpr float kShadowDrop = 6.0F;

  // Adds the shadow (raised only) and body boxes under `parent` and returns them.
  [[nodiscard]] Nodes build(Node& parent, const Spec& spec);
  // Sizes and positions every box of a material shape to x/y/width/height.
  void layout(Nodes& nodes, float x, float y, float width, float height, float scale);
  // Restyles a material shape for the current light/dark palette or a new spec.
  void apply(Nodes& nodes, const Spec& spec, bool lightTheme, float scale);

} // namespace shell::material
