#include "shell/surface/glass.h"

#include "render/core/color.h"
#include "ui/controls/box.h"
#include "ui/palette.h"

#include <algorithm>
#include <memory>

namespace shell::glass {

  namespace {

    constexpr float kShadowDrop = 2.0F;
    constexpr float kShadowSoftness = 18.0F;
    constexpr float kShadowAlphaDark = 0.36F;
    constexpr float kShadowAlphaLight = 0.16F;

  } // namespace

  RoundedRectStyle sheenStyle(const GlassStyle& style) {
    const float extent = std::clamp(style.sheenExtent, 0.0F, 1.0F);
    const Color top = rgba(1.0F, 1.0F, 1.0F, style.sheenOpacity);
    const Color clear = rgba(1.0F, 1.0F, 1.0F, 0.0F);
    return RoundedRectStyle{
        .fill = top,
        .border = rgba(0.0F, 0.0F, 0.0F, 0.0F),
        .fillMode = FillMode::LinearGradient,
        .gradientDirection = GradientDirection::Vertical,
        .gradientStops =
            {GradientStop{0.0F, top}, GradientStop{extent, clear}, GradientStop{1.0F, clear},
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
    GlassNodes nodes;
    auto make = [&parent]() {
      auto box = std::make_unique<Box>();
      box->setHitTestVisible(false);
      return static_cast<Box*>(parent.addChild(std::move(box)));
    };
    if (style.shadow) {
      nodes.shadow = make();
    }
    nodes.body = make();
    nodes.sheen = make();
    nodes.rim = make();
    applyGlass(nodes, style, isResolvedLightTheme(), 1.0F);
    return nodes;
  }

  void layoutGlass(GlassNodes& nodes, float x, float y, float width, float height, float scale) {
    const float w = std::max(width, 0.0F);
    const float h = std::max(height, 0.0F);
    for (Box* box : {nodes.body, nodes.sheen, nodes.rim}) {
      if (box != nullptr) {
        box->setPosition(x, y);
        box->setSize(w, h);
      }
    }
    if (nodes.shadow != nullptr) {
      // Tahoe glass floats: the shadow drops slightly below the shape.
      nodes.shadow->setPosition(x, y + (kShadowDrop * scale));
      nodes.shadow->setSize(w, h);
    }
  }

  void applyGlass(GlassNodes& nodes, const GlassStyle& style, bool lightTheme, float scale) {
    const bool black = style.material == Material::Black;
    if (nodes.body != nullptr) {
      if (black) {
        nodes.body->setFill(rgba(0.0F, 0.0F, 0.0F, 1.0F));
      } else {
        nodes.body->setFill(colorSpecFromRole(ColorRole::Surface, std::clamp(style.tintOpacity, 0.0F, 1.0F)));
      }
      nodes.body->setRadii(style.radius);
      nodes.body->clearBorder();
    }
    if (nodes.sheen != nullptr) {
      // A black capsule stays matte; the sheen would read as a grey band.
      nodes.sheen->setVisible(!black && style.sheenOpacity > 0.0F);
      RoundedRectStyle sheen = sheenStyle(style);
      if (lightTheme) {
        // On a light tint the white sheen is barely visible; a stronger one reads as the same highlight.
        for (auto& stop : sheen.gradientStops) {
          stop.color.a = std::min(stop.color.a * 2.0F, 1.0F);
        }
        sheen.fill = sheen.gradientStops[0].color;
      }
      nodes.sheen->setStyle(sheen);
    }
    if (nodes.rim != nullptr) {
      nodes.rim->setFill(clearColor());
      nodes.rim->setRadii(style.radius);
      // The black capsule keeps a faint light rim in both themes, so it separates from dark wallpapers.
      applyRim(*nodes.rim, style, black ? false : lightTheme, scale);
    }
    if (nodes.shadow != nullptr) {
      nodes.shadow->setVisible(style.shadow);
      nodes.shadow->setStyle(
          RoundedRectStyle{
              .fill = rgba(0.0F, 0.0F, 0.0F, lightTheme ? kShadowAlphaLight : kShadowAlphaDark),
              .radius = style.radius,
              .softness = kShadowSoftness,
              .outerShadow = true,
          }
      );
    }
  }

} // namespace shell::glass
