#pragma once

#include "capture/screencopy_capture.h"
#include "render/scene/input_area.h"
#include "ui/palette.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class AsyncTextureCache;
class Box;
class Glyph;
class Image;
class Renderer;
struct WindowSwitcherEntry;

// One window miniature inside an expo card: its live snapshot (or its app icon on a soft tile
// until a snapshot arrives), an app icon badge at the bottom left and an accent ring while selected.
class ExpoWindowTile : public InputArea {
public:
  // `iconPixelScale` is the output's buffer scale, so icons load at their drawn size in pixels.
  ExpoWindowTile(float contentScale, float iconPixelScale, AsyncTextureCache* asyncTextures);

  void setTileSize(float width, float height);
  void setAppIconColorizeTint(std::optional<ColorSpec> tint) { m_appIconColorizeTint = tint; }
  void setOnActivate(std::function<void()> callback) { m_onActivate = std::move(callback); }
  void setOnHover(std::function<void()> callback) { m_onHover = std::move(callback); }
  void setOnInvalidate(std::function<void()> callback) { m_onInvalidate = std::move(callback); }
  // Cheap when the icon and snapshot did not change.
  void bind(Renderer& renderer, const WindowSwitcherEntry& entry);
  void setSelected(bool selected, bool animate);
  [[nodiscard]] bool selected() const noexcept { return m_selected; }

protected:
  void doLayout(Renderer& renderer) override;

private:
  void refreshIcons(Renderer& renderer);
  void applyVisualState();

  float m_contentScale = 1.0F;
  float m_iconPixelScale = 1.0F;
  float m_tileWidth = 0.0F;
  float m_tileHeight = 0.0F;

  Box* m_shadow = nullptr;
  Box* m_frame = nullptr;
  Image* m_thumbnail = nullptr;
  Image* m_centerIcon = nullptr;
  Glyph* m_fallbackGlyph = nullptr;
  Image* m_badge = nullptr;
  Box* m_ring = nullptr;

  bool m_selected = false;
  bool m_hasThumbnail = false;
  std::string m_iconPath;
  int m_centerIconSize = 0;
  int m_badgeSize = 0;
  std::uint32_t m_scaleAnimation = 0;
  std::shared_ptr<const ScreencopyImage> m_thumbnailImage;
  AsyncTextureCache* m_asyncTextures = nullptr;
  std::optional<ColorSpec> m_appIconColorizeTint;
  std::function<void()> m_onActivate;
  std::function<void()> m_onHover;
  std::function<void()> m_onInvalidate;
};
