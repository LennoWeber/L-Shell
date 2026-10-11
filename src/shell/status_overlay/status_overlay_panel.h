#pragma once

#include "core/timer_manager.h"
#include "shell/panel/panel.h"
#include "shell/surface/glass.h"
#include "ui/signal.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class BluetoothService;
class BrightnessService;
class Box;
class ConfigService;
class Flex;
class Glyph;
class INetworkService;
class Label;
class MprisService;
class PipeWireService;
class ProgressBar;
class Renderer;
class UPowerService;

struct StatusOverlayServices {
  ConfigService* config = nullptr;
  UPowerService* upower = nullptr;
  INetworkService* network = nullptr;
  BluetoothService* bluetooth = nullptr;
  PipeWireService* audio = nullptr;
  BrightnessService* brightness = nullptr;
  MprisService* mpris = nullptr;
};

// Full-screen glanceable status, Tahoe widget style: a large clock over the dimmed, blurred desktop and a grid of
// glass tiles (battery, network, Bluetooth, volume, brightness, now playing). Opened by the bottom handle, a
// screen edge or `panel-toggle status-overlay`.
class StatusOverlayPanel : public Panel {
public:
  static constexpr std::string_view kPanelId = "status-overlay";

  explicit StatusOverlayPanel(StatusOverlayServices services);

  void create() override;
  void onOpen(std::string_view context) override;
  void onClose() override;
  [[nodiscard]] bool handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) override;

  [[nodiscard]] float preferredWidth() const override;
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] bool fillsWidth() const noexcept override { return true; }
  [[nodiscard]] bool fillsHeight() const noexcept override { return true; }
  [[nodiscard]] bool hasDecoration() const override { return false; }
  [[nodiscard]] LayerShellLayer layer() const override { return LayerShellLayer::Overlay; }
  // Full screen like Mission Control: it takes the keyboard so Escape always closes it.
  [[nodiscard]] LayerShellKeyboard keyboardMode() const override { return LayerShellKeyboard::Exclusive; }
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override { return PanelPlacement::Floating; }
  [[nodiscard]] std::string panelScreenPosition() const override { return "center"; }

private:
  enum class TileKind : std::uint8_t { Battery, Network, Bluetooth, Volume, Brightness, Media };

  struct Tile {
    TileKind kind = TileKind::Battery;
    shell::glass::GlassNodes glass;
    Node* root = nullptr;
    Glyph* glyph = nullptr;
    Label* title = nullptr;
    Label* value = nullptr;
    ProgressBar* level = nullptr; // accent fill; null for tiles without a level
    bool valueDimmed = false;     // placeholder value ("Off", "Nothing Playing") in on_surface_variant
    bool levelNeutral = false;    // level in neutral on_surface instead of the accent (muted volume)
  };

  void doLayout(Renderer& renderer, float width, float height) override;
  void doUpdate(Renderer& renderer) override;
  void onPanelCardOpacityChanged(float opacity) override;
  void buildTiles(float scale);
  void buildTile(Tile& tile, float scale);
  // The refresh functions return true when something visible changed, i.e. a relayout is due.
  [[nodiscard]] bool refreshTile(Tile& tile);
  [[nodiscard]] bool refreshClock();
  [[nodiscard]] bool refreshAll();
  // Scrim fill and tile glass for the current card opacity and light/dark theme.
  void applyChrome();

  StatusOverlayServices m_services;
  Box* m_scrim = nullptr;
  Flex* m_rootLayout = nullptr;
  Label* m_time = nullptr;
  Label* m_date = nullptr;
  Flex* m_tileGrid = nullptr;
  std::vector<Tile> m_tiles;
  // Ticks the clock and polls the services while open: service callbacks only refresh the panels they know about.
  Timer m_refreshTimer;
  Signal<>::ScopedConnection m_paletteConn;
  bool m_glassLightTheme = false;
};
