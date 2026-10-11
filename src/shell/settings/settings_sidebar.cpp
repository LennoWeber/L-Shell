#include "shell/settings/settings_sidebar.h"

#include "i18n/i18n.h"
#include "render/core/renderer.h"
#include "shell/settings/settings_registry.h"
#include "ui/builders.h"
#include "ui/controls/roving_list_nav.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "util/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace settings {
  namespace {

    // macOS Tahoe System Settings sidebar metrics.
    constexpr float kSidebarWidth = 214.0F;
    constexpr float kSidebarPadding = 8.0F;
    constexpr float kSidebarGap = 1.0F;
    constexpr float kSidebarGroupGap = 10.0F;
    constexpr float kPrimaryNavGlyphSize = 18.0F;
    constexpr float kPrimaryNavGap = 8.0F;
    constexpr float kPrimaryNavPaddingH = 6.0F;
    constexpr float kSidebarRowHeight = 30.0F;
    constexpr float kSidebarRowRadius = 8.0F;
    constexpr float kSidebarFontSize = 13.0F;
    constexpr float kSquircleSize = 22.0F;
    constexpr float kSquircleRadius = 6.0F;
    constexpr float kSquircleGlyphSize = 14.0F;

    // How a section looks in the sidebar: a white SF-Symbol-like glyph on a colored squircle, and which group
    // it sits in (groups are separated by a gap, like macOS's System Settings).
    struct SidebarStyle {
      std::string_view glyph;
      std::uint32_t color = 0xff8e8e93U;
      std::uint8_t group = 0;
    };

    SidebarStyle sidebarStyle(SettingsSection section) {
      switch (section) {
      case SettingsSection::Layout:
        return {"layout-dashboard-filled", 0xff0a84ffU, 0};
      case SettingsSection::Appearance:
        return {"circle-half-2", 0xff3a3a3cU, 0};
      case SettingsSection::Wallpaper:
        return {"photo-filled", 0xff32ade6U, 0};
      case SettingsSection::Templates:
        return {"palette-filled", 0xffbf5af2U, 0};
      case SettingsSection::Desktop:
        return {"layout-board-filled", 0xff5e5ce6U, 0};
      case SettingsSection::Bar:
        return {"layout-navbar-filled", 0xff3a3a3cU, 1};
      case SettingsSection::Dock:
        return {"layout-bottombar-filled", 0xff3a3a3cU, 1};
      case SettingsSection::Panels:
        return {"app-window-filled", 0xff64d2ffU, 1};
      case SettingsSection::ControlCenter:
        return {"adjustments-filled", 0xff8e8e93U, 1};
      case SettingsSection::Launcher:
        return {"search", 0xff8e8e93U, 1};
      case SettingsSection::Notifications:
        return {"bell-filled", 0xffff3b30U, 1};
      case SettingsSection::Osd:
        return {"message-circle-filled", 0xffff9500U, 1};
      case SettingsSection::Shell:
        return {"settings-filled", 0xff8e8e93U, 2};
      case SettingsSection::Keybinds:
        return {"keyboard-filled", 0xff8e8e93U, 2};
      case SettingsSection::Screenshot:
        return {"camera-filled", 0xff636366U, 2};
      case SettingsSection::Power:
        return {"battery-3-filled", 0xff30d158U, 2};
      case SettingsSection::System:
        return {"cpu", 0xff636366U, 2};
      case SettingsSection::Services:
        return {"stack-2-filled", 0xff34c759U, 2};
      case SettingsSection::Location:
        return {"location-filled", 0xff0a84ffU, 2};
      case SettingsSection::Calendar:
        return {"calendar-filled", 0xffff3b30U, 2};
      case SettingsSection::Security:
        return {"hand-stop", 0xff0a84ffU, 2};
      case SettingsSection::Hooks:
        return {"link", 0xffff9f0aU, 3};
      case SettingsSection::Plugins:
        return {"puzzle-filled", 0xffff9f0aU, 3};
      case SettingsSection::Niri:
      case SettingsSection::Umbriel:
        return {sectionGlyph(section), 0xff5e5ce6U, 3};
      }
      return {sectionGlyph(section), 0xff8e8e93U, 3};
    }

    [[nodiscard]] Color argbColor(std::uint32_t argb) {
      return rgba(
          static_cast<float>((argb >> 16U) & 0xffU) / 255.0F, static_cast<float>((argb >> 8U) & 0xffU) / 255.0F,
          static_cast<float>(argb & 0xffU) / 255.0F, 1.0F
      );
    }

    std::unique_ptr<Flex> makeSquircle(std::string_view glyph, std::uint32_t argb, float scale) {
      return ui::row(
          {
              .align = FlexAlign::Center,
              .justify = FlexJustify::Center,
              .fill = fixedColorSpec(argbColor(argb)),
              .radius = kSquircleRadius * scale,
              .width = kSquircleSize * scale,
              .height = kSquircleSize * scale,
          },
          ui::glyph({
              .glyph = std::string(glyph),
              .glyphSize = kSquircleGlyphSize * scale,
              .color = fixedColorSpec(rgba(1.0F, 1.0F, 1.0F, 1.0F)),
          })
      );
    }

    // Sidebar rows: no fill at rest, a faint wash on hover, the accent when selected.
    Button::ButtonPalette sidebarRowPalette(bool selected) {
      if (selected) {
        const Button::ButtonStateColors accent{
            .bg = colorSpecFromRole(ColorRole::Primary),
            .border = clearColorSpec(),
            .label = colorSpecFromRole(ColorRole::OnPrimary),
        };
        return Button::ButtonPalette{
            .borderWidth = 0.0F, .normal = accent, .hover = accent, .pressed = accent, .disabled = accent,
        };
      }
      return Button::ButtonPalette{
          .borderWidth = 0.0F,
          .normal = {.bg = clearColorSpec(), .border = clearColorSpec(), .label = colorSpecFromRole(ColorRole::OnSurface)},
          .hover = {.bg = colorSpecFromRole(ColorRole::OnSurface, 0.07F), .border = clearColorSpec(),
                    .label = colorSpecFromRole(ColorRole::OnSurface)},
          .pressed = {.bg = colorSpecFromRole(ColorRole::OnSurface, 0.12F), .border = clearColorSpec(),
                      .label = colorSpecFromRole(ColorRole::OnSurface)},
          .disabled = {.bg = clearColorSpec(), .border = clearColorSpec(),
                       .label = colorSpecFromRole(ColorRole::OnSurfaceVariant)},
      };
    }

    void addNavButton(RovingListNavHost& nav, std::unique_ptr<Button> button, std::function<void()> onClick) {
      Button* raw = button.get();
      nav.registerItem(raw, onClick);
      nav.addChild(std::move(button));
    }

    std::string normalizedConfigId(std::string_view text) { return StringUtils::trim(text); }

    bool isValidConfigId(std::string_view text) {
      const auto trimmed = StringUtils::trim(text);
      if (trimmed.empty()) {
        return false;
      }
      return std::ranges::all_of(trimmed, [](unsigned char c) { return std::isalnum(c) != 0 || c == '_' || c == '-'; });
    }

    bool barNameExists(const std::vector<std::string>& barNames, std::string_view name) {
      return std::ranges::contains(barNames, name);
    }

    std::string nextAvailableBarName(const std::vector<std::string>& barNames) {
      for (std::size_t i = 1;; ++i) {
        const std::string candidate = i == 1 ? "bar" : std::format("bar_{}", i);
        if (!barNameExists(barNames, candidate)) {
          return candidate;
        }
      }
    }

    void makeButtonLabelBold(Button& button) {
      if (button.label() != nullptr) {
        button.label()->setFontWeight(FontWeight::Bold);
      }
    }

    // Primary sidebar nav style: a section row with its colored squircle icon, macOS System Settings style.
    std::unique_ptr<Button> makePrimaryNavButton(
        const SidebarStyle& style, std::string text, float scale, bool selected, std::function<void()> onClick
    ) {
      auto button = ui::button({
          .text = std::move(text),
          .fontSize = kSidebarFontSize * scale,
          .contentAlign = ButtonContentAlign::Start,
          .variant = selected ? ButtonVariant::TabActive : ButtonVariant::Tab,
          .minHeight = kSidebarRowHeight * scale,
          .paddingV = 0.0F,
          .paddingH = kPrimaryNavPaddingH * scale,
          .gap = kPrimaryNavGap * scale,
          .radius = kSidebarRowRadius * scale,
          .onClick = std::move(onClick),
          .configure = [](Button& button) { button.setTabStop(false); },
      });
      button->setCustomPalette(sidebarRowPalette(selected));
      button->insertChildAt(0, makeSquircle(style.glyph, style.color, scale));
      return button;
    }

    // Secondary sidebar nav style: indented compact rows for bars and monitors.
    std::unique_ptr<Button> makeSecondaryNavButton(
        std::string_view glyph, std::string text, float scale, bool selected, std::function<void()> onClick
    ) {
      return ui::button({
          .text = std::move(text),
          .glyph = std::string(glyph),
          .fontSize = Style::fontSizeCaption * scale,
          .glyphSize = Style::fontSizeCaption * scale,
          .contentAlign = ButtonContentAlign::Start,
          .variant = selected ? ButtonVariant::TabActive : ButtonVariant::Tab,
          .minHeight = kSidebarRowHeight * scale,
          .paddingTop = 0.0F,
          .paddingRight = Style::spaceMd * scale,
          .paddingBottom = 0.0F,
          .paddingLeft = (kPrimaryNavPaddingH + kSquircleSize + kPrimaryNavGap) * scale,
          .gap = Style::spaceXs * scale,
          .radius = kSidebarRowRadius * scale,
          .onClick = std::move(onClick),
          .configure = [selected](Button& button) {
            button.setTabStop(false);
            button.setCustomPalette(sidebarRowPalette(selected));
          },
      });
    }

    std::unique_ptr<Button> makeCreateButton(std::string text, float scale, std::function<void()> onClick) {
      return ui::button({
          .text = std::move(text),
          .fontSize = Style::fontSizeCaption * scale,
          .variant = ButtonVariant::Default,
          .minHeight = Style::controlHeightSm * scale,
          .paddingV = Style::spaceXs * scale,
          .paddingH = Style::spaceSm * scale,
          .radius = Style::scaledRadiusSm(scale),
          .onClick = std::move(onClick),
      });
    }

    std::unique_ptr<Button> makeCreateCancelButton(float scale, std::function<void()> onClick) {
      return ui::button({
          .glyph = "close",
          .glyphSize = Style::fontSizeCaption * scale,
          .variant = ButtonVariant::Ghost,
          .minWidth = Style::controlHeightSm * scale,
          .minHeight = Style::controlHeightSm * scale,
          .padding = Style::spaceXs * scale,
          .radius = Style::scaledRadiusSm(scale),
          .onClick = std::move(onClick),
      });
    }

  } // namespace

  std::unique_ptr<Flex> buildSettingsSidebar(SettingsSidebarContext ctx) {
    const Config& cfg = ctx.config;
    std::vector<std::string> existingBarNames = ctx.availableBars;
    const std::string nextBarName = nextAvailableBarName(existingBarNames);

    auto* scroll = &ctx.contentScrollState;
    auto* selectedSection = &ctx.selectedSection;
    auto* selectedBarName = &ctx.selectedBarName;
    auto* selectedMonitorOverride = &ctx.selectedMonitorOverride;
    auto* creatingBarName = &ctx.creatingBarName;

    const auto clearTransientState = std::move(ctx.clearTransientState);
    const auto clearSearchQuery = std::move(ctx.clearSearchQuery);
    const auto requestRebuild = std::move(ctx.requestRebuild);
    const auto createBar = std::move(ctx.createBar);
    const auto openMonitorOverrideCreate = std::move(ctx.openMonitorOverrideCreate);
    const auto openDockMonitorOverrideCreate = std::move(ctx.openDockMonitorOverrideCreate);
    const float scale = ctx.scale;
    const bool searchActive = ctx.globalSearchActive;
    const bool showActiveTab = !searchActive;

    auto sidebarScroll = ui::scrollView({
        .state = &ctx.sidebarScrollState,
        .contentScale = scale,
        .scrollbarVisible = true,
        .viewportPaddingH = 0.0F,
        .viewportPaddingV = 0.0F,
        .fill = clearColorSpec(),
        .radius = Style::scaledRadiusXl(scale),
        .minWidth = kSidebarWidth * scale,
        .fillHeight = true,
        .width = kSidebarWidth * scale,
        .height = 0.0F,
        .configure = [](ScrollView& scrollView) { scrollView.clearBorder(); },
    });

    auto sidebarNav = std::make_unique<RovingListNavHost>(RovingListNavController::Options{
        .axis = RovingListNavAxis::Vertical,
        .mode = RovingListNavMode::FollowFocus,
        .keepItemsInTabOrder = false,
        .scrollIntoView = std::move(ctx.scrollSidebarNodeIntoView),
        .syncIndexFromSelection = {},
    });
    sidebarNav->setTabFocusKey("settings.sidebar");
    sidebarNav->setGap(kSidebarGap * scale);
    sidebarNav->setPadding(kSidebarPadding * scale);
    RovingListNavHost* nav = sidebarNav.get();

    std::vector<SettingsSection> orderedSections = ctx.sections;
    std::ranges::stable_sort(orderedSections, {}, [](SettingsSection section) { return sidebarStyle(section).group; });
    std::optional<std::uint8_t> previousGroup;
    for (const auto& section : orderedSections) {
      const SidebarStyle style = sidebarStyle(section);
      if (previousGroup.has_value() && *previousGroup != style.group) {
        nav->addChild(ui::row({.height = kSidebarGroupGap * scale}));
      }
      previousGroup = style.group;
      const std::string sectionId(settingsSectionId(section));
      const bool selected = showActiveTab
          && sectionId == *selectedSection
          && (section != SettingsSection::Dock || selectedMonitorOverride->empty());
      const auto onClick = [selectedSection, scroll, sectionId, searchActive, clearTransientState, clearSearchQuery,
                            requestRebuild, selectedMonitorOverride]() {
        if (searchActive || *selectedSection != sectionId) {
          scroll->offset = 0.0F;
        }
        *selectedSection = sectionId;
        selectedMonitorOverride->clear();
        clearSearchQuery();
        clearTransientState();
        requestRebuild();
      };
      addNavButton(
          *nav,
          makePrimaryNavButton(style, i18n::tr(settingsSectionLabelKey(section)), scale, selected, onClick),
          onClick
      );

      if (section != SettingsSection::Dock) {
        continue;
      }
      for (const auto& override : cfg.dock.monitorOverrides) {
        const bool overrideSelected =
            showActiveTab && *selectedSection == "dock" && *selectedMonitorOverride == override.tableName;
        const std::string tableName = override.tableName;
        const std::string label = override.match;
        const auto onMonitorClick = [selectedSection, selectedMonitorOverride, scroll, tableName, searchActive,
                                     clearTransientState, clearSearchQuery, requestRebuild]() {
          if (searchActive || *selectedSection != "dock" || *selectedMonitorOverride != tableName) {
            scroll->offset = 0.0F;
          }
          *selectedSection = "dock";
          *selectedMonitorOverride = tableName;
          clearSearchQuery();
          clearTransientState();
          requestRebuild();
        };
        addNavButton(
            *nav,
            makeSecondaryNavButton(
                "device-desktop", i18n::tr("settings.entities.monitor-override.label", "name", label), scale,
                overrideSelected, onMonitorClick
            ),
            onMonitorClick
        );
      }
      if (*selectedSection == "dock") {
        const auto onNewDockMonitorClick = [openDockMonitorOverrideCreate, clearTransientState]() {
          clearTransientState();
          if (openDockMonitorOverrideCreate) {
            openDockMonitorOverrideCreate();
          }
        };
        addNavButton(
            *nav,
            ui::button({
                .text = i18n::tr("settings.entities.monitor-override.new"),
                .glyph = "add",
                .fontSize = Style::fontSizeCaption * scale,
                .glyphSize = Style::fontSizeCaption * scale,
                .contentAlign = ButtonContentAlign::Start,
                .variant = ButtonVariant::Ghost,
                .minHeight = Style::controlHeightSm * scale,
                .paddingTop = Style::spaceXs * scale,
                .paddingRight = Style::spaceMd * scale,
                .paddingBottom = Style::spaceXs * scale,
                .paddingLeft = Style::spaceLg * scale,
                .gap = Style::spaceXs * scale,
                .radius = Style::scaledRadiusMd(scale),
                .onClick = onNewDockMonitorClick,
                .configure = [](Button& button) { button.setTabStop(false); },
            }),
            onNewDockMonitorClick
        );
      }
    }

    // TODO(phase): bar rows and "New bar" only in the classic layout mode.
    for (const auto& barName : ctx.availableBars) {
      const bool barSelected =
          showActiveTab && *selectedSection == "bar" && *selectedBarName == barName && selectedMonitorOverride->empty();
      const auto onBarClick = [selectedSection, selectedBarName, selectedMonitorOverride, scroll, barName, searchActive,
                               clearTransientState, clearSearchQuery, requestRebuild]() {
        if (searchActive
            || *selectedSection != "bar"
            || *selectedBarName != barName
            || !selectedMonitorOverride->empty()) {
          scroll->offset = 0.0F;
        }
        *selectedSection = "bar";
        *selectedBarName = barName;
        selectedMonitorOverride->clear();
        clearSearchQuery();
        clearTransientState();
        requestRebuild();
      };
      addNavButton(
          *nav,
          makePrimaryNavButton(
              sidebarStyle(SettingsSection::Bar), i18n::tr("settings.entities.bar.label", "name", barName), scale,
              barSelected, onBarClick
          ),
          onBarClick
      );

      const auto* bar = settings::findBar(cfg, barName);
      if (bar == nullptr) {
        continue;
      }

      for (const auto& ovr : bar->monitorOverrides) {
        const bool ovrSelected = showActiveTab
            && *selectedSection == "bar"
            && *selectedBarName == barName
            && *selectedMonitorOverride == ovr.match;
        auto match = ovr.match;
        const auto onMonitorClick = [selectedSection, selectedBarName, selectedMonitorOverride, scroll, barName, match,
                                     searchActive, clearTransientState, clearSearchQuery, requestRebuild]() {
          if (searchActive
              || *selectedSection != "bar"
              || *selectedBarName != barName
              || *selectedMonitorOverride != match) {
            scroll->offset = 0.0F;
          }
          *selectedSection = "bar";
          *selectedBarName = barName;
          *selectedMonitorOverride = match;
          clearSearchQuery();
          clearTransientState();
          requestRebuild();
        };
        addNavButton(
            *nav,
            makeSecondaryNavButton(
                "device-desktop", i18n::tr("settings.entities.monitor-override.label", "name", ovr.match), scale,
                ovrSelected, onMonitorClick
            ),
            onMonitorClick
        );
      }

      if (*selectedSection != "bar" || *selectedBarName != barName) {
        continue;
      }

      // Secondary sidebar action style: same compact indentation as monitor rows. Opens the create
      // flow in a modal (wide enough for the output picker) rather than expanding it inline here.
      const auto onNewMonitorClick = [openMonitorOverrideCreate, barName, clearTransientState]() {
        clearTransientState();
        if (openMonitorOverrideCreate) {
          openMonitorOverrideCreate(barName);
        }
      };
      addNavButton(
          *nav,
          ui::button({
              .text = i18n::tr("settings.entities.monitor-override.new"),
              .glyph = "add",
              .fontSize = Style::fontSizeCaption * scale,
              .glyphSize = Style::fontSizeCaption * scale,
              .contentAlign = ButtonContentAlign::Start,
              .variant = ButtonVariant::Ghost,
              .minHeight = Style::controlHeightSm * scale,
              .paddingTop = Style::spaceXs * scale,
              .paddingRight = Style::spaceMd * scale,
              .paddingBottom = Style::spaceXs * scale,
              .paddingLeft = Style::spaceLg * scale,
              .gap = Style::spaceXs * scale,
              .radius = Style::scaledRadiusMd(scale),
              .onClick = onNewMonitorClick,
              .configure = [](Button& button) { button.setTabStop(false); },
          }),
          onNewMonitorClick
      );
    }

    // Primary sidebar action style: same scale as top-level section rows.
    const auto onNewBarClick = [creatingBarName, nextBarName, clearTransientState, requestRebuild]() {
      clearTransientState();
      *creatingBarName = nextBarName;
      requestRebuild();
    };
    addNavButton(
        *nav,
        ui::button({
            .text = i18n::tr("settings.entities.bar.new"),
            .glyph = "add",
            .fontSize = Style::fontSizeCaption * scale,
            .glyphSize = kPrimaryNavGlyphSize * scale,
            .contentAlign = ButtonContentAlign::Start,
            .variant = ButtonVariant::Ghost,
            .minHeight = Style::controlHeightSm * scale,
            .paddingV = Style::spaceXs * scale,
            .paddingH = kPrimaryNavPaddingH * scale,
            .gap = kPrimaryNavGap * scale,
            .radius = Style::scaledRadiusMd(scale),
            .onClick = onNewBarClick,
            .configure =
                [](Button& button) {
                  makeButtonLabelBold(button);
                  button.setTabStop(false);
                },
        }),
        onNewBarClick
    );

    if (!creatingBarName->empty()) {
      auto createPanel = ui::column({
          .align = FlexAlign::Stretch,
          .gap = Style::spaceXs * scale,
          .configure = [scale](Flex& panel) { panel.setPadding(0.0F, Style::spaceXs * scale); },
      });

      Input* inputPtr = nullptr;
      auto input = ui::input({
          .out = &inputPtr,
          .value = *creatingBarName,
          .placeholder = i18n::tr("settings.entities.bar.id-placeholder"),
          .fontSize = Style::fontSizeCaption * scale,
          .controlHeight = Style::controlHeightSm * scale,
          .horizontalPadding = Style::spaceXs * scale,
          .width = 120.0F * scale,
          .height = Style::controlHeightSm * scale,
      });

      auto doCreate = [existingBarNames, createBar, inputPtr](std::string rawName) {
        const std::string name = normalizedConfigId(rawName);
        if (!isValidConfigId(name) || barNameExists(existingBarNames, name)) {
          inputPtr->setInvalid(true);
          return;
        }
        inputPtr->setInvalid(false);
        createBar(name);
      };

      inputPtr->setOnChange([creatingBarName, inputPtr](const std::string& value) {
        *creatingBarName = value;
        inputPtr->setInvalid(false);
      });
      inputPtr->setOnSubmit([doCreate](const std::string& text) mutable { doCreate(text); });

      createPanel->addChild(std::move(input));
      createPanel->addChild(
          ui::row(
              {
                  .align = FlexAlign::Center,
                  .gap = Style::spaceXs * scale,
              },
              makeCreateButton(
                  i18n::tr("settings.entities.bar.create"), scale,
                  [doCreate, inputPtr]() mutable { doCreate(inputPtr->value()); }
              ),
              makeCreateCancelButton(scale, [creatingBarName, requestRebuild]() {
                creatingBarName->clear();
                requestRebuild();
              })
          )
      );
      sidebarNav->addChild(std::move(createPanel));
    }

    auto* sidebar = sidebarScroll->content();
    sidebar->setDirection(FlexDirection::Vertical);
    sidebar->setAlign(FlexAlign::Stretch);
    sidebar->addChild(std::move(sidebarNav));

    if (ctx.outNav != nullptr) {
      *ctx.outNav = nav;
    }

    return sidebarScroll;
  }

} // namespace settings
