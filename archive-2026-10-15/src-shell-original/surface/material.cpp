#include "shell/surface/material.h"

#include "render/core/color.h"
#include "ui/controls/box.h"
#include "ui/palette.h"

#include <algorithm>
#include <memory>

namespace shell::material {

  namespace {

    constexpr float kHairlineAlphaDark = 0.09F;
    constexpr float kHairlineAlphaLight = 0.10F;
    constexpr float kShadowSoftness = 22.0F;
    constexpr float kShadowAlphaDark = 0.34F;
    constexpr float kShadowAlphaLight = 0.14F;

  } // namespace

  Color hairlineColor(bool lightTheme, Kind kind) {
    // Ink keeps a faint light edge in both themes, so it separates from a dark wallpaper.
    if (lightTheme && kind == Kind::Slate) {
      return rgba(0.0F, 0.0F, 0.0F, kHairlineAlphaLight);
    }
    return rgba(1.0F, 1.0F, 1.0F, kHairlineAlphaDark);
  }

  void applyHairline(Box& box, bool lightTheme, float scale, Kind kind) {
    box.setBorder(hairlineColor(lightTheme, kind), Style::borderWidth * scale);
    box.setRimLight(0.0F);
  }

  RoundedRectStyle shadowStyle(const Radii& radius, bool lightTheme) {
    return RoundedRectStyle{
        .fill = rgba(0.0F, 0.0F, 0.0F, lightTheme ? kShadowAlphaLight : kShadowAlphaDark),
        .radius = radius,
        .softness = kShadowSoftness,
        .outerShadow = true,
    };
  }

  Nodes build(Node& parent, const Spec& spec) {
    Nodes nodes;
    auto make = [&parent]() {
      auto box = std::make_unique<Box>();
      box->setHitTestVisible(false);
      return static_cast<Box*>(parent.addChild(std::move(box)));
    };
    if (spec.elevation == Elevation::Raised) {
      nodes.shadow = make();
    }
    nodes.body = make();
    apply(nodes, spec, isResolvedLightTheme(), 1.0F);
    return nodes;
  }

  void layout(Nodes& nodes, float x, float y, float width, float height, float scale) {
    const float w = std::max(width, 0.0F);
    const float h = std::max(height, 0.0F);
    if (nodes.body != nullptr) {
      nodes.body->setPosition(x, y);
      nodes.body->setSize(w, h);
    }
    if (nodes.shadow != nullptr) {
      nodes.shadow->setPosition(x, y + (kShadowDrop * scale));
      nodes.shadow->setSize(w, h);
    }
  }

  void apply(Nodes& nodes, const Spec& spec, bool lightTheme, float scale) {
    if (nodes.body != nullptr) {
      if (spec.kind == Kind::Ink) {
        nodes.body->setFill(rgba(0.0F, 0.0F, 0.0F, 1.0F));
      } else {
        nodes.body->setFill(colorSpecFromRole(ColorRole::Surface, std::clamp(spec.tint, 0.0F, 1.0F)));
      }
      nodes.body->setRadii(spec.radius);
      applyHairline(*nodes.body, lightTheme, scale, spec.kind);
    }
    if (nodes.shadow != nullptr) {
      nodes.shadow->setVisible(spec.elevation == Elevation::Raised);
      nodes.shadow->setStyle(shadowStyle(spec.radius, lightTheme));
    }
  }

} // namespace shell::material
