#include "shell/design/design_picker_panel.h"

#include "config/config_service.h"
#include "config/design_presets.h"
#include "i18n/i18n.h"
#include "render/core/color.h"
#include "shell/panel/panel_manager.h"
#include "ui/builders.h"
#include "ui/controls/button.h"
#include "ui/controls/flex.h"
#include "ui/controls/grid_view.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <cmath>
#include <memory>

namespace {

  [[nodiscard]] Color colorFromRgb(std::uint32_t rgb) {
    return rgba(
        static_cast<float>((rgb >> 16) & 0xffU) / 255.0F, static_cast<float>((rgb >> 8) & 0xffU) / 255.0F,
        static_cast<float>(rgb & 0xffU) / 255.0F, 1.0F
    );
  }

  [[nodiscard]] std::size_t rowCount(std::size_t items, std::size_t columns) { return (items + columns - 1) / columns; }

} // namespace

DesignPickerPanel::DesignPickerPanel(ConfigService* config) : m_config(config) {
  if (m_config != nullptr) {
    m_config->addReloadCallback(
        [this]() {
          if (m_config->lastChange().design && !m_cards.empty()) {
            syncSelection();
            PanelManager::instance().refresh();
          }
        },
        "design-picker"
    );
  }
}

float DesignPickerPanel::preferredWidth() const {
  const auto columns = static_cast<float>(kColumns);
  return scaled(kCardWidth * columns + Style::spaceSm * (columns - 1.0F) + Style::panelPadding * 2.0F);
}

float DesignPickerPanel::preferredHeight() const {
  const auto rows = static_cast<float>(rowCount(noctalia::config::designPresets().size(), kColumns));
  return std::ceil(scaled(
      kHeaderHeight + kCardHeight * rows + Style::spaceSm * (rows - 1.0F) + Style::spaceMd + Style::panelPadding * 2.0F
  ));
}

void DesignPickerPanel::create() {
  const float scale = contentScale();
  auto root = ui::column({.out = &m_root, .align = FlexAlign::Stretch, .gap = Style::spaceMd * scale});

  auto header = ui::column({.align = FlexAlign::Start, .gap = 2.0F * scale});
  header->addChild(ui::label({
      .text = i18n::tr("design.title"),
      .fontSize = Style::fontSizeTitle * scale,
      .fontWeight = FontWeight::Bold,
      .color = colorSpecFromRole(ColorRole::OnSurface),
  }));
  header->addChild(ui::label({
      .text = i18n::tr("design.subtitle"),
      .fontSize = Style::fontSizeCaption * scale,
      .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
  }));
  root->addChild(std::move(header));

  auto grid = std::make_unique<GridView>();
  grid->setColumns(kColumns);
  grid->setColumnGap(Style::spaceSm * scale);
  grid->setRowGap(Style::spaceSm * scale);
  grid->setStretchItems(true);
  grid->setUniformCellSize(true);
  grid->setMinCellWidth(kCardWidth * scale);
  grid->setMinCellHeight(kCardHeight * scale);

  m_cards.clear();
  const auto presets = noctalia::config::designPresets();
  for (std::size_t index = 0; index < presets.size(); ++index) {
    const auto& preset = presets[index];
    auto card = ui::button({
        .contentAlign = ButtonContentAlign::Start,
        .variant = ButtonVariant::Default,
        .surfaceOpacity = panelCardOpacity(),
        .minWidth = kCardWidth * scale,
        .minHeight = kCardHeight * scale,
        .padding = Style::spaceSm * scale,
        .gap = Style::spaceSm * scale,
        .radius = Style::scaledRadiusLg(scale),
        .flexGrow = 1.0F,
        .onClick = [this, index]() { select(index); },
        .configure =
            [](Button& control) {
              control.setDirection(FlexDirection::Vertical);
              control.setAlign(FlexAlign::Stretch);
              control.setTabStop(false);
            },
    });
    // The design in four strokes: background, surface, text, accent.
    auto swatches = ui::row({.align = FlexAlign::Stretch, .gap = 0.0F, .flexGrow = 1.0F});
    for (std::size_t s = 0; s < preset.swatches.size(); ++s) {
      const float radius = Style::scaledRadiusMd(scale);
      const bool first = s == 0;
      const bool last = s + 1 == preset.swatches.size();
      swatches->addChild(ui::box({
          .fill = fixedColorSpec(colorFromRgb(preset.swatches[s])),
          .flexGrow = first ? 2.0F : 1.0F,
          .configure =
              [radius, first, last](Box& box) {
                box.setHitTestVisible(false);
                const float start = first ? radius : 0.0F;
                const float end = last ? radius : 0.0F;
                box.setRadii(Radii{start, end, end, start});
              },
      }));
    }
    card->addChild(std::move(swatches));
    card->addChild(ui::label({
        .text = std::string(preset.name),
        .fontSize = Style::fontSizeBody * scale,
        .fontWeight = FontWeight::Bold,
        .color = colorSpecFromRole(ColorRole::OnSurface),
        .maxLines = 1,
    }));
    m_cards.push_back(card.get());
    grid->addChild(std::move(card));
  }
  root->addChild(std::move(grid));
  setRoot(std::move(root));
  if (m_animations != nullptr) {
    this->root()->setAnimationManager(m_animations);
  }
  syncSelection();
}

void DesignPickerPanel::onOpen(std::string_view /*context*/) { syncSelection(); }

void DesignPickerPanel::onClose() {
  m_cards.clear();
  m_root = nullptr;
  clearReleasedRoot();
}

void DesignPickerPanel::doLayout(Renderer& renderer, float width, float height) {
  if (m_root == nullptr) {
    return;
  }
  m_root->setSize(width, height);
  m_root->layout(renderer);
  for (Button* card : m_cards) {
    card->updateInputArea();
  }
}

void DesignPickerPanel::select(std::size_t index) {
  const auto presets = noctalia::config::designPresets();
  if (m_config == nullptr || index >= presets.size()) {
    return;
  }
  (void)m_config->setDesign(presets[index].id);
  syncSelection();
}

void DesignPickerPanel::syncSelection() {
  const auto presets = noctalia::config::designPresets();
  const std::string_view active = m_config != nullptr ? std::string_view(m_config->config().design.id) : "slate";
  for (std::size_t index = 0; index < m_cards.size() && index < presets.size(); ++index) {
    m_cards[index]->setSelected(presets[index].id == active);
  }
}
