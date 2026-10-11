#include "shell/expo/expo_window_tile.h"

#include "cursor-shape-v1-client-protocol.h"
#include "render/animation/animation_manager.h"
#include "render/core/color.h"
#include "render/core/renderer.h"
#include "render/core/texture_manager.h"
#include "shell/switcher/window_switcher_tile.h"
#include "ui/builders.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <linux/input-event-codes.h>

namespace {

  constexpr float kRadiusFraction = 0.07F;
  constexpr float kMinRadius = 3.0F;
  constexpr float kMaxRadius = 10.0F;
  constexpr float kShadowAlpha = 0.38F;
  constexpr float kShadowSoftness = 9.0F;
  constexpr float kShadowDrop = 2.0F;
  // Without a snapshot: the app icon centered on a soft tile.
  constexpr float kCenterIconFraction = 0.42F;
  constexpr float kMinCenterIcon = 16.0F;
  constexpr float kMaxCenterIcon = 64.0F;
  // With a snapshot: a small badge at the bottom left, like Mission Control.
  constexpr float kBadgeFraction = 0.26F;
  constexpr float kMinBadge = 14.0F;
  constexpr float kMaxBadge = 36.0F;
  constexpr float kBadgeInsetFraction = 0.18F;
  constexpr float kRimAlpha = 0.22F;
  constexpr float kRingWidth = 2.5F;
  constexpr float kRingOutset = 2.0F;
  constexpr float kMaxSelectedScale = 1.04F;
  constexpr float kMaxSelectedGrowth = 3.0F; // logical px per side

} // namespace

ExpoWindowTile::ExpoWindowTile(float contentScale, float iconPixelScale, AsyncTextureCache* asyncTextures)
    : m_contentScale(contentScale), m_iconPixelScale(std::max(iconPixelScale, 1.0F)), m_asyncTextures(asyncTextures) {
  setAcceptedButtons(InputArea::buttonMask(BTN_LEFT));
  setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
  setOnClick([this](const InputArea::PointerData&) {
    if (m_onActivate) {
      m_onActivate();
    }
  });
  setOnEnter([this](const InputArea::PointerData&) {
    if (m_onHover) {
      m_onHover();
    }
  });

  addChild(ui::box({.out = &m_shadow, .participatesInLayout = false}));
  m_shadow->setHitTestVisible(false);
  addChild(
      ui::box({
          .out = &m_frame,
          .participatesInLayout = false,
          .configure = [](Box& box) { box.setClipChildren(true); },
      })
  );
  m_frame->addChild(
      ui::image({
          .out = &m_thumbnail,
          .fit = ImageFit::Cover,
          .visible = false,
          .participatesInLayout = false,
      })
  );
  m_frame->addChild(
      ui::image({
          .out = &m_centerIcon,
          .fit = ImageFit::Contain,
          .visible = false,
          .participatesInLayout = false,
      })
  );
  m_frame->addChild(
      ui::glyph({
          .out = &m_fallbackGlyph,
          .glyph = "app-window",
          .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          .visible = false,
          .participatesInLayout = false,
      })
  );
  addChild(
      ui::image({
          .out = &m_badge,
          .fit = ImageFit::Contain,
          .visible = false,
          .participatesInLayout = false,
      })
  );
  addChild(ui::box({.out = &m_ring, .fill = clearColorSpec(), .participatesInLayout = false}));
  m_ring->setHitTestVisible(false);

  const auto onIconReady = [this]() {
    applyVisualState();
    if (m_onInvalidate) {
      m_onInvalidate();
    }
  };
  m_centerIcon->setAsyncReadyCallback(onIconReady);
  m_badge->setAsyncReadyCallback(onIconReady);
}

void ExpoWindowTile::setTileSize(float width, float height) {
  m_tileWidth = std::max(0.0F, width);
  m_tileHeight = std::max(0.0F, height);
  setSize(m_tileWidth, m_tileHeight);
  markLayoutDirty();
}

void ExpoWindowTile::bind(Renderer& renderer, const WindowSwitcherEntry& entry) {
  if (entry.iconPath != m_iconPath) {
    m_iconPath = entry.iconPath;
    m_centerIconSize = 0;
    m_badgeSize = 0;
    m_centerIcon->clear(renderer);
    m_badge->clear(renderer);
  }
  if (entry.thumbnail != m_thumbnailImage) {
    m_thumbnailImage = entry.thumbnail;
    m_thumbnail->clear(renderer);
    bool loaded = false;
    if (m_thumbnailImage != nullptr
        && m_thumbnailImage->width > 0
        && m_thumbnailImage->height > 0
        && !m_thumbnailImage->rgba.empty()) {
      loaded = m_thumbnail->setSourceRaw(
          renderer, m_thumbnailImage->rgba.data(), m_thumbnailImage->rgba.size(), m_thumbnailImage->width,
          m_thumbnailImage->height, m_thumbnailImage->width * 4, PixmapFormat::RGBA, true
      );
    }
    m_hasThumbnail = loaded;
  }
  applyVisualState();
  markLayoutDirty();
}

void ExpoWindowTile::setSelected(bool selected, bool animate) {
  if (m_selected == selected) {
    return;
  }
  m_selected = selected;
  // Lifted above its neighbors, so overlapping windows show the selected one whole.
  setZIndex(selected ? 2 : 0);
  applyVisualState();

  AnimationManager* animations = animationManager();
  if (animations != nullptr && m_scaleAnimation != 0) {
    animations->cancel(m_scaleAnimation);
    m_scaleAnimation = 0;
  }
  // Grows by a few pixels at most, so a window filling its desktop stays inside the desktop's outline.
  const float growth = m_tileWidth > 0.0F ? (2.0F * kMaxSelectedGrowth * m_contentScale) / m_tileWidth : 0.0F;
  const float target = selected ? 1.0F + std::min(kMaxSelectedScale - 1.0F, growth) : 1.0F;
  if (animate && animations != nullptr) {
    m_scaleAnimation = animations->animate(
        scaleX(), target, Style::animFast, Easing::EaseOutCubic, [this](float value) { setScale(value); },
        [this]() { m_scaleAnimation = 0; }, this
    );
  } else {
    setScale(target);
  }
  markLayoutDirty();
  if (m_onInvalidate) {
    m_onInvalidate();
  }
}

void ExpoWindowTile::refreshIcons(Renderer& renderer) {
  const float s = m_contentScale;
  const float side = std::min(m_tileWidth, m_tileHeight);
  const bool wantCenter = !m_hasThumbnail;
  const float badge = std::round(std::clamp(side * kBadgeFraction, kMinBadge * s, kMaxBadge * s));
  // A badge would cover most of a very small snapshot.
  const bool wantBadge = m_hasThumbnail && side >= badge * 2.0F;

  if (m_iconPath.empty()) {
    m_centerIcon->setVisible(false);
    m_badge->setVisible(false);
    m_fallbackGlyph->setVisible(wantCenter);
    return;
  }

  if (wantCenter) {
    const float icon = std::round(
        side < kMinCenterIcon * s * 1.5F
            ? side * 0.7F
            : std::clamp(side * kCenterIconFraction, kMinCenterIcon * s, kMaxCenterIcon * s)
    );
    const int targetSize = std::max(1, static_cast<int>(std::round(icon * m_iconPixelScale)));
    if (targetSize != m_centerIconSize || !m_centerIcon->hasImage()) {
      m_centerIconSize = targetSize;
      m_centerIcon->setAppIconColorization(m_appIconColorizeTint);
      if (m_asyncTextures != nullptr) {
        (void)m_centerIcon->setSourceFileAsync(renderer, *m_asyncTextures, m_iconPath, targetSize, true);
      } else {
        (void)m_centerIcon->setSourceFile(renderer, m_iconPath, targetSize, true);
      }
    }
  }
  if (wantBadge) {
    const int targetSize = std::max(1, static_cast<int>(std::round(badge * m_iconPixelScale)));
    if (targetSize != m_badgeSize || !m_badge->hasImage()) {
      m_badgeSize = targetSize;
      m_badge->setAppIconColorization(m_appIconColorizeTint);
      if (m_asyncTextures != nullptr) {
        (void)m_badge->setSourceFileAsync(renderer, *m_asyncTextures, m_iconPath, targetSize, true);
      } else {
        (void)m_badge->setSourceFile(renderer, m_iconPath, targetSize, true);
      }
    }
  }
  m_centerIcon->setVisible(wantCenter && m_centerIcon->hasImage());
  m_fallbackGlyph->setVisible(wantCenter && !m_centerIcon->hasImage());
  m_badge->setVisible(wantBadge && m_badge->hasImage());
}

void ExpoWindowTile::applyVisualState() {
  const float s = m_contentScale;
  m_thumbnail->setVisible(m_hasThumbnail);
  // Opaque even on translucent palettes: the desktop must not show through a window.
  Color tile = colorForRole(m_hasThumbnail ? ColorRole::Surface : ColorRole::SurfaceVariant);
  tile.a = 1.0F;
  m_frame->setFill(tile);
  if (m_selected) {
    m_ring->setBorder(colorSpecFromRole(ColorRole::Primary), kRingWidth * s);
  } else {
    // A faint rim separates a snapshot from a desktop of the same color.
    m_ring->setBorder(rgba(1.0F, 1.0F, 1.0F, kRimAlpha), 1.0F);
  }
  if (!m_iconPath.empty()) {
    const bool wantCenter = !m_hasThumbnail;
    m_centerIcon->setVisible(wantCenter && m_centerIcon->hasImage());
    m_fallbackGlyph->setVisible(wantCenter && !m_centerIcon->hasImage());
    if (!m_hasThumbnail) {
      m_badge->setVisible(false);
    }
  }
}

void ExpoWindowTile::doLayout(Renderer& renderer) {
  const float s = m_contentScale;
  const float w = m_tileWidth;
  const float h = m_tileHeight;
  const float side = std::min(w, h);
  const float radius = std::clamp(side * kRadiusFraction, kMinRadius * s, kMaxRadius * s);

  m_shadow->setStyle(
      RoundedRectStyle{
          .fill = rgba(0.0F, 0.0F, 0.0F, kShadowAlpha),
          .radius = radius,
          .softness = kShadowSoftness * s,
          .outerShadow = true,
      }
  );
  m_shadow->setPosition(0.0F, kShadowDrop * s);
  m_shadow->setFrameSize(w, h);

  m_frame->setRadius(radius);
  m_frame->setPosition(0.0F, 0.0F);
  m_frame->setFrameSize(w, h);
  m_thumbnail->setRadius(radius);
  m_thumbnail->setPosition(0.0F, 0.0F);
  m_thumbnail->setSize(w, h);

  refreshIcons(renderer);

  const float icon = std::round(
      side < kMinCenterIcon * s * 1.5F ? side * 0.7F
                                       : std::clamp(side * kCenterIconFraction, kMinCenterIcon * s, kMaxCenterIcon * s)
  );
  m_centerIcon->setSize(icon, icon);
  m_centerIcon->setPosition(std::round((w - icon) * 0.5F), std::round((h - icon) * 0.5F));
  m_fallbackGlyph->setGlyphSize(icon * 0.8F);
  m_fallbackGlyph->measure(renderer);
  m_fallbackGlyph->setPosition(
      std::round((w - m_fallbackGlyph->width()) * 0.5F), std::round((h - m_fallbackGlyph->height()) * 0.5F)
  );

  const float badge = std::round(std::clamp(side * kBadgeFraction, kMinBadge * s, kMaxBadge * s));
  const float inset = std::round(std::max(3.0F * s, badge * kBadgeInsetFraction));
  m_badge->setSize(badge, badge);
  m_badge->setPosition(inset, h - badge - inset);

  if (m_selected) {
    const float outset = kRingOutset * s;
    m_ring->setRadius(radius + outset);
    m_ring->setPosition(-outset, -outset);
    m_ring->setFrameSize(w + (2.0F * outset), h + (2.0F * outset));
  } else {
    m_ring->setRadius(radius);
    m_ring->setPosition(0.0F, 0.0F);
    m_ring->setFrameSize(w, h);
  }

  InputArea::doLayout(renderer);
}
