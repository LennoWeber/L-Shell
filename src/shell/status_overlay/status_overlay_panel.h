#pragma once

#include "shell/panel/panel.h"
#include "shell/surface/glass.h"

#include <cstdint>
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
  };

  StatusOverlayServices m_services;
  Box* m_scrim = nullptr;
  Flex* m_rootLayout = nullptr;
  Label* m_time = nullptr;
  Label* m_date = nullptr;
  Flex* m_tileGrid = nullptr;
  std::vector<Tile> m_tiles;
};
