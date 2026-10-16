#pragma once

#include "config/config_types.h"
#include "shell/panel/panel.h"
#include "ui/style.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

class Box;
class Button;
class Flex;
class GridView;
class InputArea;
class Label;
class Node;
class CountdownRing;
class Renderer;
class ConfigService;
class SessionActionRunner;

class SessionPanel : public Panel {
public:
  explicit SessionPanel(ConfigService* config, SessionActionRunner& actionRunner)
      : m_config(config), m_actionRunner(&actionRunner) {}

  void create() override;
  void onOpen(std::string_view context) override;
  void onClose() override;
  void onFrameTick(float deltaMs) override;
  [[nodiscard]] bool handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) override;

  [[nodiscard]] float preferredWidth() const override;
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] bool hasDecoration() const override { return true; }
  [[nodiscard]] LayerShellKeyboard keyboardMode() const override { return LayerShellKeyboard::Exclusive; }
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override;
  [[nodiscard]] std::optional<float> contentPadding() const override;

private:
  struct ActionCountdownOverlay {
    Flex* root = nullptr;
    Box* scrim = nullptr;
    CountdownRing* ring = nullptr;
  };

  struct PendingCountdown {
    std::size_t index = 0;
    double remainingMs = 0.0;
    double totalMs = 0.0;
  };

  // Slate sheet (the default): quiet actions (lock, sleep, log out, custom) as a row of tiles over the power
  // actions (restart, shut down) as a row of wide buttons, so what ends the session stands apart.
  static constexpr float kSheetTileWidth = 152.0F;
  static constexpr float kSheetTileHeight = 84.0F;
  static constexpr float kSheetPowerHeight = 44.0F;
  static constexpr float kActionButtonMinHeight = 112.0F;
  static constexpr float kButtonMinWidth = 152.0F;
  static constexpr std::size_t kMaxColumns = 5;
  // Hairline grid (Monolith): a sheet with a small versal title over the tiles at its foot.
  static constexpr float kHairlineWidth = 380.0F;
  static constexpr float kHairlinePadding = 22.0F;
  static constexpr float kHairlineTitleSize = 11.0F;
  static constexpr float kHairlineTileHeight = 66.0F;
  // Menu style (the Apple menu): slim rows with an icon, the name and the shortcut on the right.
  static constexpr float kMenuWidth = 236.0F;
  static constexpr float kMenuRowHeight = 30.0F;
  static constexpr float kMenuRowGap = 1.0F;

  void doLayout(Renderer& renderer, float width, float height) override;
  void doUpdate(Renderer& renderer) override;
  void onPanelCardOpacityChanged(float opacity) override;
  void armEntry(std::size_t index);
  void executeEntry(std::size_t index);
  void cancelCountdown();
  void updateSelectionVisuals();
  void updateCountdownVisuals();
  void layoutCountdownOverlays(Renderer& renderer);
  void restoreEntryBadges();
  void hideCountdownOverlays();
  void attachCountdownOverlay(Button& button, ActionCountdownOverlay& overlay, float scale, bool wideButton);
  void syncCountdownOverlayColors(std::size_t index);
  [[nodiscard]] std::vector<SessionPanelActionConfig> effectiveActions() const;
  [[nodiscard]] Button* createActionButton(const SessionPanelActionConfig& cfg, std::size_t index, float scale);
  [[nodiscard]] std::size_t entryCountForLayout() const;
  [[nodiscard]] bool gridEnabled() const;
  [[nodiscard]] bool menuStyle() const;
  [[nodiscard]] bool sheetStyle() const;
  [[nodiscard]] bool hairlineStyle() const;
  // Sheet style: the quiet actions, then the power ones; m_quietCount marks the split.
  [[nodiscard]] static bool isPowerAction(const SessionPanelActionConfig& action);
  // Arrow-key target between the sheet's two rows (proportional column), or nullopt to stay.
  [[nodiscard]] std::optional<std::size_t> sheetVerticalTarget(std::size_t from, bool down) const;
  // Shortcut hint color of a menu row: white on the accent highlight, secondary text otherwise.
  void syncMenuShortcut(std::size_t index);
  [[nodiscard]] std::size_t visibleColumnCount() const;
  [[nodiscard]] std::size_t visibleRowCount() const;

  Node* m_rootLayout = nullptr;
  std::size_t m_quietCount = 0;
  std::vector<SessionPanelActionConfig> m_visibleEntries;
  std::vector<Button*> m_visibleButtons;
  std::vector<ActionCountdownOverlay> m_countdownOverlays;
  std::vector<std::optional<std::string>> m_entryShortcutBadges;
  std::vector<Label*> m_menuShortcuts; // menu style: right-aligned shortcut hint per row (nullptr without one)
  std::vector<bool> m_menuHovered;
  std::optional<std::size_t> m_selectedIndex;
  std::optional<PendingCountdown> m_pendingCountdown;
  bool m_actionQueued = false;
  ConfigService* m_config = nullptr;
  SessionActionRunner* m_actionRunner = nullptr;
};
