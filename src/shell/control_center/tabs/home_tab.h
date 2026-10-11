#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "shell/control_center/control_center_services.h"
#include "shell/control_center/shortcut_services.h"
#include "shell/control_center/tab.h"
#include "ui/signal.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class BluetoothService;
class Box;
class BrightnessService;
class Button;
class CompositorPlatform;
class ConfigService;
class GammaService;
class Glyph;
class HttpClient;
class IdleInhibitor;
class Image;
class INetworkService;
class InputArea;
class Label;
class MprisService;
class NotificationManager;
class PipeWireService;
class Shortcut;
class Slider;
struct BrightnessDisplay;

struct ShortcutPad {
  // Survives HomeTab::onClose(); button/label are nulled with the scene.
  std::unique_ptr<Shortcut> shortcut;
  Button* button = nullptr;
  Label* label = nullptr;
  // Last on/enabled state pushed to the button, so its palette is rebuilt only on change.
  std::int8_t appliedState = -1;
};

// The Control Center home view, laid out like the macOS Tahoe Control Center: a four-column grid of
// rounded glass module tiles (connectivity, focus, caffeine, night light, display, sound, now playing)
// above a row of round shortcut buttons.
class HomeTab : public Tab {
public:
  explicit HomeTab(const ControlCenterServices& services);
  ~HomeTab() override;

  std::unique_ptr<Flex> create() override;
  void setActive(bool active) override;
  void onClose() override;

  // Opens another control-center tab by its key (e.g. "network"); wired by the panel.
  void setNavigateCallback(std::function<void(std::string_view)> callback) { m_navigate = std::move(callback); }
  // Fired after a layout in which the grid's natural height changed, so the panel can resize to it.
  void setNaturalHeightChangedCallback(std::function<void()> callback) { m_naturalHeightChanged = std::move(callback); }
  // Height the module grid needs at `scale`. Valid before create(): it is derived from service state
  // and config, not from the scene.
  [[nodiscard]] float naturalHeight(float scale) const;
  [[nodiscard]] bool dragging() const noexcept;

private:
  enum class TileId : std::uint8_t {
    Connectivity,
    Focus,
    Caffeine,
    NightLight,
    Display,
    Sound,
    Media,
    Count,
  };
  static constexpr std::size_t kTileCount = static_cast<std::size_t>(TileId::Count);

  struct Tile {
    Flex* root = nullptr;
    Box* surface = nullptr;
    // Whole-tile click target behind the content; null for tiles whose rows carry their own.
    InputArea* overlay = nullptr;
    bool focused = false;
  };

  // A round toggle followed by a title and a status line (connectivity rows, focus).
  struct ToggleRow {
    Flex* root = nullptr;
    // Text click target behind the row content, and the soft highlight that marks its keyboard focus.
    InputArea* overlay = nullptr;
    Box* highlight = nullptr;
    Button* circle = nullptr;
    Label* title = nullptr;
    Label* status = nullptr;
    std::int8_t appliedState = -1;
  };

  void doLayout(Renderer& renderer, float contentWidth, float bodyHeight) override;
  void doUpdate(Renderer& renderer) override;
  void onPanelCardOpacityChanged(float opacity) override;

  [[nodiscard]] Tile& tile(TileId id) noexcept { return m_tiles[static_cast<std::size_t>(id)]; }
  void addTileChrome(TileId id, Flex& root, std::function<void()> onActivate);
  void addRowOverlay(ToggleRow& row, std::function<void()> onActivate);
  void applyTileChrome();
  void layoutTileChrome();
  void navigate(std::string_view tab);

  [[nodiscard]] std::unique_ptr<Flex>
  buildToggleRow(ToggleRow& row, std::string_view glyph, std::string title, std::function<void()> onToggle);
  [[nodiscard]] std::unique_ptr<Flex> buildSmallToggleTile(
      TileId id, Button** circle, std::string_view glyph, std::string title, std::function<void()> onToggle,
      std::function<void()> onRightClick
  );
  [[nodiscard]] std::unique_ptr<Flex> buildDisplayTile();
  [[nodiscard]] std::unique_ptr<Flex> buildSoundTile();
  [[nodiscard]] std::unique_ptr<Flex> buildMediaTile();
  [[nodiscard]] std::unique_ptr<Flex> buildShortcutRows();
  void rebuildShortcutPads();

  void sync(Renderer& renderer);
  void syncConnectivity();
  void syncToggles();
  void syncDisplay();
  void syncSound();
  void syncMedia(Renderer& renderer);
  void syncShortcuts();

  void toggleWifi();
  void toggleBluetooth();
  void toggleVpn();
  void toggleDoNotDisturb();
  void toggleCaffeine();
  void toggleNightLight();
  void toggleMute();

  [[nodiscard]] const BrightnessDisplay* targetDisplay() const;
  [[nodiscard]] std::size_t shortcutItemCount() const;
  [[nodiscard]] float volumeSliderMax() const;
  void queueBrightness(float value);
  void flushBrightness();
  void queueVolume(float value);
  void flushVolume();

  MprisService* m_mpris = nullptr;
  HttpClient* m_httpClient = nullptr;
  ConfigService* m_config = nullptr;
  INetworkService* m_network = nullptr;
  BluetoothService* m_bluetooth = nullptr;
  NotificationManager* m_notifications = nullptr;
  IdleInhibitor* m_idleInhibitor = nullptr;
  GammaService* m_nightLight = nullptr;
  CompositorPlatform* m_platform = nullptr;
  BrightnessService* m_brightness = nullptr;
  PipeWireService* m_audio = nullptr;
  ShortcutServices m_services;
  bool m_active = false;

  std::function<void(std::string_view)> m_navigate;
  std::function<void()> m_naturalHeightChanged;
  float m_reportedNaturalHeight = 0.0F;
  Signal<>::ScopedConnection m_paletteConn;

  Flex* m_rootLayout = nullptr;
  std::array<Tile, kTileCount> m_tiles{};

  ToggleRow m_wifiRow;
  ToggleRow m_bluetoothRow;
  ToggleRow m_vpnRow;
  ToggleRow m_focusRow;

  Button* m_caffeineButton = nullptr;
  std::int8_t m_caffeineState = -1;
  Button* m_nightLightButton = nullptr;
  std::int8_t m_nightLightState = -1;

  // Slider commits are throttled to one per frame interval; after a commit the service state is
  // ignored briefly so its lagging echo cannot pull the thumb back.
  Slider* m_brightnessSlider = nullptr;
  std::string m_brightnessDisplayId;
  float m_lastBrightness = -1.0F;
  float m_pendingBrightness = -1.0F;
  std::chrono::steady_clock::time_point m_lastBrightnessCommitAt;
  std::chrono::steady_clock::time_point m_brightnessHoldUntil;
  Timer m_brightnessTimer;

  Slider* m_volumeSlider = nullptr;
  Button* m_muteButton = nullptr;
  std::uint32_t m_pendingSinkId = 0;
  float m_lastVolume = -1.0F;
  float m_pendingVolume = -1.0F;
  std::chrono::steady_clock::time_point m_lastVolumeCommitAt;
  std::chrono::steady_clock::time_point m_volumeHoldUntil;
  Timer m_volumeTimer;
  // Set while the panel pushes service state into a slider, so the slider's change callback
  // does not echo it back to the service.
  bool m_syncingSlider = false;

  Image* m_mediaArt = nullptr;
  Glyph* m_mediaArtFallback = nullptr;
  Label* m_mediaTitle = nullptr;
  Label* m_mediaArtist = nullptr;
  Button* m_mediaPrev = nullptr;
  Button* m_mediaPlay = nullptr;
  Button* m_mediaNext = nullptr;
  // -1 until the first sync; then whether the tile shows the dimmed idle state.
  std::int8_t m_mediaIdle = -1;
  std::string m_loadedMediaArtUrl;
  std::unordered_set<std::string> m_pendingArtDownloads;
  std::shared_ptr<void> m_aliveGuard = std::make_shared<int>(0);

  std::vector<ShortcutPad> m_shortcutPads;
  // Plugin config as of the last shortcut build. A plugin shortcut seeds its Luau runtime at
  // construction, so a change here must block instance reuse.
  PluginsConfig m_lastPlugins;
};
