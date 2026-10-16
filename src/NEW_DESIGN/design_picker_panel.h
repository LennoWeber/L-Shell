#pragma once

#include "shell/panel/panel.h"

#include <string_view>
#include <vector>

class Button;
class ConfigService;
class Flex;

// The design menu: every built-in L-Shell design as a card (its colours and name). A click switches the desktop
// at once and the menu stays open, so designs can be tried one after another. Opened with `panel-toggle design`.
class DesignPickerPanel : public Panel {
public:
  static constexpr std::string_view kPanelId = "design";

  // Follows design changes from anywhere (IPC, Settings), not only its own clicks.
  explicit DesignPickerPanel(ConfigService* config);

  void create() override;
  void onOpen(std::string_view context) override;
  void onClose() override;
  [[nodiscard]] float preferredWidth() const override;
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override { return PanelPlacement::Floating; }
  [[nodiscard]] LayerShellKeyboard keyboardMode() const override { return LayerShellKeyboard::Exclusive; }

private:
  static constexpr std::size_t kColumns = 5;
  static constexpr float kCardWidth = 168.0F;
  static constexpr float kCardHeight = 112.0F;
  static constexpr float kHeaderHeight = 52.0F;

  void doLayout(Renderer& renderer, float width, float height) override;
  void select(std::size_t index);
  void syncSelection();

  ConfigService* m_config = nullptr;
  Flex* m_root = nullptr;
  std::vector<Button*> m_cards;
};
