#include "shell/launcher/launcher_panel.h"

#include "config/config_service.h"
#include "core/deferred_call.h"
#include "core/input/key_modifiers.h"
#include "core/input/key_symbols.h"
#include "core/input/keybind_matcher.h"
#include "core/ui_phase.h"
#include "cursor-shape-v1-client-protocol.h"
#include "i18n/i18n.h"
#include "launcher/app_provider.h"
#include "render/core/async_texture_cache.h"
#include "render/core/renderer.h"
#include "render/scene/node.h"
#include "shell/dock/pinned_apps.h"
#include "shell/panel/panel_manager.h"
#include "shell/session/session_action_runner.h"
#include "shell/session/session_tiles.h"
#include "system/desktop_entry.h"
#include "ui/app_icon_colorization.h"
#include "ui/builders.h"
#include "ui/controls/button.h"
#include "ui/controls/context_menu_popup.h"
#include "ui/controls/scroll_view.h"
#include "ui/palette.h"
#include "ui/signal.h"
#include "ui/style.h"
#include "util/fuzzy_match.h"
#include "util/string_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <tuple>

namespace {

  constexpr std::size_t kRowOverscan = 3;
  // Minimum trimmed query length before prefixed opt-in providers join the global search.
  constexpr std::size_t kGlobalOptInMinChars = 2;
  constexpr float kIconSizeDefault = 28.0F;
  constexpr float kIconSizeCompact = 28.0F;
  constexpr std::size_t kAppGridColumns = 5;
  constexpr std::string_view kApplicationsProviderId = "Applications";
  constexpr double kUsageScorePerCount = 0.1;
  constexpr double kTypedUsageScoreCap = 0.5;
  constexpr std::string_view kProviderOverviewProviderId = "__launcher_provider_overview__";
  constexpr std::string_view kProviderOverviewResultPrefix = "provider:";
  // Pseudo-results that render as Spotlight-style group headers; never selectable.
  constexpr std::string_view kSectionHeaderProviderId = "__launcher_section__";
  constexpr float kSpotlightSearchHeight = 38.0F;
  constexpr float kSpotlightExpandedHeight = 360.0F;
  // Tahoe's Spotlight: a capsule while it is the search field alone, a generously rounded sheet with results.
  constexpr float kSpotlightExpandedRadius = 26.0F;
  // The command bar: a compact, square-cut palette. All sizes are before contentScale().
  constexpr float kCommandWidth = 620.0F;
  constexpr float kCommandRadius = 12.0F;
  constexpr float kCommandSearchHeight = 50.0F;
  constexpr float kCommandDividerThickness = 1.0F;
  constexpr float kCommandListPadding = 6.0F;
  constexpr float kCommandRowHeight = 34.0F;
  constexpr float kCommandRowGap = 2.0F;
  constexpr float kCommandRowRadius = 7.0F;
  constexpr float kCommandIconSize = 20.0F;
  constexpr float kCommandFooterHeight = 32.0F;
  constexpr float kCommandDetailHeight = 220.0F;
  constexpr std::size_t kCommandVisibleRows = 9;
  // The sheet (Monolith): a full-height column beside the rail. All sizes are before contentScale().
  constexpr float kSheetWidth = 380.0F;
  constexpr float kSheetPadding = 22.0F;
  constexpr float kSheetSearchFontSize = 18.0F;
  constexpr float kSheetSearchGlyphSize = 20.0F;
  constexpr float kSheetSearchHeight = 24.0F;
  constexpr float kSheetSearchGap = 10.0F;
  constexpr float kSheetUnderline = 2.0F;
  constexpr float kSheetRowHeight = 44.0F;
  constexpr float kSheetRowPaddingH = 10.0F;
  constexpr float kSheetRowGap = 12.0F;
  constexpr float kSheetIconSize = 20.0F;
  constexpr float kSheetTitleSize = 15.0F;
  constexpr float kSheetQuietTitleSize = 14.0F;
  constexpr float kSheetHeaderSize = 11.0F;
  constexpr float kSheetHeaderGap = 6.0F;
  constexpr float kSheetHintSize = 11.0F;
  constexpr float kSheetQuietMetaSize = 12.0F;
  constexpr float kSheetHeaderTracking = 0.12F;
  constexpr float kSheetHintTracking = 0.08F;
  constexpr float kSheetFallbackHeight = 600.0F;
  // Rows per section while nothing is typed: the most used apps, the latest files.
  constexpr std::size_t kSheetIdleRows = 6;

  [[nodiscard]] bool isSectionHeader(const LauncherResult& result) {
    return result.providerId == kSectionHeaderProviderId;
  }

  double usageBoostForScore(double score, int usageCount, bool typedQuery) {
    if (usageCount <= 0) {
      return 0.0;
    }

    const double rawBoost = static_cast<double>(usageCount) * kUsageScorePerCount;
    if (!typedQuery) {
      return rawBoost;
    }
    if (!FuzzyMatch::isMatch(score)) {
      return 0.0;
    }

    // For typed searches, usage should nudge close matches without letting a
    // weak fuzzy hit outrank a much stronger lexical match.
    return std::min(rawBoost, kTypedUsageScoreCap);
  }

  [[nodiscard]] bool isDescendantOf(const Node* node, const Node* ancestor) {
    if (node == nullptr || ancestor == nullptr) {
      return false;
    }
    for (const Node* current = node; current != nullptr; current = current->parent()) {
      if (current == ancestor) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::string singleLinePreview(std::string_view text) {
    std::string preview;
    preview.reserve(text.size());
    bool lastWasSpace = false;
    for (const char c : text) {
      const bool whitespace = c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v';
      if (whitespace) {
        if (!lastWasSpace) {
          preview.push_back(' ');
          lastWasSpace = true;
        }
        continue;
      }
      preview.push_back(c);
      lastWasSpace = c == ' ';
    }
    return preview;
  }

  [[nodiscard]] bool isDetailPresentation(const LauncherResult& result) { return result.presentation == "detail"; }

  [[nodiscard]] std::string providerOverviewId(std::string_view prefix) {
    std::string id(kProviderOverviewResultPrefix);
    id += prefix;
    return id;
  }

  void sortResultsByScore(std::vector<LauncherResult>& results) {
    std::ranges::stable_sort(results, std::ranges::greater{}, &LauncherResult::score);
  }

  // A key cap for the command bar's hints: glyphs or a short word on a hairline-framed chip.
  [[nodiscard]] std::unique_ptr<Flex>
  commandKeycap(std::initializer_list<std::string_view> glyphs, std::string_view text, float scale) {
    auto cap = ui::row({
        .align = FlexAlign::Center,
        .justify = FlexJustify::Center,
        .gap = 1.0F * scale,
        .paddingH = 4.0F * scale,
        .fill = colorSpecFromRole(ColorRole::OnSurface, 0.07F),
        .radius = 4.0F * scale,
        .border = colorSpecFromRole(ColorRole::Outline, 0.7F),
        .minWidth = 18.0F * scale,
        .minHeight = 18.0F * scale,
    });
    for (const std::string_view glyph : glyphs) {
      cap->addChild(
          ui::glyph({
              .glyph = std::string(glyph),
              .glyphSize = Style::fontSizeMini * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          })
      );
    }
    if (!text.empty()) {
      cap->addChild(
          ui::label({
              .text = std::string(text),
              .fontSize = Style::fontSizeMini * scale,
              .fontWeight = FontWeight::Medium,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          })
      );
    }
    return cap;
  }

  // A key cap followed by what the key does.
  [[nodiscard]] std::unique_ptr<Flex> commandKeyHint(std::unique_ptr<Flex> cap, std::string label, float scale) {
    auto hint = ui::row({.align = FlexAlign::Center, .gap = Style::spaceXs * scale + 1.0F * scale});
    hint->addChild(std::move(cap));
    if (!label.empty()) {
      hint->addChild(
          ui::label({
              .text = std::move(label),
              .fontSize = Style::fontSizeMini * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          })
      );
    }
    return hint;
  }

  struct LauncherListStyle {
    float scale = 1.0F;
    bool showIcons = true;
    bool showAppOriginIndicator = true;
    bool compact = false;
    bool command = false; // command-bar rows: one line, fixed height
    bool sheet = false;   // sheet rows: glyph and title, an accent fill when selected
    bool appGlyphs = false; // apps drawn as their category glyph, not their icon (shell.app_icon_glyphs)
    std::optional<ColorSpec> appIconColorizeTint;
    std::optional<ColorSpec> listItemBackground;
  };

  [[nodiscard]] float launcherIconSize(const LauncherListStyle& style) {
    return (style.compact ? kIconSizeCompact : kIconSizeDefault) * style.scale;
  }

  [[nodiscard]] float stableLabelHeight(const TextMetrics& metrics) { return std::round(metrics.bottom - metrics.top); }

  [[nodiscard]] float launcherTextStackHeight(Renderer& renderer, const LauncherListStyle& style) {
    const float bodySize = Style::fontSizeBody * style.scale;
    float textHeight = stableLabelHeight(renderer.measureFont(bodySize, FontWeight::SemiBold));
    if (!style.compact) {
      const float captionSize = Style::fontSizeCaption * style.scale;
      textHeight += stableLabelHeight(renderer.measureFont(captionSize, FontWeight::Normal));
    }
    return textHeight;
  }

  [[nodiscard]] float launcherRowHeight(Renderer& renderer, const LauncherListStyle& style) {
    if (style.sheet) {
      return std::round(kSheetRowHeight * style.scale);
    }
    if (style.command) {
      return std::round(kCommandRowHeight * style.scale);
    }
    const float paddingY = (style.compact ? Style::spaceXs * 0.5F : Style::spaceXs) * style.scale;
    const float textHeight = launcherTextStackHeight(renderer, style);
    if (!style.showIcons) {
      return std::ceil(textHeight + paddingY * 2.0F);
    }
    return std::ceil(std::max(launcherIconSize(style), textHeight) + paddingY * 2.0F);
  }

  [[nodiscard]] float launcherRowHeightEstimate(const LauncherListStyle& style) {
    if (style.sheet) {
      return std::round(kSheetRowHeight * style.scale);
    }
    if (style.command) {
      return std::round(kCommandRowHeight * style.scale);
    }
    const float paddingY = (style.compact ? Style::spaceXs * 0.5F : Style::spaceXs) * style.scale;
    const float bodySize = Style::fontSizeBody * style.scale;
    const float captionSize = Style::fontSizeCaption * style.scale;
    const float textHeight = bodySize + (style.compact ? 0.0F : captionSize);
    if (!style.showIcons) {
      return std::ceil(textHeight + paddingY * 2.0F);
    }
    return std::ceil(std::max(launcherIconSize(style), textHeight) + paddingY * 2.0F);
  }

  [[nodiscard]] float launcherAppGridLabelHeight(Renderer& renderer, const LauncherListStyle& style, float wrapWidth) {
    const float fontSize = Style::fontSizeCaption * style.scale;
    const TextMetrics metrics =
        renderer.measureText("Ag\nyg", fontSize, FontWeight::Normal, wrapWidth, 2, TextAlign::Center);
    const float actualHeight = metrics.bottom - metrics.top;
    const float inkSpan = std::max(0.0F, metrics.inkBottom - metrics.inkTop);
    const float rowExtent = renderer.fontRowExtent(fontSize, FontWeight::Normal);
    return std::ceil(std::max({actualHeight, inkSpan, rowExtent * 2.0F}));
  }

  [[nodiscard]] float launcherAppGridCellHeight(Renderer& renderer, const LauncherListStyle& style, float wrapWidth) {
    const float paddingY = Style::spaceSm * style.scale;
    const float gap = Style::spaceXs * style.scale;
    const float iconSize = launcherIconSize(style);
    const float labelHeight = launcherAppGridLabelHeight(renderer, style, wrapWidth);
    return std::ceil(paddingY * 2.0F + iconSize + gap + labelHeight);
  }

  [[nodiscard]] float launcherAppGridCellHeightEstimate(const LauncherListStyle& style) {
    const float paddingY = Style::spaceSm * style.scale;
    const float gap = Style::spaceXs * style.scale;
    const float iconSize = launcherIconSize(style);
    const float labelHeight = Style::fontSizeCaption * style.scale * 2.4F;
    return std::ceil(paddingY * 2.0F + iconSize + gap + labelHeight);
  }

  [[nodiscard]] LauncherListStyle
  launcherListStyleFrom(const ConfigService* config, float scale, float cardOpacity, bool command, bool sheet) {
    LauncherListStyle style{
        .scale = scale,
        .command = command,
        .sheet = sheet,
        .appIconColorizeTint = std::nullopt,
        .listItemBackground = std::nullopt,
    };
    if (config != nullptr) {
      style.appGlyphs = config->config().shell.appIconGlyphs;
      const auto& launcher = config->config().shell.launcher;
      style.showIcons = launcher.showIcons;
      style.showAppOriginIndicator = launcher.showAppOriginIndicator;
      style.compact = launcher.compact;
      style.appIconColorizeTint = effectiveShellAppIconColorizationTint(config->config().shell);
      if (config->config().shell.panel.listItemBackground) {
        style.listItemBackground = colorSpecFromRole(ColorRole::SurfaceVariant, cardOpacity);
      }
    }
    return style;
  }

  class LauncherResultRow final : public Node {
  public:
    LauncherResultRow(LauncherListStyle style, AsyncTextureCache* asyncTextures)
        : m_style(style), m_asyncTextures(asyncTextures) {
      const float iconSize = launcherIconSize(m_style);
      const float gap = (m_style.compact ? Style::spaceSm : Style::spaceMd) * m_style.scale;
      const float paddingV = (m_style.compact ? Style::spaceXs * 0.5F : Style::spaceXs) * m_style.scale;
      auto row = ui::row(
          {.out = &m_row,
           .align = FlexAlign::Center,
           .gap = gap,
           .paddingV = paddingV,
           .paddingH = Style::spaceSm * m_style.scale,
           .radius = Style::scaledRadiusMd(m_style.scale)}
      );
      addChild(std::move(row));

      m_row->addChild(
          ui::label({
              .out = &m_badgeLabel,
              .fontSize = iconSize,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .visible = false,
          })
      );

      m_row->addChild(
          ui::image({
              .out = &m_image,
              .width = iconSize,
              .height = iconSize,
              .visible = false,
          })
      );

      m_row->addChild(
          ui::glyph({
              .out = &m_glyph,
              .glyphSize = iconSize,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .visible = false,
          })
      );

      m_image->setAsyncReadyCallback([this]() {
        if (!m_style.showIcons
            || m_badgeVisible
            || m_iconPath.empty()
            || m_image == nullptr
            || m_glyph == nullptr
            || !m_image->hasImage()) {
          return;
        }
        m_image->setVisible(true);
        m_glyph->setVisible(false);
      });

      m_row->addChild(
          ui::column(
              {
                  .out = &m_textCol,
                  .align = FlexAlign::Start,
                  .gap = 0.0F,
                  .flexGrow = 1.0F,
              },
              ui::label({
                  .out = &m_title,
                  .fontSize = Style::fontSizeBody * m_style.scale,
                  .fontWeight = FontWeight::SemiBold,
                  .color = colorSpecFromRole(ColorRole::OnSurface),
                  .maxLines = 1,
                  .baselineMode = LabelBaselineMode::TextFixedHeight,
              }),
              ui::label({
                  .out = &m_subtitle,
                  .fontSize = Style::fontSizeCaption * m_style.scale,
                  .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
                  .maxLines = 1,
                  .baselineMode = LabelBaselineMode::TextFixedHeight,
              })
          )
      );

      m_row->addChild(
          ui::glyph({
              .out = &m_originGlyph,
              .glyph = "package",
              .glyphSize = Style::fontSizeBody * m_style.scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .visible = false,
          })
      );

      m_row->addChild(
          ui::glyph({
              .out = &m_pinnedGlyph,
              .glyph = "pin-filled",
              .glyphSize = Style::fontSizeBody * m_style.scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .visible = false,
          })
      );
    }

    void setListStyle(LauncherListStyle style) { m_style = style; }
    void setReorderTarget(bool target) { m_reorderTarget = target; }

    void
    bind(Renderer& renderer, const LauncherResult& result, float width, float height, bool selected, bool hovered) {
      if (isSectionHeader(result)) {
        bindHeader(renderer, result, width, height);
        return;
      }
      m_title->setFontSize(Style::fontSizeBody * m_style.scale);
      m_selected = selected;
      m_hovered = hovered;
      m_iconPath = m_style.appGlyphs ? std::string{} : result.iconPath;
      m_fallbackGlyph = result.glyphName.empty() ? "app-window" : result.glyphName;
      const float iconSize = launcherIconSize(m_style);
      m_iconTargetSize = static_cast<int>(std::round(iconSize));
      m_badgeVisible = !result.badge.empty();
      m_rowHeight = height;

      setSize(width, height);
      m_row->setFrameSize(width, height);

      m_badgeLabel->setVisible(false);
      m_badgeLabel->setParticipatesInLayout(false);
      m_image->setVisible(false);
      m_image->setParticipatesInLayout(false);
      m_glyph->setVisible(false);
      m_glyph->setParticipatesInLayout(false);

      const bool showAppIcon = m_style.showIcons && !m_badgeVisible;
      const bool showLeadingVisual = m_badgeVisible || showAppIcon;
      if (m_badgeVisible) {
        m_badgeLabel->setText(singleLinePreview(result.badge));
        m_badgeLabel->setSize(iconSize, iconSize);
        m_badgeLabel->setVisible(true);
        m_badgeLabel->setParticipatesInLayout(true);
        m_image->clear(renderer);
      } else if (showAppIcon) {
        m_image->setParticipatesInLayout(true);
        m_glyph->setParticipatesInLayout(true);
        if (!m_iconPath.empty()) {
          const bool ready = refreshAsyncIcon(renderer);
          m_image->setVisible(ready);
          m_glyph->setGlyph(m_fallbackGlyph);
          m_glyph->setVisible(!ready);
        } else {
          m_image->clear(renderer);
          m_glyph->setGlyph(m_fallbackGlyph);
          m_glyph->setVisible(true);
        }
      } else {
        m_image->clear(renderer);
      }

      const float gap = (m_style.compact ? Style::spaceSm : Style::spaceMd) * m_style.scale;
      const float horizontalPad = Style::spaceSm * m_style.scale * 2.0F;
      const float leadingWidth = showLeadingVisual ? iconSize + gap : 0.0F;
      const float pinnedWidth = result.pinned ? Style::fontSizeBody * m_style.scale + gap : 0.0F;
      m_pinnedGlyph->setGlyphSize(Style::fontSizeBody * m_style.scale);
      m_pinnedGlyph->setVisible(result.pinned);
      m_pinnedGlyph->setParticipatesInLayout(result.pinned);
      const bool hasOrigin = m_style.showAppOriginIndicator && !result.originGlyph.empty();
      if (hasOrigin) {
        m_originGlyph->setGlyph(result.originGlyph);
      }
      m_originGlyph->setGlyphSize(Style::fontSizeBody * m_style.scale);
      m_originGlyph->setVisible(hasOrigin);
      m_originGlyph->setParticipatesInLayout(hasOrigin);
      const float originWidth = hasOrigin ? Style::fontSizeBody * m_style.scale + gap : 0.0F;
      const float textWidth = std::max(0.0F, width - leadingWidth - pinnedWidth - originWidth - horizontalPad);
      m_title->setText(singleLinePreview(result.title));
      m_title->setMaxWidth(textWidth);

      const bool showSubtitle = !m_style.compact && !result.subtitle.empty();
      if (!showSubtitle) {
        m_subtitle->setVisible(false);
        m_subtitle->setText("");
      } else {
        m_subtitle->setVisible(true);
        m_subtitle->setText(singleLinePreview(result.subtitle));
        m_subtitle->setMaxWidth(textWidth);
      }

      applyVisualState();
    }

    bool refreshAsyncIcon(Renderer& renderer) {
      if (!m_style.showIcons || m_badgeVisible || m_iconPath.empty()) {
        m_image->setVisible(false);
        m_glyph->setVisible(false);
        return false;
      }

      m_image->setAppIconColorization(m_style.appIconColorizeTint);

      bool ready = false;
      if (m_asyncTextures != nullptr) {
        ready = m_image->setSourceFileAsync(renderer, *m_asyncTextures, m_iconPath, m_iconTargetSize, true);
      } else {
        ready = m_image->setSourceFile(renderer, m_iconPath, m_iconTargetSize, true);
      }

      m_image->setSize(launcherIconSize(m_style), launcherIconSize(m_style));
      m_image->setVisible(ready);
      m_glyph->setGlyph(m_fallbackGlyph);
      m_glyph->setVisible(!ready);
      return ready;
    }

  protected:
    void doLayout(Renderer& renderer) override {
      if (m_style.showIcons && !m_badgeVisible && !m_iconPath.empty()) {
        (void)refreshAsyncIcon(renderer);
      }
      Node::doLayout(renderer);
    }

  private:
    void bindHeader(Renderer& renderer, const LauncherResult& result, float width, float height) {
      m_selected = false;
      m_hovered = false;
      m_iconPath.clear();
      m_badgeVisible = false;
      m_rowHeight = height;
      setSize(width, height);
      m_row->setFrameSize(width, height);
      m_image->clear(renderer);
      for (Node* node : std::initializer_list<Node*>{m_badgeLabel, m_image, m_glyph, m_pinnedGlyph, m_originGlyph}) {
        node->setVisible(false);
        node->setParticipatesInLayout(false);
      }
      m_subtitle->setVisible(false);
      m_subtitle->setText("");
      m_title->setText(result.title);
      m_title->setFontSize(Style::fontSizeCaption * m_style.scale);
      m_title->setColor(colorSpecFromRole(ColorRole::OnSurfaceVariant));
      m_title->setMaxWidth(std::max(0.0F, width - Style::spaceSm * m_style.scale * 2.0F));
      m_row->setFill(clearColorSpec());
      m_row->clearBorder();
    }

    void applyVisualState() {
      if (m_selected) {
        m_row->setFill(colorSpecFromRole(ColorRole::Primary));
      } else if (m_hovered) {
        m_row->setFill(colorSpecFromRole(ColorRole::Hover));
      } else {
        m_row->setFill(m_style.listItemBackground.value_or(clearColorSpec()));
      }
      if (m_reorderTarget) {
        m_row->setBorder(colorSpecFromRole(ColorRole::Primary), Style::focusRingWidth);
      } else {
        m_row->clearBorder();
      }

      const auto activeRole = m_selected ? ColorRole::OnPrimary : ColorRole::OnHover;
      const bool active = m_selected || m_hovered;
      const ColorSpec foreground = colorSpecFromRole(active ? activeRole : ColorRole::OnSurface);
      const ColorSpec mutedForeground =
          active ? colorSpecFromRole(activeRole, 0.7F) : colorSpecFromRole(ColorRole::OnSurfaceVariant);
      m_badgeLabel->setColor(foreground);
      m_glyph->setColor(foreground);
      m_title->setColor(foreground);
      m_subtitle->setColor(mutedForeground);
      m_pinnedGlyph->setColor(mutedForeground);
      m_originGlyph->setColor(mutedForeground);
    }

    LauncherListStyle m_style{};
    float m_rowHeight = 0.0F;
    bool m_selected = false;
    bool m_hovered = false;
    Flex* m_row = nullptr;
    Label* m_badgeLabel = nullptr;
    Image* m_image = nullptr;
    Glyph* m_glyph = nullptr;
    Flex* m_textCol = nullptr;
    Label* m_title = nullptr;
    Label* m_subtitle = nullptr;
    Glyph* m_pinnedGlyph = nullptr;
    Glyph* m_originGlyph = nullptr;
    AsyncTextureCache* m_asyncTextures = nullptr;
    std::string m_iconPath;
    std::string m_fallbackGlyph;
    int m_iconTargetSize = 0;
    bool m_badgeVisible = false;
    bool m_reorderTarget = false;
  };

  // Command-bar result: icon, title and subtitle on one line, the result's kind as a quiet tag on the right. The
  // selection is a neutral wash with a short accent bar at its leading edge, so the accent stays a highlight.
  class LauncherCommandRow final : public Node {
  public:
    LauncherCommandRow(LauncherListStyle style, AsyncTextureCache* asyncTextures)
        : m_style(style), m_asyncTextures(asyncTextures) {
      const float scale = m_style.scale;
      const float iconSize = kCommandIconSize * scale;
      addChild(ui::box({.out = &m_background, .radius = kCommandRowRadius * scale}));
      addChild(
          ui::box({
              .out = &m_marker,
              .fill = colorSpecFromRole(ColorRole::Primary),
              .radius = 1.5F * scale,
              .visible = false,
          })
      );
      addChild(
          ui::label({
              .out = &m_badgeLabel,
              .fontSize = iconSize * 0.85F,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .maxLines = 1,
              .visible = false,
          })
      );
      addChild(ui::image({.out = &m_image, .width = iconSize, .height = iconSize, .visible = false}));
      addChild(
          ui::glyph({
              .out = &m_glyph,
              .glyphSize = iconSize * 0.9F,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .visible = false,
          })
      );
      m_image->setAsyncReadyCallback([this]() {
        if (m_iconPath.empty() || m_image == nullptr || m_glyph == nullptr || !m_image->hasImage()) {
          return;
        }
        m_image->setVisible(true);
        m_glyph->setVisible(false);
      });
      addChild(
          ui::label({
              .out = &m_title,
              .fontSize = Style::fontSizeBody * scale,
              .fontWeight = FontWeight::Medium,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .maxLines = 1,
          })
      );
      addChild(
          ui::label({
              .out = &m_subtitle,
              .fontSize = Style::fontSizeCaption * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .maxLines = 1,
          })
      );
      for (Glyph** out : {&m_originGlyph, &m_pinnedGlyph}) {
        addChild(
            ui::glyph({
                .out = out,
                .glyphSize = Style::fontSizeCaption * scale,
                .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
                .visible = false,
            })
        );
      }
      m_pinnedGlyph->setGlyph("pin-filled");
      addChild(
          ui::box({
              .out = &m_keyBox,
              .fill = colorSpecFromRole(ColorRole::OnSurface, 0.07F),
              .border = colorSpecFromRole(ColorRole::Outline, 0.7F),
              .borderWidth = 1.0F,
              .radius = 4.0F * scale,
              .visible = false,
          })
      );
      addChild(
          ui::label({
              .out = &m_tag,
              .fontSize = Style::fontSizeMini * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .maxLines = 1,
          })
      );
    }

    void setListStyle(LauncherListStyle style) { m_style = style; }

    // `tag` is the result's kind; `keyTag` draws it as a key cap instead (a command's trigger, like "/calc").
    void bind(
        Renderer& renderer, const LauncherResult& result, std::string_view tag, bool keyTag, float width, float height,
        bool selected, bool hovered
    ) {
      const float scale = m_style.scale;
      const float iconSize = kCommandIconSize * scale;
      const float padding = Style::spaceSm * scale + 2.0F * scale;
      const float gap = Style::spaceSm * scale + 2.0F * scale;
      m_selected = selected;
      m_hovered = hovered;
      setSize(width, height);
      m_background->setPosition(0.0F, 0.0F);
      m_background->setSize(width, height);
      const float markerHeight = std::round(height * 0.46F);
      m_marker->setPosition(0.0F, std::round((height - markerHeight) * 0.5F));
      m_marker->setSize(3.0F * scale, markerHeight);

      // Leading visual: an emoji or symbol badge, the app icon, or the provider's glyph.
      m_iconPath = m_style.appGlyphs ? std::string{} : result.iconPath;
      m_iconTargetSize = static_cast<int>(std::round(iconSize));
      const bool badge = !result.badge.empty();
      const bool showIcon = m_style.showIcons && !badge;
      m_badgeLabel->setVisible(badge);
      m_image->setVisible(false);
      m_glyph->setVisible(false);
      if (badge) {
        m_badgeLabel->setText(singleLinePreview(result.badge));
        m_badgeLabel->measure(renderer);
        m_badgeLabel->setPosition(
            padding + std::round((iconSize - m_badgeLabel->width()) * 0.5F),
            std::round((height - m_badgeLabel->height()) * 0.5F)
        );
        m_image->clear(renderer);
      } else if (showIcon) {
        const float iconY = std::round((height - iconSize) * 0.5F);
        m_image->setPosition(padding, iconY);
        m_image->setSize(iconSize, iconSize);
        m_glyph->setGlyph(result.glyphName.empty() ? "app-window" : result.glyphName);
        m_glyph->setGlyphSize(iconSize * 0.9F);
        m_glyph->measure(renderer);
        m_glyph->setPosition(
            padding + std::round((iconSize - m_glyph->width()) * 0.5F), std::round((height - m_glyph->height()) * 0.5F)
        );
        const bool ready = !m_iconPath.empty() && refreshAsyncIcon(renderer);
        m_image->setVisible(ready);
        m_glyph->setVisible(!ready);
      } else {
        m_image->clear(renderer);
      }
      float x = padding + (badge || showIcon ? iconSize + gap : 0.0F);

      // Trailing: tag, then the pinned and origin marks to its left.
      float right = width - padding;
      m_tag->setText(std::string(tag));
      m_tag->setMaxWidth(width * 0.3F);
      m_tag->setFontWeight(keyTag ? FontWeight::Medium : FontWeight::Normal);
      m_tag->measure(renderer);
      m_tag->setVisible(!tag.empty());
      m_keyBox->setVisible(keyTag && !tag.empty());
      if (!tag.empty()) {
        const float keyPadding = keyTag ? 5.0F * scale : 0.0F;
        right -= m_tag->width() + keyPadding;
        const float tagY = std::round((height - m_tag->height()) * 0.5F);
        m_tag->setPosition(std::round(right), tagY);
        if (keyTag) {
          const float keyHeight = std::round(18.0F * scale);
          m_keyBox->setPosition(std::round(right - keyPadding), std::round((height - keyHeight) * 0.5F));
          m_keyBox->setSize(std::round(m_tag->width() + keyPadding * 2.0F), keyHeight);
          right -= keyPadding;
        }
        right -= gap;
      }
      const auto placeMark = [&](Glyph* mark, bool visible) {
        mark->setVisible(visible);
        if (!visible) {
          return;
        }
        mark->measure(renderer);
        right -= mark->width();
        mark->setPosition(std::round(right), std::round((height - mark->height()) * 0.5F));
        right -= Style::spaceXs * scale + 2.0F * scale;
      };
      placeMark(m_pinnedGlyph, result.pinned);
      const bool hasOrigin = m_style.showAppOriginIndicator && !result.originGlyph.empty();
      if (hasOrigin) {
        m_originGlyph->setGlyph(result.originGlyph);
      }
      placeMark(m_originGlyph, hasOrigin);

      // Title, then as much of the subtitle as fits on the same line.
      const float textRight = std::max(x, right - Style::spaceXs * scale);
      m_title->setText(singleLinePreview(result.title));
      m_title->setMaxWidth(std::max(0.0F, textRight - x));
      m_title->measure(renderer);
      m_title->setPosition(x, std::round((height - m_title->height()) * 0.5F));
      const float subtitleX = x + m_title->width() + Style::spaceSm * scale;
      const float subtitleWidth = textRight - subtitleX;
      const bool showSubtitle = !keyTag && !result.subtitle.empty() && subtitleWidth >= 40.0F * scale;
      m_subtitle->setVisible(showSubtitle);
      if (showSubtitle) {
        m_subtitle->setText(singleLinePreview(result.subtitle));
        m_subtitle->setMaxWidth(subtitleWidth);
        m_subtitle->measure(renderer);
        // Share the title's baseline: both labels are centred, so drop the smaller one by half the size difference.
        const float baselineShift = (Style::fontSizeBody - Style::fontSizeCaption) * scale * 0.35F;
        m_subtitle->setPosition(
            std::round(subtitleX), std::round((height - m_subtitle->height()) * 0.5F + baselineShift)
        );
      }

      applyVisualState();
    }

  protected:
    void doLayout(Renderer& renderer) override {
      if (m_style.showIcons && !m_iconPath.empty()) {
        (void)refreshAsyncIcon(renderer);
      }
      Node::doLayout(renderer);
    }

  private:
    bool refreshAsyncIcon(Renderer& renderer) {
      m_image->setAppIconColorization(m_style.appIconColorizeTint);
      bool ready = false;
      if (m_asyncTextures != nullptr) {
        ready = m_image->setSourceFileAsync(renderer, *m_asyncTextures, m_iconPath, m_iconTargetSize, true);
      } else {
        ready = m_image->setSourceFile(renderer, m_iconPath, m_iconTargetSize, true);
      }
      m_image->setVisible(ready);
      m_glyph->setVisible(!ready);
      return ready;
    }

    void applyVisualState() {
      if (m_selected) {
        m_background->setFill(colorSpecFromRole(ColorRole::OnSurface, 0.11F));
      } else if (m_hovered) {
        m_background->setFill(colorSpecFromRole(ColorRole::OnSurface, 0.06F));
      } else {
        m_background->setFill(m_style.listItemBackground.value_or(clearColorSpec()));
      }
      m_marker->setVisible(m_selected);
      const ColorSpec muted = colorSpecFromRole(ColorRole::OnSurfaceVariant, m_selected ? 1.0F : 0.85F);
      m_subtitle->setColor(muted);
      m_tag->setColor(muted);
      m_originGlyph->setColor(muted);
      m_pinnedGlyph->setColor(muted);
    }

    LauncherListStyle m_style{};
    AsyncTextureCache* m_asyncTextures = nullptr;
    Box* m_background = nullptr;
    Box* m_marker = nullptr;
    Label* m_badgeLabel = nullptr;
    Image* m_image = nullptr;
    Glyph* m_glyph = nullptr;
    Label* m_title = nullptr;
    Label* m_subtitle = nullptr;
    Glyph* m_originGlyph = nullptr;
    Glyph* m_pinnedGlyph = nullptr;
    Box* m_keyBox = nullptr;
    Label* m_tag = nullptr;
    std::string m_iconPath;
    int m_iconTargetSize = 0;
    bool m_selected = false;
    bool m_hovered = false;
  };

  class LauncherAppGridTile final : public Node {
  public:
    LauncherAppGridTile(LauncherListStyle style, AsyncTextureCache* asyncTextures)
        : m_style(style), m_asyncTextures(asyncTextures) {
      const float gap = Style::spaceXs * m_style.scale;
      const float padding = Style::spaceSm * m_style.scale;
      auto col = ui::column({
          .out = &m_col,
          .align = FlexAlign::Center,
          .gap = gap,
          .paddingV = padding,
          .paddingH = padding,
          .radius = Style::scaledRadiusMd(m_style.scale),
          .fillWidth = true,
          .fillHeight = true,
      });
      addChild(std::move(col));

      addChild(
          ui::glyph({
              .out = &m_pinnedGlyph,
              .glyph = "pin-filled",
              .glyphSize = Style::fontSizeBody * m_style.scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .visible = false,
              .participatesInLayout = false,
          })
      );

      addChild(
          ui::glyph({
              .out = &m_originGlyph,
              .glyph = "package",
              .glyphSize = Style::fontSizeBody * m_style.scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .visible = false,
              .participatesInLayout = false,
          })
      );

      m_col->addChild(
          ui::image({
              .out = &m_image,
              .visible = false,
          })
      );

      m_col->addChild(
          ui::glyph({
              .out = &m_glyph,
              .glyphSize = launcherIconSize(m_style),
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .visible = false,
          })
      );

      m_image->setAsyncReadyCallback([this]() {
        if (!m_style.showIcons
            || m_iconPath.empty()
            || m_image == nullptr
            || m_glyph == nullptr
            || !m_image->hasImage()) {
          return;
        }
        m_image->setVisible(true);
        m_glyph->setVisible(false);
      });

      m_col->addChild(
          ui::label({
              .out = &m_title,
              .fontSize = Style::fontSizeCaption * m_style.scale,
              .fontWeight = FontWeight::Normal,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .maxLines = 2,
              .configure = [](Label& label) { label.setTextAlign(TextAlign::Center); },
          })
      );
    }

    void setListStyle(LauncherListStyle style) { m_style = style; }
    void setReorderTarget(bool target) { m_reorderTarget = target; }

    void
    bind(Renderer& renderer, const LauncherResult& result, float width, float height, bool selected, bool hovered) {
      m_selected = selected;
      m_hovered = hovered;
      m_iconPath = m_style.appGlyphs ? std::string{} : result.iconPath;
      m_fallbackGlyph = result.glyphName.empty() ? "app-window" : result.glyphName;
      const float iconSize = launcherIconSize(m_style);
      m_iconTargetSize = static_cast<int>(std::round(iconSize));

      setSize(width, height);
      m_col->setSize(width, height);

      const float pinSize = Style::fontSizeBody * m_style.scale;
      const float padding = Style::spaceSm * m_style.scale;
      m_pinnedGlyph->setGlyphSize(pinSize);
      m_pinnedGlyph->setVisible(result.pinned);
      m_pinnedGlyph->setPosition(Style::rtl() ? padding : width - padding - pinSize, padding);
      m_pinnedGlyph->setFrameSize(pinSize, pinSize);
      const bool hasOrigin = m_style.showAppOriginIndicator && !result.originGlyph.empty();
      if (hasOrigin) {
        m_originGlyph->setGlyph(result.originGlyph);
      }
      m_originGlyph->setGlyphSize(pinSize);
      m_originGlyph->setVisible(hasOrigin);
      m_originGlyph->setPosition(Style::rtl() ? padding : width - padding - pinSize, height - padding - pinSize);
      m_originGlyph->setFrameSize(pinSize, pinSize);

      m_image->setVisible(false);
      m_image->setParticipatesInLayout(false);
      m_glyph->setVisible(false);
      m_glyph->setParticipatesInLayout(false);

      if (m_style.showIcons) {
        m_image->setParticipatesInLayout(true);
        m_glyph->setParticipatesInLayout(true);
        m_image->setSize(iconSize, iconSize);
        m_glyph->setGlyphSize(iconSize);
        if (!m_iconPath.empty()) {
          const bool ready = refreshAsyncIcon(renderer);
          m_image->setVisible(ready);
          m_glyph->setGlyph(m_fallbackGlyph);
          m_glyph->setVisible(!ready);
        } else {
          m_image->clear(renderer);
          m_glyph->setGlyph(m_fallbackGlyph);
          m_glyph->setVisible(true);
        }
      } else {
        m_image->clear(renderer);
      }

      const float horizontalPad = Style::spaceSm * m_style.scale * 2.0F;
      const float textWidth = std::max(0.0F, width - horizontalPad);
      m_title->setText(singleLinePreview(result.title));
      m_title->setMaxWidth(textWidth);

      applyVisualState();
    }

    bool refreshAsyncIcon(Renderer& renderer) {
      if (!m_style.showIcons || m_iconPath.empty()) {
        m_image->setVisible(false);
        m_glyph->setVisible(false);
        return false;
      }

      m_image->setAppIconColorization(m_style.appIconColorizeTint);

      bool ready = false;
      if (m_asyncTextures != nullptr) {
        ready = m_image->setSourceFileAsync(renderer, *m_asyncTextures, m_iconPath, m_iconTargetSize, true);
      } else {
        ready = m_image->setSourceFile(renderer, m_iconPath, m_iconTargetSize, true);
      }

      const float iconSize = launcherIconSize(m_style);
      m_image->setSize(iconSize, iconSize);
      m_image->setVisible(ready);
      m_glyph->setGlyph(m_fallbackGlyph);
      m_glyph->setVisible(!ready);
      return ready;
    }

  protected:
    void doLayout(Renderer& renderer) override {
      m_col->setSize(width(), height());
      if (m_style.showIcons && !m_iconPath.empty()) {
        (void)refreshAsyncIcon(renderer);
      }
      Node::doLayout(renderer);
    }

  private:
    void applyVisualState() {
      if (m_selected) {
        m_col->setFill(colorSpecFromRole(ColorRole::Primary));
      } else if (m_hovered) {
        m_col->setFill(colorSpecFromRole(ColorRole::Hover));
      } else {
        m_col->setFill(m_style.listItemBackground.value_or(clearColorSpec()));
      }
      if (m_reorderTarget) {
        m_col->setBorder(colorSpecFromRole(ColorRole::Primary), Style::focusRingWidth);
      } else {
        m_col->clearBorder();
      }

      const auto activeRole = m_selected ? ColorRole::OnPrimary : ColorRole::OnHover;
      const bool active = m_selected || m_hovered;
      const ColorSpec foreground = colorSpecFromRole(active ? activeRole : ColorRole::OnSurface);
      m_glyph->setColor(foreground);
      m_title->setColor(foreground);
      m_pinnedGlyph->setColor(
          active ? colorSpecFromRole(activeRole, 0.7F) : colorSpecFromRole(ColorRole::OnSurfaceVariant)
      );
      m_originGlyph->setColor(
          active ? colorSpecFromRole(activeRole, 0.7F) : colorSpecFromRole(ColorRole::OnSurfaceVariant)
      );
    }

    LauncherListStyle m_style{};
    bool m_selected = false;
    bool m_hovered = false;
    Flex* m_col = nullptr;
    Image* m_image = nullptr;
    Glyph* m_glyph = nullptr;
    Glyph* m_pinnedGlyph = nullptr;
    Glyph* m_originGlyph = nullptr;
    Label* m_title = nullptr;
    AsyncTextureCache* m_asyncTextures = nullptr;
    std::string m_iconPath;
    std::string m_fallbackGlyph;
    int m_iconTargetSize = 0;
    bool m_reorderTarget = false;
  };

  // Sheet row (Monolith): the result's glyph and title, and while selected a full accent fill with the Enter key
  // named at its end. Quiet results (recent files) are a step smaller and muted, with their age at the end. A
  // section header is a small versal title resting on the row below it.
  class LauncherSheetRow final : public Node {
  public:
    explicit LauncherSheetRow(LauncherListStyle style) : m_style(style) {
      addChild(ui::box({.out = &m_background}));
      addChild(ui::glyph({.out = &m_glyph, .glyphSize = kSheetIconSize * style.scale}));
      addChild(ui::label({.out = &m_title, .maxLines = 1}));
      addChild(ui::label({.out = &m_trailing, .maxLines = 1}));
    }

    void setListStyle(LauncherListStyle style) { m_style = style; }

    void
    bind(Renderer& renderer, const LauncherResult& result, float width, float height, bool selected, bool hovered) {
      const float scale = m_style.scale;
      setSize(width, height);
      m_background->setPosition(0.0F, 0.0F);
      m_background->setSize(width, height);

      if (isSectionHeader(result)) {
        m_background->setFill(clearColorSpec());
        m_glyph->setVisible(false);
        m_trailing->setVisible(false);
        m_title->setText(i18n::upper(result.title));
        m_title->setFontSize(kSheetHeaderSize * scale);
        m_title->setFontWeight(FontWeight::Normal);
        m_title->setLetterSpacing(kSheetHeaderTracking);
        m_title->setColor(colorSpecFromRole(ColorRole::OnSurfaceVariant));
        m_title->setMaxWidth(width);
        m_title->measure(renderer);
        m_title->setPosition(0.0F, std::round(height - kSheetHeaderGap * scale - m_title->height()));
        return;
      }

      const bool quiet = result.presentation == "quiet";
      const ColorSpec ink = selected ? colorSpecFromRole(ColorRole::OnPrimary)
          : quiet                    ? colorSpecFromRole(ColorRole::OnSurface, 0.75F)
                                     : colorSpecFromRole(ColorRole::OnSurface);
      m_background->setFill(
          selected      ? colorSpecFromRole(ColorRole::Primary)
              : hovered ? colorSpecFromRole(ColorRole::OnSurface, 0.06F)
                        : clearColorSpec()
      );

      const float padding = kSheetRowPaddingH * scale;
      const float iconSize = kSheetIconSize * scale;
      m_glyph->setVisible(true);
      m_glyph->setGlyph(result.glyphName.empty() ? "app-window" : result.glyphName);
      m_glyph->setGlyphSize(iconSize);
      m_glyph->setColor(ink);
      m_glyph->measure(renderer);
      m_glyph->setPosition(
          padding + std::round((iconSize - m_glyph->width()) * 0.5F), std::round((height - m_glyph->height()) * 0.5F)
      );

      // Trailing: the Enter key on the selection, a quiet result's note otherwise.
      float right = width - padding;
      const std::string trailing = selected ? i18n::upper(i18n::tr("launcher.sheet.enter")) : result.trailing;
      m_trailing->setVisible(!trailing.empty());
      if (!trailing.empty()) {
        m_trailing->setText(trailing);
        m_trailing->setFontSize((selected ? kSheetHintSize : kSheetQuietMetaSize) * scale);
        m_trailing->setLetterSpacing(selected ? kSheetHintTracking : 0.0F);
        m_trailing->setColor(selected ? ink : colorSpecFromRole(ColorRole::OnSurfaceVariant));
        m_trailing->setMaxWidth(width * 0.4F);
        m_trailing->measure(renderer);
        right -= m_trailing->width();
        m_trailing->setPosition(std::round(right), std::round((height - m_trailing->height()) * 0.5F));
        right -= kSheetRowGap * scale;
      }

      const float titleX = padding + iconSize + kSheetRowGap * scale;
      m_title->setText(singleLinePreview(result.title));
      m_title->setFontSize((quiet ? kSheetQuietTitleSize : kSheetTitleSize) * scale);
      m_title->setFontWeight(selected ? FontWeight::Bold : FontWeight::Normal);
      m_title->setLetterSpacing(0.0F);
      m_title->setColor(ink);
      m_title->setMaxWidth(std::max(0.0F, right - titleX));
      m_title->measure(renderer);
      m_title->setPosition(titleX, std::round((height - m_title->height()) * 0.5F));
    }

  private:
    LauncherListStyle m_style{};
    Box* m_background = nullptr;
    Glyph* m_glyph = nullptr;
    Label* m_title = nullptr;
    Label* m_trailing = nullptr;
  };

} // namespace

class LauncherResultAdapter final : public VirtualGridAdapter {
public:
  using ActivateCallback = std::function<void(std::size_t)>;
  using SecondaryActivateCallback = std::function<void(std::size_t, float, float)>;
  using ReorderCallback = std::function<void(std::size_t, std::size_t)>;

  using TagCallback = std::function<std::string(std::string_view providerId)>;

  // `style.command` picks the row kind for the adapter's lifetime: tiles are pooled and never change kind.
  LauncherResultAdapter(LauncherListStyle style, AsyncTextureCache* cache)
      : m_style(style), m_cache(cache), m_command(style.command), m_sheet(style.sheet) {}

  void setListStyle(LauncherListStyle style) {
    m_style = style;
    m_style.command = m_command;
    m_style.sheet = m_sheet;
  }
  void setTagCallback(TagCallback callback) { m_tagFor = std::move(callback); }
  void setResults(const std::vector<LauncherResult>* results) { m_results = results; }
  void setRenderer(Renderer* renderer) { m_renderer = renderer; }
  [[nodiscard]] bool setReorderEnabled(bool enabled) {
    if (m_reorderEnabled == enabled) {
      return false;
    }
    m_reorderEnabled = enabled;
    return true;
  }
  void setOnActivate(ActivateCallback callback) { m_onActivate = std::move(callback); }
  void setOnSecondaryActivate(SecondaryActivateCallback callback) { m_onSecondaryActivate = std::move(callback); }
  void setOnReorder(ReorderCallback callback) { m_onReorder = std::move(callback); }

  [[nodiscard]] std::size_t itemCount() const override { return m_results == nullptr ? 0U : m_results->size(); }

  [[nodiscard]] std::unique_ptr<Node> createTile() override {
    if (m_sheet) {
      return std::make_unique<LauncherSheetRow>(m_style);
    }
    if (m_command) {
      return std::make_unique<LauncherCommandRow>(m_style, m_cache);
    }
    return std::make_unique<LauncherResultRow>(m_style, m_cache);
  }

  void bindTile(Node& tile, std::size_t index, bool selected, bool hovered) override {
    if (m_renderer == nullptr || m_results == nullptr || index >= m_results->size()) {
      return;
    }
    if (m_sheet) {
      auto* row = static_cast<LauncherSheetRow*>(&tile);
      row->setListStyle(m_style);
      row->bind(*m_renderer, (*m_results)[index], tile.width(), tile.height(), selected, hovered);
      return;
    }
    if (m_command) {
      auto* row = static_cast<LauncherCommandRow*>(&tile);
      const LauncherResult& result = (*m_results)[index];
      row->setListStyle(m_style);
      // The command list ("/") shows each trigger as a key cap; everything else names its kind.
      const bool command = result.providerId == kProviderOverviewProviderId;
      const std::string tag = command ? result.subtitle : (m_tagFor ? m_tagFor(result.providerId) : std::string{});
      row->bind(*m_renderer, result, tag, command, tile.width(), tile.height(), selected, hovered);
      return;
    }
    auto* row = static_cast<LauncherResultRow*>(&tile);
    row->setListStyle(m_style);
    row->setReorderTarget(m_dropIndex.has_value() && *m_dropIndex == index);
    row->bind(*m_renderer, (*m_results)[index], tile.width(), tile.height(), selected, hovered);
  }

  [[nodiscard]] std::string itemTooltip(std::size_t index) const override {
    if (m_sheet || m_results == nullptr || index >= m_results->size() || (*m_results)[index].originGlyph.empty()) {
      return {};
    }
    return (*m_results)[index].origin;
  }

  [[nodiscard]] std::optional<TooltipAnchorInsets>
  itemTooltipAnchorInsets(std::size_t index, float cellWidth, float cellHeight) const override {
    if (m_command
        || m_sheet
        || m_results == nullptr
        || index >= m_results->size()
        || (*m_results)[index].originGlyph.empty()) {
      return std::nullopt;
    }
    const float glyphSize = Style::fontSizeBody * m_style.scale;
    const float gap = (m_style.compact ? Style::spaceSm : Style::spaceMd) * m_style.scale;
    const float padding = Style::spaceSm * m_style.scale;
    const float pinnedWidth = (*m_results)[index].pinned ? glyphSize + gap : 0.0F;
    const float left = std::max(padding, cellWidth - padding - pinnedWidth - glyphSize);
    const float top = std::max(0.0F, (cellHeight - glyphSize) * 0.5F);
    return TooltipAnchorInsets{
        .top = top,
        .right = std::max(0.0F, cellWidth - left - glyphSize),
        .bottom = std::max(0.0F, cellHeight - top - glyphSize),
        .left = left,
    };
  }

  [[nodiscard]] bool overlayHitTest(
      std::size_t index, float /*cellLocalX*/, float /*cellLocalY*/, float /*cellWidth*/, float /*cellHeight*/
  ) const override {
    return isReorderable(index);
  }

  bool
  onPointerPress(std::size_t index, float cellLocalX, float cellLocalY, float cellWidth, float cellHeight) override {
    if (!overlayHitTest(index, cellLocalX, cellLocalY, cellWidth, cellHeight)) {
      return false;
    }
    m_dragSourceIndex = index;
    m_dropIndex.reset();
    m_dragging = false;
    return true;
  }

  bool onPointerDrag(
      std::optional<std::size_t> index, float /*localX*/, float /*localY*/, float /*cellWidth*/, float /*cellHeight*/
  ) override {
    if (!m_dragSourceIndex.has_value()) {
      return false;
    }
    m_dragging = true;

    const std::optional<std::size_t> nextTarget =
        index.has_value() && *index != *m_dragSourceIndex && isReorderable(*index) ? index : std::nullopt;
    if (nextTarget == m_dropIndex) {
      return false;
    }
    m_dropIndex = nextTarget;
    return true;
  }

  PointerReleaseResult onPointerRelease(std::optional<std::size_t> /*index*/) override {
    const auto sourceIndex = m_dragSourceIndex;
    const auto targetIndex = m_dropIndex;
    const bool reordered = m_dragging && sourceIndex.has_value() && targetIndex.has_value() && m_onReorder;
    const bool activated = !m_dragging && sourceIndex.has_value();
    m_dragSourceIndex.reset();
    m_dropIndex.reset();
    m_dragging = false;
    if (reordered) {
      m_onReorder(*sourceIndex, *targetIndex);
    }
    return {
        .rebind = sourceIndex.has_value() || targetIndex.has_value(),
        .activateIndex = activated ? sourceIndex : std::nullopt,
    };
  }

  void onPointerCancel() override {
    m_dragSourceIndex.reset();
    m_dropIndex.reset();
    m_dragging = false;
  }

  void onActivate(std::size_t index) override {
    if (m_onActivate) {
      m_onActivate(index);
    }
  }

  void onSecondaryActivate(std::size_t index, float anchorX, float anchorY) override {
    if (m_onSecondaryActivate) {
      m_onSecondaryActivate(index, anchorX, anchorY);
    }
  }

private:
  [[nodiscard]] bool isReorderable(std::size_t index) const {
    return m_reorderEnabled && m_results != nullptr && index < m_results->size() && (*m_results)[index].pinned;
  }

  LauncherListStyle m_style{};
  AsyncTextureCache* m_cache = nullptr;
  bool m_command = false;
  bool m_sheet = false;
  TagCallback m_tagFor;
  Renderer* m_renderer = nullptr;
  const std::vector<LauncherResult>* m_results = nullptr;
  ActivateCallback m_onActivate;
  SecondaryActivateCallback m_onSecondaryActivate;
  ReorderCallback m_onReorder;
  bool m_reorderEnabled = false;
  std::optional<std::size_t> m_dragSourceIndex;
  std::optional<std::size_t> m_dropIndex;
  bool m_dragging = false;
};

class LauncherAppGridAdapter final : public VirtualGridAdapter {
public:
  using ActivateCallback = std::function<void(std::size_t)>;
  using SecondaryActivateCallback = std::function<void(std::size_t, float, float)>;
  using ReorderCallback = std::function<void(std::size_t, std::size_t)>;

  LauncherAppGridAdapter(LauncherListStyle style, AsyncTextureCache* cache) : m_style(style), m_cache(cache) {}

  void setListStyle(LauncherListStyle style) { m_style = style; }
  void setResults(const std::vector<LauncherResult>* results) { m_results = results; }
  void setRenderer(Renderer* renderer) { m_renderer = renderer; }
  [[nodiscard]] bool setReorderEnabled(bool enabled) {
    if (m_reorderEnabled == enabled) {
      return false;
    }
    m_reorderEnabled = enabled;
    return true;
  }
  void setOnActivate(ActivateCallback callback) { m_onActivate = std::move(callback); }
  void setOnSecondaryActivate(SecondaryActivateCallback callback) { m_onSecondaryActivate = std::move(callback); }
  void setOnReorder(ReorderCallback callback) { m_onReorder = std::move(callback); }

  [[nodiscard]] std::size_t itemCount() const override { return m_results == nullptr ? 0U : m_results->size(); }

  [[nodiscard]] std::unique_ptr<Node> createTile() override {
    return std::make_unique<LauncherAppGridTile>(m_style, m_cache);
  }

  void bindTile(Node& tile, std::size_t index, bool selected, bool hovered) override {
    if (m_renderer == nullptr || m_results == nullptr || index >= m_results->size()) {
      return;
    }
    auto* gridTile = static_cast<LauncherAppGridTile*>(&tile);
    gridTile->setListStyle(m_style);
    gridTile->setReorderTarget(m_dropIndex.has_value() && *m_dropIndex == index);
    gridTile->bind(*m_renderer, (*m_results)[index], tile.width(), tile.height(), selected, hovered);
  }

  [[nodiscard]] std::string itemTooltip(std::size_t index) const override {
    if (m_results == nullptr || index >= m_results->size() || (*m_results)[index].originGlyph.empty()) {
      return {};
    }
    return (*m_results)[index].origin;
  }

  [[nodiscard]] std::optional<TooltipAnchorInsets>
  itemTooltipAnchorInsets(std::size_t index, float cellWidth, float cellHeight) const override {
    if (m_results == nullptr || index >= m_results->size() || (*m_results)[index].originGlyph.empty()) {
      return std::nullopt;
    }
    const float glyphSize = Style::fontSizeBody * m_style.scale;
    const float padding = Style::spaceSm * m_style.scale;
    const float left = std::max(0.0F, cellWidth - padding - glyphSize);
    const float top = std::max(0.0F, cellHeight - padding - glyphSize);
    return TooltipAnchorInsets{
        .top = top,
        .right = std::max(0.0F, cellWidth - left - glyphSize),
        .bottom = std::max(0.0F, cellHeight - top - glyphSize),
        .left = left,
    };
  }

  [[nodiscard]] bool overlayHitTest(
      std::size_t index, float /*cellLocalX*/, float /*cellLocalY*/, float /*cellWidth*/, float /*cellHeight*/
  ) const override {
    return isReorderable(index);
  }

  bool
  onPointerPress(std::size_t index, float cellLocalX, float cellLocalY, float cellWidth, float cellHeight) override {
    if (!overlayHitTest(index, cellLocalX, cellLocalY, cellWidth, cellHeight)) {
      return false;
    }
    m_dragSourceIndex = index;
    m_dropIndex.reset();
    m_dragging = false;
    return true;
  }

  bool onPointerDrag(
      std::optional<std::size_t> index, float /*localX*/, float /*localY*/, float /*cellWidth*/, float /*cellHeight*/
  ) override {
    if (!m_dragSourceIndex.has_value()) {
      return false;
    }
    m_dragging = true;
    const std::optional<std::size_t> nextTarget =
        index.has_value() && *index != *m_dragSourceIndex && isReorderable(*index) ? index : std::nullopt;
    if (nextTarget == m_dropIndex) {
      return false;
    }
    m_dropIndex = nextTarget;
    return true;
  }

  PointerReleaseResult onPointerRelease(std::optional<std::size_t> /*index*/) override {
    const auto sourceIndex = m_dragSourceIndex;
    const auto targetIndex = m_dropIndex;
    const bool reordered = m_dragging && sourceIndex.has_value() && targetIndex.has_value() && m_onReorder;
    const bool activated = !m_dragging && sourceIndex.has_value();
    m_dragSourceIndex.reset();
    m_dropIndex.reset();
    m_dragging = false;
    if (reordered) {
      m_onReorder(*sourceIndex, *targetIndex);
    }
    return {
        .rebind = sourceIndex.has_value() || targetIndex.has_value(),
        .activateIndex = activated ? sourceIndex : std::nullopt,
    };
  }

  void onPointerCancel() override {
    m_dragSourceIndex.reset();
    m_dropIndex.reset();
    m_dragging = false;
  }

  void onActivate(std::size_t index) override {
    if (m_onActivate) {
      m_onActivate(index);
    }
  }

  void onSecondaryActivate(std::size_t index, float anchorX, float anchorY) override {
    if (m_onSecondaryActivate) {
      m_onSecondaryActivate(index, anchorX, anchorY);
    }
  }

private:
  [[nodiscard]] bool isReorderable(std::size_t index) const {
    return m_reorderEnabled && m_results != nullptr && index < m_results->size() && (*m_results)[index].pinned;
  }

  LauncherListStyle m_style{};
  AsyncTextureCache* m_cache = nullptr;
  Renderer* m_renderer = nullptr;
  const std::vector<LauncherResult>* m_results = nullptr;
  ActivateCallback m_onActivate;
  SecondaryActivateCallback m_onSecondaryActivate;
  ReorderCallback m_onReorder;
  bool m_reorderEnabled = false;
  std::optional<std::size_t> m_dragSourceIndex;
  std::optional<std::size_t> m_dropIndex;
  bool m_dragging = false;
};

LauncherPanel::LauncherPanel(
    ConfigService* config, AsyncTextureCache* asyncTextures, SessionActionRunner* sessionActions
)
    : m_iconResolver(true), m_config(config), m_asyncTextures(asyncTextures), m_sessionActions(sessionActions) {}

LauncherPanel::~LauncherPanel() = default;

PanelPlacement LauncherPanel::panelPlacement() const noexcept {
  return m_config != nullptr ? m_config->config().shell.panel.launcherPlacement : PanelPlacement::Floating;
}

void LauncherPanel::applyProviderConfig(LauncherProvider& provider) const {
  std::string triggerWord = std::string(provider.defaultPrefix());
  std::string prefix = "/";
  std::optional<bool> global;
  if (m_config != nullptr) {
    const auto& launcherCfg = m_config->config().shell.launcher;
    prefix = launcherCfg.providerPrefix;
    if (provider.allowCustomPrefix()) {
      const std::string key = StringUtils::toLower(std::string(provider.id()));
      auto it = std::ranges::find(launcherCfg.providers, key, &LauncherProviderConfig::name);
      if (it != launcherCfg.providers.end()) {
        if (!it->prefix.empty()) {
          triggerWord = it->prefix;
        }
        global = it->global;
      }
    }
  }

  if (provider.allowCustomPrefix()) {
    provider.setCustomPrefix(triggerWord.empty() ? std::string() : prefix + triggerWord);
    provider.setCustomIncludeInGlobalSearch(global);
  }
}

void LauncherPanel::finishActivation(LauncherProvider& provider, const std::string& resultId, bool copied) {
  if (shouldTrackUsage() && provider.trackUsage()) {
    m_usageTracker.record(provider.id(), resultId);
  }
  PanelManager::instance().closePanel(false);
  if (copied && provider.supportsAutoPaste() && m_onCopiedActivation) {
    m_onCopiedActivation();
  }
}

void LauncherPanel::addProvider(std::unique_ptr<LauncherProvider> provider) {
  applyProviderConfig(*provider);
  provider->initialize();
  provider->setResultsChangedCallback([this]() { onProviderResultsChanged(); });
  provider->setQueryRequestedCallback([this](std::string query) { setQuery(std::move(query)); });
  LauncherProvider* providerPtr = provider.get();
  provider->setActivationDoneCallback([this, providerPtr](const std::string& resultId, bool copied) {
    finishActivation(*providerPtr, resultId, copied);
  });
  m_providers.push_back(std::move(provider));
}

void LauncherPanel::clearDynamicProviders() {
  std::erase_if(m_providers, [](const std::unique_ptr<LauncherProvider>& provider) { return provider->isDynamic(); });
}

void LauncherPanel::clearProvidersWithIdPrefix(std::string_view prefix) {
  std::erase_if(m_providers, [&](const std::unique_ptr<LauncherProvider>& provider) {
    return provider->id().starts_with(prefix);
  });
}

void LauncherPanel::setScopedProvider(std::string_view providerId, std::string_view placeholder) {
  m_scopedProviderId = providerId;
  m_scopedPlaceholder = placeholder;
  if (m_input != nullptr) {
    m_input->setPlaceholder(m_scopedPlaceholder.empty() ? defaultPlaceholder() : m_scopedPlaceholder);
  }
}

bool LauncherPanel::commandStyle() const {
  if (m_container != nullptr) {
    return m_commandStyle;
  }
  return m_config == nullptr || m_config->config().shell.launcher.style == ShellConfig::LauncherStyle::Command;
}

bool LauncherPanel::sheetStyle() const {
  if (m_container != nullptr) {
    return m_sheetStyle;
  }
  return m_config != nullptr && m_config->config().shell.launcher.style == ShellConfig::LauncherStyle::Sheet;
}

std::string LauncherPanel::sectionTitle(std::string_view providerId) const {
  if (providerId == kApplicationsProviderId) {
    return i18n::tr("launcher.sections.apps");
  }
  return providerDisplayName(providerId);
}

std::unique_ptr<Flex> LauncherPanel::buildSessionFoot(float scale) {
  if (m_sessionActions == nullptr || m_config == nullptr) {
    return nullptr;
  }
  // One tile each for locking, sleeping and switching off, from the configured actions.
  const auto& actions = m_config->config().shell.session.actions;
  const auto find = [&](std::initializer_list<std::string_view> names) -> const SessionPanelActionConfig* {
    for (const std::string_view name : names) {
      for (const auto& action : actions) {
        if (action.enabled && action.action == name) {
          return &action;
        }
      }
    }
    return nullptr;
  };
  const bool canLock = m_config->isLockScreenEnabled();
  const SessionPanelActionConfig* lock = canLock ? find({"lock"}) : nullptr;
  const SessionPanelActionConfig* sleep = canLock ? find({"lock_and_suspend", "suspend"}) : find({"suspend"});
  const SessionPanelActionConfig* off = find({"shutdown"});
  std::vector<std::unique_ptr<Button>> tiles;
  for (const SessionPanelActionConfig* action : {lock, sleep, off}) {
    if (action != nullptr) {
      tiles.push_back(session_tiles::makeTile(*action, scale, [this, cfg = *action]() { runSessionAction(cfg); }));
    }
  }
  if (tiles.empty()) {
    return nullptr;
  }
  const std::size_t columns = tiles.size();
  return session_tiles::makeGrid(std::move(tiles), columns, scale);
}

void LauncherPanel::runSessionAction(const SessionPanelActionConfig& action) {
  SessionActionRunner* const runner = m_sessionActions;
  DeferredCall::callLater([runner, action]() {
    PanelManager::instance().closePanel(/*animateClose=*/false);
    runner->invoke(action);
  });
}

std::string LauncherPanel::defaultPlaceholder() const {
  if (sheetStyle()) {
    return i18n::tr("launcher.sheet-placeholder");
  }
  return i18n::tr(commandStyle() ? "launcher.command-placeholder" : "launcher.search-placeholder");
}

std::string LauncherPanel::resultTag(std::string_view providerId) const {
  // Built-in providers get a singular kind ("Application"); plugins and dmenu entries show their own name.
  static constexpr std::pair<std::string_view, std::string_view> kTags[] = {
      {"Applications", "launcher.tags.applications"},
      {"Calculator", "launcher.tags.calculator"},
      {"Emoji", "launcher.tags.emoji"},
      {"Panels", "launcher.tags.panels"},
      {"Session", "launcher.tags.session"},
      {"Settings", "launcher.tags.settings"},
      {"Wallpaper", "launcher.tags.wallpaper"},
      {"Windows", "launcher.tags.windows"},
  };
  for (const auto& [id, key] : kTags) {
    if (id == providerId) {
      return i18n::tr(key);
    }
  }
  return providerDisplayName(providerId);
}

void LauncherPanel::create() {
  m_launcherRowHeight = 0.0F;
  m_commandStyle =
      m_config == nullptr || m_config->config().shell.launcher.style == ShellConfig::LauncherStyle::Command;
  m_sheetStyle = m_config != nullptr && m_config->config().shell.launcher.style == ShellConfig::LauncherStyle::Sheet;
  m_commandHeight = 0.0F;
  const bool command = m_commandStyle;
  const bool sheet = m_sheetStyle;
  const float scale = contentScale();
  auto container = ui::column({
      .out = &m_container,
      .align = FlexAlign::Stretch,
      .gap = command || sheet ? 0.0F : Style::spaceSm * scale,
  });

  // Search row: magnifier glyph and a large, frameless field (a tighter one in the command bar, an underlined one
  // in the sheet).
  const float searchFontSize = sheet ? kSheetSearchFontSize * scale
      : command                      ? Style::fontSizeTitle * 1.07F * scale
                                     : Style::fontSizeTitle * 1.125F * scale;
  const float searchGlyphSize = sheet ? kSheetSearchGlyphSize * scale
      : command                       ? Style::fontSizeTitle * 1.07F * scale
                                      : Style::fontSizeTitle * scale;
  const float searchHeight = sheet ? kSheetSearchHeight : command ? kCommandSearchHeight : kSpotlightSearchHeight;
  auto searchRow = ui::row(
      {
          .align = FlexAlign::Center,
          .gap = sheet ? kSheetSearchGap * scale
              : command ? (Style::spaceSm + 2.0F) * scale
                        : Style::spaceSm * scale,
          .paddingH = sheet ? 0.0F : command ? Style::spaceLg * scale : Style::spaceSm * scale,
      },
      ui::glyph({
          .glyph = "search",
          .glyphSize = searchGlyphSize,
          .color = colorSpecFromRole(sheet ? ColorRole::OnSurface : ColorRole::OnSurfaceVariant),
      }),
      ui::input({
          .out = &m_input,
          .placeholder = m_scopedPlaceholder.empty() ? defaultPlaceholder() : m_scopedPlaceholder,
          .fontSize = searchFontSize,
          .controlHeight = searchHeight * scale,
          .horizontalPadding = sheet ? 0.0F : Style::spaceXs * scale,
          .clearButtonEnabled = !sheet,
          .frameVisible = false,
          .fontWeight = sheet ? std::optional<FontWeight>(FontWeight::Medium) : std::nullopt,
          .surfaceOpacity = panelCardOpacity(),
          .flexGrow = 1.0F,
          .onChange =
              [this](const std::string& text) {
                onInputChanged(text);
                if (m_input == nullptr) {
                  return;
                }
                const std::string preview = singleLinePreview(text);
                if (preview != text) {
                  m_input->setValue(preview);
                }
              },
          .onSubmit = [this](const std::string& /*text*/) { activateSelected(); },
          .onKeyEvent = [this](std::uint32_t sym, std::uint32_t modifiers) { return handleKeyEvent(sym, modifiers); },
          .configure =
              [sheet, scale](Input& input) {
                if (sheet) {
                  input.setCaretSize(2.0F * scale, kSheetSearchGlyphSize * scale);
                }
              },
      })
  );
  if (command) {
    // While the field is empty: "/" lists the commands (every provider prefix).
    auto hint = commandKeyHint(commandKeycap({}, "/", scale), i18n::tr("launcher.footer.commands"), scale);
    m_commandHint = hint.get();
    searchRow->addChild(std::move(hint));
  }
  if (sheet) {
    // The field sits on an accent rule; the results start a section's breath below it.
    container->addChild(
        ui::column(
            {.align = FlexAlign::Stretch, .gap = kSheetSearchGap * scale}, std::move(searchRow),
            ui::box({.fill = colorSpecFromRole(ColorRole::Primary), .height = kSheetUnderline * scale})
        )
    );
  } else {
    container->addChild(std::move(searchRow));
    container->addChild(
        ui::separator({
            .out = &m_searchDivider,
            .color = colorSpecFromRole(ColorRole::Outline, command ? 0.55F : 1.0F),
            .thickness = command ? kCommandDividerThickness : 1.0F,
            .visible = !m_collapsed,
            .participatesInLayout = !m_collapsed,
        })
    );
  }

  container->addChild(
      ui::segmented({
          .out = &m_categoryFilter,
          .scale = scale,
          .compact = true,
          .surfaceOpacity = panelCardOpacity(),
          .equalSegmentWidths = true,
          .visible = false,
          .participatesInLayout = false,
          .configure = [](Segmented& segmented) { segmented.setAlign(FlexAlign::Center); },
      })
  );

  auto body = ui::column({
      .out = &m_body,
      .align = FlexAlign::Stretch,
      .justify = command ? FlexJustify::Center : FlexJustify::Start,
      .padding = command ? kCommandListPadding * scale : 0.0F,
      .fillWidth = true,
      .flexGrow = 1.0F,
      .visible = !m_collapsed,
      .participatesInLayout = !m_collapsed,
  });

  const LauncherListStyle initialStyle =
      launcherListStyleFrom(m_config, scale, panelCardOpacity(), m_commandStyle, m_sheetStyle);
  m_listAdapter = std::make_unique<LauncherResultAdapter>(initialStyle, m_asyncTextures);
  m_gridAdapter = std::make_unique<LauncherAppGridAdapter>(initialStyle, m_asyncTextures);
  m_listAdapter->setResults(&m_results);
  m_listAdapter->setTagCallback([this](std::string_view providerId) { return resultTag(providerId); });
  m_gridAdapter->setResults(&m_results);
  const auto onActivate = [this](std::size_t index) { activateAt(index); };
  const auto onSecondaryActivate = [this](std::size_t index, float ax, float ay) {
    (void)openAppActionsMenu(index, ax, ay);
  };
  m_listAdapter->setOnActivate(onActivate);
  m_listAdapter->setOnSecondaryActivate(onSecondaryActivate);
  m_listAdapter->setOnReorder([this](std::size_t sourceIndex, std::size_t targetIndex) {
    if (sourceIndex >= m_results.size() || targetIndex >= m_results.size()) {
      return;
    }
    reorderPinnedApplication(m_results[sourceIndex].desktopEntryPath, m_results[targetIndex].desktopEntryPath);
  });
  m_gridAdapter->setOnActivate(onActivate);
  m_gridAdapter->setOnSecondaryActivate(onSecondaryActivate);
  m_gridAdapter->setOnReorder([this](std::size_t sourceIndex, std::size_t targetIndex) {
    if (sourceIndex >= m_results.size() || targetIndex >= m_results.size()) {
      return;
    }
    reorderPinnedApplication(m_results[sourceIndex].desktopEntryPath, m_results[targetIndex].desktopEntryPath);
  });

  body->addChild(
      ui::virtualGridView({
          .out = &m_grid,
          .columns = 1,
          .cellHeight = launcherRowHeightEstimate(initialStyle),
          .squareCells = false,
          .columnGap = 0.0F,
          .rowGap = sheet ? 0.0F : (command ? kCommandRowGap : Style::spaceXs) * scale,
          .overscanRows = kRowOverscan,
          .itemCursorShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER,
          .adapter = m_listAdapter.get(),
          .flexGrow = 1.0F,
          .onSelectionChanged =
              [this](std::optional<std::size_t> idx) {
                if (idx.has_value() && *idx < m_results.size()) {
                  m_selectedIndex = *idx;
                  syncCommandChrome();
                }
              },
          .configure = [](VirtualGridView& grid) { grid.setFillWidth(true); },
      })
  );

  auto detailScroll = ui::scrollView({
      .out = &m_detailScroll,
      .contentScale = scale,
      .scrollbarVisible = true,
      .viewportPaddingH = Style::spaceSm * scale,
      .viewportPaddingV = Style::spaceSm * scale,
      .flexGrow = 1.0F,
      .visible = false,
      .participatesInLayout = false,
      .configure = [scale, opacity = panelCardOpacity()](ScrollView& scrollView) {
        scrollView.setCardStyle(scale, opacity);
      },
  });
  auto* detailContent = detailScroll->content();
  detailContent->setDirection(FlexDirection::Vertical);
  detailContent->setAlign(FlexAlign::Stretch);
  detailContent->setGap(Style::spaceSm * scale);
  detailContent->addChild(
      ui::label({
          .out = &m_detailSubtitle,
          .fontSize = Style::fontSizeCaption * scale,
          .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
          .visible = false,
          .participatesInLayout = false,
      })
  );
  detailContent->addChild(
      ui::label({
          .out = &m_detailBody,
          .fontSize = Style::fontSizeBody * scale,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxLines = 0,
          .flexGrow = 1.0F,
      })
  );
  body->addChild(std::move(detailScroll));

  body->addChild(
      ui::label({
          .out = &m_emptyLabel,
          .fontSize = Style::fontSizeCaption * scale,
          .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          .textAlign = command ? TextAlign::Center : TextAlign::Start,
          .visible = false,
          .participatesInLayout = false,
      })
  );

  container->addChild(std::move(body));

  if (sheet) {
    if (auto foot = buildSessionFoot(scale); foot != nullptr) {
      container->addChild(ui::box({.fill = clearColorSpec(), .height = kSheetPadding * scale}));
      container->addChild(std::move(foot));
    }
  }

  if (command) {
    container->addChild(
        ui::separator({
            .out = &m_footerDivider,
            .color = colorSpecFromRole(ColorRole::Outline, 0.55F),
            .thickness = kCommandDividerThickness,
            .visible = !m_collapsed,
            .participatesInLayout = !m_collapsed,
        })
    );
    auto footer = ui::row(
        {
            .out = &m_footer,
            .align = FlexAlign::Center,
            .gap = Style::spaceMd * scale,
            .paddingH = Style::spaceLg * scale,
            .height = kCommandFooterHeight * scale,
            .visible = !m_collapsed,
            .participatesInLayout = !m_collapsed,
        },
        ui::label({
            .out = &m_footerCount,
            .fontSize = Style::fontSizeMini * scale,
            .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
            .maxLines = 1,
            .flexGrow = 1.0F,
        }),
        commandKeyHint(commandKeycap({"arrows-up-down"}, {}, scale), {}, scale)
    );
    m_footerNavigate = static_cast<Flex*>(footer->children().back().get());
    auto open = commandKeyHint(commandKeycap({"corner-down-left"}, {}, scale), i18n::tr("launcher.footer.open"), scale);
    m_footerOpen = open.get();
    footer->addChild(std::move(open));
    auto actions = commandKeyHint(
        commandKeycap({"arrow-big-up", "corner-down-left"}, {}, scale), i18n::tr("launcher.footer.actions"), scale
    );
    m_footerActions = actions.get();
    footer->addChild(std::move(actions));
    footer->addChild(commandKeyHint(commandKeycap({}, "esc", scale), i18n::tr("launcher.footer.close"), scale));
    container->addChild(std::move(footer));
  }

  setRoot(std::move(container));

  if (m_animations != nullptr) {
    root()->setAnimationManager(m_animations);
  }

  m_appIconColorizeConn = shellAppIconColorizationChanged().connect([this]() { refreshLauncherAppIconColorization(); });

  syncLauncherListStyle();
}

void LauncherPanel::refreshLauncherAppIconColorization() {
  if (m_listAdapter == nullptr || m_gridAdapter == nullptr || m_grid == nullptr) {
    return;
  }
  const LauncherListStyle style =
      launcherListStyleFrom(m_config, contentScale(), panelCardOpacity(), m_commandStyle, m_sheetStyle);
  m_listAdapter->setListStyle(style);
  m_gridAdapter->setListStyle(style);
  m_grid->notifyDataChanged();
}

bool LauncherPanel::shouldUseAppGrid() const {
  if (m_commandStyle || m_config == nullptr || !m_config->config().shell.launcher.appGrid || !m_launcherShowIcons) {
    return false;
  }
  if (m_results.empty()) {
    return false;
  }
  return std::ranges::all_of(m_results, [](const LauncherResult& result) {
    return result.providerId == kApplicationsProviderId;
  });
}

void LauncherPanel::syncLauncherViewLayout(Renderer* renderer) {
  if (m_grid == nullptr || m_listAdapter == nullptr || m_gridAdapter == nullptr) {
    return;
  }

  const bool useGrid = shouldUseAppGrid();
  const float scale = contentScale();
  const LauncherListStyle style =
      launcherListStyleFrom(m_config, scale, panelCardOpacity(), m_commandStyle, m_sheetStyle);
  m_grid->setScale(scale);
  m_listAdapter->setListStyle(style);
  m_gridAdapter->setListStyle(style);
  const bool reorderEnabled = m_query.empty() && m_scopedProviderId.empty() && m_activeCategoryType == All;
  const bool listReorderEnabledChanged = m_listAdapter->setReorderEnabled(reorderEnabled);
  const bool gridReorderEnabledChanged = m_gridAdapter->setReorderEnabled(reorderEnabled);
  const bool reorderEnabledChanged = listReorderEnabledChanged || gridReorderEnabledChanged;
  if (renderer != nullptr) {
    m_listAdapter->setRenderer(renderer);
    m_gridAdapter->setRenderer(renderer);
  }

  const bool modeChanged = useGrid != m_usingAppGrid;
  m_usingAppGrid = useGrid;
  if (modeChanged || reorderEnabledChanged) {
    m_launcherRowHeight = 0.0F;
  }

  if (useGrid) {
    m_grid->setColumns(kAppGridColumns);
    m_grid->setSquareCells(false);
    m_grid->setColumnGap(Style::spaceSm * scale);
    m_grid->setRowGap(Style::spaceSm * scale);
    m_grid->setCellHeight(launcherAppGridCellHeightEstimate(style));
    if (modeChanged) {
      m_grid->setAdapter(m_gridAdapter.get());
    }
  } else {
    m_grid->setColumns(1);
    m_grid->setColumnGap(0.0F);
    m_grid->setRowGap(m_sheetStyle ? 0.0F : (m_commandStyle ? kCommandRowGap : Style::spaceXs) * scale);
    const float listCellHeight =
        renderer != nullptr ? launcherRowHeight(*renderer, style) : launcherRowHeightEstimate(style);
    m_grid->setCellHeight(listCellHeight);
    if (renderer != nullptr) {
      m_launcherRowHeight = listCellHeight;
    }
    if (modeChanged) {
      m_grid->setAdapter(m_listAdapter.get());
    }
  }

  if (modeChanged || reorderEnabledChanged) {
    if (renderer != nullptr) {
      updateLauncherGridMetrics(*renderer);
    }
    m_grid->notifyDataChanged();
  }
}

void LauncherPanel::syncLauncherListStyle() {
  const bool showIcons = m_config == nullptr || m_config->config().shell.launcher.showIcons;
  const bool showAppOriginIndicator = m_config == nullptr || m_config->config().shell.launcher.showAppOriginIndicator;
  const bool compact = m_config != nullptr && m_config->config().shell.launcher.compact;
  const bool appGrid = m_config != nullptr && m_config->config().shell.launcher.appGrid;
  if (showIcons == m_launcherShowIcons
      && showAppOriginIndicator == m_launcherShowAppOriginIndicator
      && compact == m_launcherCompact
      && appGrid == m_launcherAppGrid
      && m_listAdapter != nullptr) {
    return;
  }
  m_launcherShowIcons = showIcons;
  m_launcherShowAppOriginIndicator = showAppOriginIndicator;
  m_launcherCompact = compact;
  m_launcherAppGrid = appGrid;
  m_launcherRowHeight = 0.0F;
  syncLauncherViewLayout(nullptr);
  if (m_grid != nullptr) {
    m_grid->notifyDataChanged();
  }
}

void LauncherPanel::updateLauncherGridMetrics(Renderer& renderer) {
  if (m_grid == nullptr) {
    return;
  }

  const LauncherListStyle style =
      launcherListStyleFrom(m_config, contentScale(), panelCardOpacity(), m_commandStyle, m_sheetStyle);
  float cellHeight = launcherRowHeight(renderer, style);
  if (m_usingAppGrid) {
    float wrapWidth = 0.0F;
    const std::size_t columns = std::max<std::size_t>(1, m_grid->layoutColumnCount());
    const float viewportW = m_grid->scrollView().contentViewportWidth();
    const float gap = Style::spaceSm * contentScale();
    const float cellW =
        columns > 0 ? (viewportW - static_cast<float>(columns - 1) * gap) / static_cast<float>(columns) : viewportW;
    const float paddingH = Style::spaceSm * contentScale() * 2.0F;
    wrapWidth = std::max(0.0F, cellW - paddingH);
    cellHeight = launcherAppGridCellHeight(renderer, style, wrapWidth);
  }
  if (std::abs(cellHeight - m_launcherRowHeight) < 0.5F) {
    return;
  }

  m_launcherRowHeight = cellHeight;
  m_grid->setCellHeight(cellHeight);
}

void LauncherPanel::onPanelCardOpacityChanged(float opacity) {
  if (m_input != nullptr) {
    m_input->setSurfaceOpacity(opacity);
  }
  if (m_categoryFilter != nullptr) {
    m_categoryFilter->setSurfaceOpacity(opacity);
  }
  if (m_detailScroll != nullptr) {
    m_detailScroll->setCardStyle(contentScale(), opacity);
  }
}

void LauncherPanel::doLayout(Renderer& renderer, float width, float height) {
  if (m_container == nullptr || m_input == nullptr) {
    return;
  }

  syncLauncherListStyle();
  syncLauncherViewLayout(&renderer);
  updateLauncherGridMetrics(renderer);

  m_container->setSize(width, height);
  m_container->layout(renderer);
}

void LauncherPanel::onOpen(std::string_view context) {
  for (auto& provider : m_providers) {
    applyProviderConfig(*provider);
  }

  // Pick up apps installed since the last scan (notably Nix profile swaps that
  // inotify cannot observe). Cheap stat-only check; only rescans on real change.
  refreshDesktopEntriesIfSourcesChanged();

  m_categoryFilterVisible = m_config != nullptr && m_config->config().shell.launcher.categories;
  m_activeCategoryType = All;
  m_activeCategory.clear();
  m_currentCategories.clear();
  m_categoryFilterSlots.clear();
  m_hasRecentlyUsed = false;
  if (m_categoryFilter != nullptr) {
    m_categoryFilter->clearOptions();
    m_categoryFilter->setVisible(false);
    m_categoryFilter->setParticipatesInLayout(false);
  }

  const std::string initialValue(context);
  if (m_input != nullptr) {
    m_input->setPlaceholder(m_scopedPlaceholder.empty() ? defaultPlaceholder() : m_scopedPlaceholder);
    m_input->setValue(singleLinePreview(initialValue));
  }
  if (m_grid != nullptr) {
    m_grid->scrollView().setScrollOffset(0.0F);
  }
  onInputChanged(initialValue);
}

void LauncherPanel::onClose() {
  if (m_actionsMenu != nullptr && m_actionsMenu->isOpen()) {
    m_actionsMenu->close();
  }

  if (m_asyncTextures != nullptr) {
    DeferredCall::callLater([asyncTextures = m_asyncTextures]() { asyncTextures->trimUnused(0); });
  }

  for (auto& provider : m_providers) {
    provider->reset();
  }

  m_query.clear();
  m_results.clear();
  m_allResults.clear();
  m_scopedProviderId.clear();
  m_scopedPlaceholder.clear();
  m_activeCategoryType = All;
  m_activeCategory.clear();
  m_currentCategories.clear();
  m_categoryFilterSlots.clear();
  m_hasRecentlyUsed = false;
  m_selectedIndex = 0;
  m_usingAppGrid = false;
  m_collapsed = true;
  m_launcherRowHeight = 0.0F;

  if (m_grid != nullptr) {
    m_grid->setAdapter(nullptr);
  }
  m_listAdapter.reset();
  m_gridAdapter.reset();

  // The scene tree (and all nodes) is destroyed by PanelManager after onClose().
  m_container = nullptr;
  m_input = nullptr;
  m_commandHint = nullptr;
  m_footerDivider = nullptr;
  m_footer = nullptr;
  m_footerCount = nullptr;
  m_footerNavigate = nullptr;
  m_footerOpen = nullptr;
  m_footerActions = nullptr;
  m_commandHeight = 0.0F;
  m_searchDivider = nullptr;
  m_categoryFilter = nullptr;
  m_body = nullptr;
  m_grid = nullptr;
  m_detailScroll = nullptr;
  m_detailSubtitle = nullptr;
  m_detailBody = nullptr;
  m_emptyLabel = nullptr;
  clearReleasedRoot();
}

void LauncherPanel::onIconThemeChanged() { reapplyCurrentQuery(); }

void LauncherPanel::clearUsage() {
  m_usageTracker.clear();
  if (m_input != nullptr) {
    reapplyCurrentQuery();
  }
}

bool LauncherPanel::shouldTrackUsage() const {
  return m_config != nullptr && m_config->config().shell.launcher.sortByUsage;
}

void LauncherPanel::syncUsageTrackingState() {
  if (m_input != nullptr) {
    reapplyCurrentQuery();
  }
}

void LauncherPanel::reapplyCurrentQuery() {
  std::string selectedProvider;
  std::string selectedId;
  if (m_selectedIndex < m_results.size()) {
    selectedProvider = m_results[m_selectedIndex].providerId;
    selectedId = m_results[m_selectedIndex].id;
  }

  onInputChanged(m_query);

  if (!selectedId.empty()) {
    for (std::size_t i = 0; i < m_results.size(); ++i) {
      if (m_results[i].providerId == selectedProvider && m_results[i].id == selectedId) {
        m_selectedIndex = i;
        break;
      }
    }
  }
  refreshResults();
}

void LauncherPanel::setQuery(std::string query) {
  if (m_input == nullptr) {
    return;
  }
  m_input->setValue(singleLinePreview(query));
  if (m_grid != nullptr) {
    m_grid->scrollView().setScrollOffset(0.0F);
  }
  onInputChanged(query);
}

void LauncherPanel::onProviderResultsChanged() {
  // Only re-gather while the panel is open and built; after onClose the scene
  // nodes are gone and a refresh would touch null grid/label pointers.
  if (m_input == nullptr) {
    return;
  }
  reapplyCurrentQuery();
}

InputArea* LauncherPanel::initialFocusArea() const { return m_input != nullptr ? m_input->inputArea() : nullptr; }

bool LauncherPanel::handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) {
  if (!pressed || preedit) {
    return false;
  }

  auto& dispatcher = PanelManager::instance().inputDispatcher();
  InputArea* const focused = dispatcher.focusedArea();
  if (focused != nullptr) {
    const bool onInput = (m_input != nullptr && focused == m_input->inputArea());
    const bool inResults = (m_grid != nullptr && isDescendantOf(focused, m_grid));
    if (!onInput && !inResults) {
      return false;
    }
  }

  if (m_categoryFilter != nullptr && m_categoryFilter->visible()) {
    if (focused == m_categoryFilter->focusArea()) {
      return false;
    }
  }

  return handleKeyEvent(sym, modifiers);
}

void LauncherPanel::onInputChanged(const std::string& text) {
  const auto desktopVersion = desktopEntriesVersion();
  if (desktopVersion != m_desktopEntriesVersion) {
    m_iconResolver.invalidateMissingCache();
    m_desktopEntriesVersion = desktopVersion;
  }

  m_query = text;
  m_allResults.clear();

  std::vector<LauncherCategory> newCategories;
  bool hasRecentlyUsed = false;
  bool anyProviderLoading = false;

  if (!m_scopedProviderId.empty()) {
    for (auto& provider : m_providers) {
      if (provider->id() != m_scopedProviderId) {
        continue;
      }
      m_allResults = provider->query(text);
      anyProviderLoading = provider->isLoading();
      for (auto& result : m_allResults) {
        result.providerId = provider->id();
      }
      sortResultsByScore(m_allResults);
      break;
    }
  } else {
    // Route query to providers
    LauncherProvider* activeProvider = nullptr;
    std::string_view queryText = text;

    // Check for prefix match (longest first)
    for (auto& provider : m_providers) {
      auto prefix = provider->prefix();
      if (prefix.empty()) {
        continue;
      }
      if (text.size() >= prefix.size()
          && std::string_view(text).starts_with(prefix)
          && (activeProvider == nullptr || prefix.size() > activeProvider->prefix().size())) {
        activeProvider = provider.get();
        queryText = std::string_view(text).substr(prefix.size());
      }
    }
    // Trim leading space after prefix
    if (activeProvider != nullptr && !queryText.empty() && queryText.front() == ' ') {
      queryText = queryText.substr(1);
    }

    const bool typedQuery = !queryText.empty();
    const bool sortByUsage = m_config != nullptr && m_config->config().shell.launcher.sortByUsage;

    auto applyUsageBoost = [&](std::vector<LauncherResult>& results, const LauncherProvider& provider) {
      if (!sortByUsage) {
        return;
      }
      for (auto& result : results) {
        const int usageCount = m_usageTracker.getCount(provider.id(), result.id);
        result.score += usageBoostForScore(result.score, usageCount, typedQuery);
        result.recentlyUsedIndex = m_usageTracker.getRecentlyUsedIndex(provider.id(), result.id);
      }
    };

    if (activeProvider != nullptr) {
      m_allResults = activeProvider->queryPrefixed(queryText);
      anyProviderLoading = activeProvider->isLoading();
      if (activeProvider->trackUsage()) {
        applyUsageBoost(m_allResults, *activeProvider);
        if (sortByUsage && m_usageTracker.getRecentlyUsedCount(activeProvider->id()) > 0) {
          hasRecentlyUsed = true;
        }
      }
      for (auto& result : m_allResults) {
        result.providerId = activeProvider->id();
      }
      sortResultsByScore(m_allResults);
      newCategories = activeProvider->categories();
    } else if (startsWithLauncherPrefix(text)) {
      m_allResults = providerOverviewResults(text);
    } else {
      // Query default providers (empty prefix), plus prefixed providers that opt into global search.
      // Prefixed opt-in providers (e.g. Session) only contribute once the query is long enough,
      // so opening the launcher with no/short input does not flood it with their entries.
      const bool allowGlobalOptIn =
          StringUtils::trimRightView(StringUtils::trimLeftView(queryText)).size() >= kGlobalOptInMinChars;
      const bool emptyQuery = StringUtils::trimRightView(StringUtils::trimLeftView(queryText)).empty();
      for (auto& provider : m_providers) {
        const bool isDefault = provider->prefix().empty();
        const bool optedIn =
            provider->includeInGlobalSearch() && (allowGlobalOptIn || (emptyQuery && provider->listsWithoutQuery()));
        if (!isDefault && !optedIn) {
          continue;
        }
        auto results = provider->query(queryText);
        if (provider->isLoading()) {
          anyProviderLoading = true;
        }
        if (provider->trackUsage()) {
          applyUsageBoost(results, *provider);
          if (sortByUsage && m_usageTracker.getRecentlyUsedCount(provider->id()) > 0) {
            hasRecentlyUsed = true;
          }
        }
        for (auto& result : results) {
          result.providerId = provider->id();
        }
        m_allResults.insert(
            m_allResults.end(), std::make_move_iterator(results.begin()), std::make_move_iterator(results.end())
        );
        auto providerCats = provider->categories();
        for (auto& cat : providerCats) {
          newCategories.push_back(std::move(cat));
        }
      }
      sortResultsByScore(m_allResults);
    }
  }

  const int iconTargetSize = static_cast<int>(
      std::round(launcherIconSize(
          launcherListStyleFrom(m_config, contentScale(), panelCardOpacity(), m_commandStyle, m_sheetStyle)
      ))
  );
  for (auto& result : m_allResults) {
    if (result.iconPath.empty() && !result.iconName.empty()) {
      const std::string& resolved = m_iconResolver.resolve(result.iconName, iconTargetSize);
      if (!resolved.empty()) {
        result.iconPath = resolved;
      } else if (result.iconName != "application-x-executable") {
        const std::string& fallback = m_iconResolver.resolve("application-x-executable", iconTargetSize);
        if (!fallback.empty()) {
          result.iconPath = fallback;
        }
      }
      result.iconName.clear();
    }
  }

  updatePinnedApplicationState();

  bool categoriesChanged = newCategories.size() != m_currentCategories.size();
  if (!categoriesChanged) {
    for (std::size_t i = 0; i < newCategories.size(); ++i) {
      if (newCategories[i].label != m_currentCategories[i].label) {
        categoriesChanged = true;
        break;
      }
    }
  }

  if (hasRecentlyUsed != m_hasRecentlyUsed) {
    m_hasRecentlyUsed = hasRecentlyUsed;
    categoriesChanged = true;
  }

  if (categoriesChanged) {
    m_activeCategoryType = All;
    m_activeCategory.clear();
    rebuildCategoryFilter(newCategories);
  }

  if (text.empty() && m_scopedProviderId.empty()) {
    applyPinnedApplicationOrder();
  }

  m_anyProviderLoading = anyProviderLoading;

  applyActiveCategory();
}

void LauncherPanel::applyPinnedApplicationOrder() {
  if (m_config == nullptr || m_config->config().shell.launcher.pinned.empty()) {
    return;
  }

  const auto pinnedEntries = shell::dock::pinned_apps::resolveEntries(m_config->config().shell.launcher.pinned);
  if (pinnedEntries.empty()) {
    return;
  }

  std::vector<std::string> pinnedPaths;
  pinnedPaths.reserve(pinnedEntries.size());
  for (const DesktopEntry& entry : pinnedEntries) {
    if (!entry.path.empty()) {
      pinnedPaths.push_back(entry.path);
    }
  }

  std::ranges::stable_sort(m_allResults, [&pinnedPaths](const LauncherResult& a, const LauncherResult& b) {
    const auto rank = [&pinnedPaths](const LauncherResult& result) {
      if (result.providerId != kApplicationsProviderId) {
        return pinnedPaths.size();
      }
      const auto it = std::ranges::find(pinnedPaths, result.desktopEntryPath);
      return it == pinnedPaths.end() ? pinnedPaths.size() : static_cast<std::size_t>(it - pinnedPaths.begin());
    };
    return rank(a) < rank(b);
  });
}

void LauncherPanel::updatePinnedApplicationState() {
  for (LauncherResult& result : m_allResults) {
    result.pinned = false;
  }
  if (m_config == nullptr || m_config->config().shell.launcher.pinned.empty()) {
    return;
  }

  const auto pinnedEntries = shell::dock::pinned_apps::resolveEntries(m_config->config().shell.launcher.pinned);
  for (LauncherResult& result : m_allResults) {
    if (result.providerId != kApplicationsProviderId) {
      continue;
    }
    result.pinned = std::ranges::any_of(pinnedEntries, [&result](const DesktopEntry& entry) {
      return entry.path == result.desktopEntryPath;
    });
  }
}

void LauncherPanel::reorderPinnedApplication(std::string_view sourcePath, std::string_view targetPath) {
  if (m_config == nullptr || sourcePath.empty() || targetPath.empty() || sourcePath == targetPath) {
    return;
  }

  std::vector<std::string> pinned = m_config->config().shell.launcher.pinned;
  const auto entries = shell::dock::pinned_apps::resolveEntries(pinned);
  const auto indexForPath = [&entries](std::string_view path) -> std::optional<std::size_t> {
    const auto it = std::ranges::find(entries, path, &DesktopEntry::path);
    if (it == entries.end()) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(it - entries.begin());
  };

  const auto sourceIndex = indexForPath(sourcePath);
  const auto targetIndex = indexForPath(targetPath);
  if (!sourceIndex.has_value() || !targetIndex.has_value() || *sourceIndex == *targetIndex) {
    return;
  }

  std::string moved = std::move(pinned[*sourceIndex]);
  pinned.erase(pinned.begin() + static_cast<std::ptrdiff_t>(*sourceIndex));
  pinned.insert(pinned.begin() + static_cast<std::ptrdiff_t>(*targetIndex), std::move(moved));

  if (m_config->setOverride({"shell", "launcher", "pinned"}, std::move(pinned))) {
    reapplyCurrentQuery();
  }
}

void LauncherPanel::rebuildCategoryFilter(const std::vector<LauncherCategory>& categories) {
  m_currentCategories = categories;
  m_categoryFilterSlots.clear();
  if (categories.empty() && !m_hasRecentlyUsed) {
    if (m_categoryFilter != nullptr) {
      m_categoryFilter->clearOptions();
      setCategoryFilterVisible(false);
    }
    return;
  }

  m_categoryFilterSlots.push_back({All, 0});
  if (m_hasRecentlyUsed) {
    m_categoryFilterSlots.push_back({RecentlyUsed, 0});
  }
  for (std::size_t i = 0; i < categories.size(); ++i) {
    m_categoryFilterSlots.push_back({Category, i});
  }

  if (m_categoryFilter == nullptr) {
    return;
  }

  m_categoryFilter->clearOptions();
  for (std::size_t i = 0; i < m_categoryFilterSlots.size(); ++i) {
    const auto& slot = m_categoryFilterSlots[i];
    switch (slot.type) {
    case All:
      m_categoryFilter->addOption("", "layout-grid");
      m_categoryFilter->setOptionTooltip(i, i18n::tr("launcher.categories.all"));
      break;
    case RecentlyUsed:
      m_categoryFilter->addOption("", "history");
      m_categoryFilter->setOptionTooltip(i, i18n::tr("launcher.categories.recently-used"));
      break;
    case Category:
      m_categoryFilter->addOption("", categories[slot.categoryIndex].glyphName);
      m_categoryFilter->setOptionTooltip(i, categories[slot.categoryIndex].label);
      break;
    }
  }
  m_categoryFilter->setSelectedIndex(0);
  m_categoryFilter->setOnChange([this](std::size_t idx) { setActiveCategorySlot(idx); });
  setCategoryFilterVisible(m_categoryFilterVisible);
}

void LauncherPanel::setActiveCategorySlot(std::size_t slotIndex) {
  if (slotIndex >= m_categoryFilterSlots.size()) {
    return;
  }

  const auto& slot = m_categoryFilterSlots[slotIndex];
  m_activeCategoryType = slot.type;
  if (slot.type == Category && slot.categoryIndex < m_currentCategories.size()) {
    m_activeCategory = m_currentCategories[slot.categoryIndex].label;
  } else {
    m_activeCategory.clear();
  }
  applyActiveCategory();
}

void LauncherPanel::setCategoryFilterVisible(bool visible) {
  if (m_categoryFilter == nullptr) {
    return;
  }
  const bool show = visible && !m_categoryFilterSlots.empty();
  m_categoryFilter->setVisible(show);
  m_categoryFilter->setParticipatesInLayout(show);
  if (m_container != nullptr) {
    m_container->markLayoutDirty();
  }
}

bool LauncherPanel::startsWithLauncherPrefix(std::string_view text) const {
  const std::string& prefix = m_config != nullptr ? m_config->config().shell.launcher.providerPrefix : "/";
  return text.starts_with(prefix);
}

std::vector<LauncherResult> LauncherPanel::providerOverviewResults(std::string_view text) const {
  std::string filter;
  if (startsWithLauncherPrefix(text)) {
    const std::string& prefix = m_config != nullptr ? m_config->config().shell.launcher.providerPrefix : "/";
    filter = StringUtils::toLower(StringUtils::trim(text.substr(prefix.size())));
  }

  std::vector<LauncherResult> results;
  results.reserve(m_providers.size());
  for (const auto& provider : m_providers) {
    const std::string_view prefix = provider->prefix();
    if (prefix.empty()) {
      continue;
    }

    const std::string title(provider->displayName());
    const std::string prefixText(prefix);
    const std::string searchable = StringUtils::toLower(title + " " + prefixText);
    const double score = filter.empty() ? 0.0 : FuzzyMatch::score(filter, searchable);
    if (!filter.empty() && !FuzzyMatch::isMatch(score)) {
      continue;
    }

    LauncherResult result;
    result.id = providerOverviewId(prefix);
    result.providerId = std::string(kProviderOverviewProviderId);
    result.title = title;
    result.subtitle = prefixText;
    result.glyphName = std::string(provider->defaultGlyphName());
    result.score = score;
    results.push_back(std::move(result));
  }

  if (!filter.empty()) {
    sortResultsByScore(results);
  }
  return results;
}

void LauncherPanel::applyActiveCategory() {
  m_results.clear();
  switch (m_activeCategoryType) {
  case All:
    m_results = m_allResults;
    break;
  case RecentlyUsed:
    std::ranges::copy_if(m_allResults, std::back_inserter(m_results), [](const LauncherResult& r) {
      return r.recentlyUsedIndex > 0;
    });
    std::ranges::sort(m_results, [](const LauncherResult& a, const LauncherResult& b) {
      return a.recentlyUsedIndex > b.recentlyUsedIndex
          || (a.recentlyUsedIndex == b.recentlyUsedIndex
              && std::tie(a.providerId, a.id) < std::tie(b.providerId, b.id));
    });
    break;
  case Category:
    for (const auto& r : m_allResults) {
      if (r.category == m_activeCategory) {
        m_results.push_back(r);
      }
    }
    break;
  }
  insertSectionHeaders();
  m_selectedIndex = 0;
  while (m_selectedIndex < m_results.size() && isSectionHeader(m_results[m_selectedIndex])) {
    ++m_selectedIndex;
  }
  refreshResults();
}

bool LauncherPanel::wantsSectionHeaders() const {
  // The command bar tags every row with its kind instead.
  if (m_commandStyle || (m_query.empty() && !m_sheetStyle) || !m_scopedProviderId.empty() || m_results.empty()) {
    return false;
  }
  if (m_results.front().providerId == kProviderOverviewProviderId) {
    return false;
  }
  if (std::ranges::any_of(m_results, isDetailPresentation)) {
    return false;
  }
  // App-only results go to the app grid when it is enabled; headers would break that layout.
  const bool appGrid = m_config != nullptr && m_config->config().shell.launcher.appGrid && m_launcherShowIcons;
  return !(appGrid && std::ranges::all_of(m_results, [](const LauncherResult& result) {
             return result.providerId == kApplicationsProviderId;
           }));
}

void LauncherPanel::insertSectionHeaders() {
  if (!wantsSectionHeaders()) {
    return;
  }

  std::vector<LauncherResult> grouped;
  grouped.reserve(m_results.size() + 8);
  const auto addHeader = [&grouped](std::string label) {
    LauncherResult header;
    header.providerId = std::string(kSectionHeaderProviderId);
    header.id = "section:" + std::to_string(grouped.size());
    header.title = std::move(label);
    grouped.push_back(std::move(header));
  };

  // The sheet has no top hit: every result sits in its kind's section, and the empty sheet keeps each short.
  const std::size_t first = m_sheetStyle ? 0 : 1;
  const std::size_t perSection = m_sheetStyle && m_query.empty() ? kSheetIdleRows : m_results.size();
  if (!m_sheetStyle) {
    addHeader(i18n::tr("launcher.sections.top-hit"));
    grouped.push_back(m_results.front());
  }

  // Remaining results grouped by provider; groups keep the order of their best match.
  std::vector<std::string_view> providerOrder;
  for (std::size_t i = first; i < m_results.size(); ++i) {
    if (!std::ranges::contains(providerOrder, std::string_view(m_results[i].providerId))) {
      providerOrder.emplace_back(m_results[i].providerId);
    }
  }
  for (const std::string_view providerId : providerOrder) {
    addHeader(m_sheetStyle ? sectionTitle(providerId) : providerDisplayName(providerId));
    std::size_t shown = 0;
    for (std::size_t i = first; i < m_results.size() && shown < perSection; ++i) {
      if (m_results[i].providerId == providerId) {
        grouped.push_back(m_results[i]);
        ++shown;
      }
    }
  }
  m_results = std::move(grouped);
}

std::string LauncherPanel::providerDisplayName(std::string_view providerId) const {
  for (const auto& provider : m_providers) {
    if (provider->id() == providerId) {
      return provider->displayName();
    }
  }
  return std::string(providerId);
}

bool LauncherPanel::shouldCollapse() const {
  if (sheetStyle()
      || panelPlacement() != PanelPlacement::Floating
      || !m_query.empty()
      || !m_scopedProviderId.empty()) {
    return false;
  }
  // With the app grid enabled, the empty launcher is a Launchpad-style app overview (Spotlight style only).
  return commandStyle() || m_config == nullptr || !m_config->config().shell.launcher.appGrid;
}

float LauncherPanel::preferredWidth() const {
  if (sheetStyle()) {
    return scaled(kSheetWidth);
  }
  return scaled(commandStyle() ? kCommandWidth : 540.0F);
}

float LauncherPanel::commandBodyHeight() const {
  // The list grows with its rows up to kCommandVisibleRows, then scrolls; a lone text result gets a reading area.
  if (shouldUseDetailPresentation()) {
    return kCommandDetailHeight;
  }
  const std::size_t rows = std::clamp<std::size_t>(m_results.size(), 1, kCommandVisibleRows);
  float height = kCommandListPadding * 2.0F
      + static_cast<float>(rows) * kCommandRowHeight
      + static_cast<float>(rows - 1) * kCommandRowGap;
  if (m_categoryFilter != nullptr && m_categoryFilter->visible() && contentScale() > 0.0F) {
    height += m_categoryFilter->height() / contentScale();
  }
  return height;
}

float LauncherPanel::preferredHeight() const {
  if (sheetStyle()) {
    return scaled(kSheetFallbackHeight); // attached as a sheet it runs the bar's length instead
  }
  if (commandStyle()) {
    if (shouldCollapse()) {
      return scaled(kCommandSearchHeight);
    }
    return scaled(kCommandSearchHeight + kCommandDividerThickness * 2.0F + commandBodyHeight() + kCommandFooterHeight);
  }
  if (shouldCollapse()) {
    return scaled(kSpotlightSearchHeight) + contentScale() * Style::panelPadding * 2.0F;
  }
  return scaled(kSpotlightExpandedHeight);
}

std::optional<float> LauncherPanel::floatingCornerRadius() const {
  if (sheetStyle()) {
    return std::nullopt;
  }
  if (commandStyle()) {
    return scaled(kCommandRadius);
  }
  if (shouldCollapse()) {
    return preferredHeight() * 0.5F;
  }
  return scaled(kSpotlightExpandedRadius);
}

std::optional<float> LauncherPanel::contentPadding() const {
  if (sheetStyle()) {
    return kSheetPadding;
  }
  if (commandStyle()) {
    return 0.0F; // dividers run edge to edge; the rows bring their own insets
  }
  return std::nullopt;
}

void LauncherPanel::queuePreferredSizeRelayout() {
  // Deferred: this can run inside the panel's first scene build, after the surface size was chosen.
  DeferredCall::callLater([]() {
    if (auto* manager = PanelManager::current(); manager != nullptr && manager->isOpenPanel("launcher")) {
      manager->relayoutActivePanelPreferredSize();
    }
  });
}

void LauncherPanel::syncCommandChrome() {
  if (!m_commandStyle || m_container == nullptr) {
    return;
  }
  const bool showHint = m_collapsed && m_query.empty() && m_scopedProviderId.empty();
  if (m_commandHint != nullptr) {
    m_commandHint->setVisible(showHint);
    m_commandHint->setParticipatesInLayout(showHint);
  }
  if (m_footerCount != nullptr) {
    const std::size_t count = m_results.size();
    m_footerCount->setText(
        count == 0 ? std::string{}
                   : i18n::tr(
                         count == 1 ? "launcher.footer.results" : "launcher.footer.results-plural", "count",
                         std::to_string(count)
                     )
    );
  }
  const bool hasResults = !m_results.empty();
  const bool hasActions = m_selectedIndex < m_results.size() && !m_results[m_selectedIndex].desktopEntryPath.empty();
  for (const auto& [hint, shown] :
       {std::pair{m_footerNavigate, m_results.size() > 1}, std::pair{m_footerOpen, hasResults},
        std::pair{m_footerActions, hasActions}}) {
    if (hint != nullptr) {
      hint->setVisible(shown);
      hint->setParticipatesInLayout(shown);
    }
  }
  const float height = preferredHeight();
  if (std::abs(height - m_commandHeight) >= 0.5F) {
    m_commandHeight = height;
    queuePreferredSizeRelayout();
  }
}

void LauncherPanel::syncCollapsedState() {
  const bool collapsed = shouldCollapse();
  if (collapsed == m_collapsed) {
    return;
  }
  m_collapsed = collapsed;
  for (Node* node : std::initializer_list<Node*>{m_body, m_searchDivider, m_footerDivider, m_footer}) {
    if (node != nullptr) {
      node->setVisible(!collapsed);
      node->setParticipatesInLayout(!collapsed);
    }
  }
  queuePreferredSizeRelayout();
}

void LauncherPanel::refreshResults() {
  uiAssertNotRendering("LauncherPanel::refreshResults");
  if (m_grid == nullptr || m_emptyLabel == nullptr) {
    return;
  }
  syncCollapsedState();

  syncLauncherViewLayout(nullptr);
  m_grid->notifyDataChanged();
  if (m_results.empty()) {
    m_grid->setSelectedIndex(std::nullopt);
    m_grid->scrollView().setScrollOffset(0.0F);
  } else {
    m_grid->setSelectedIndex(m_selectedIndex);
  }
  bindDetailResult();
  applyEmptyState();
  syncCommandChrome();
}

void LauncherPanel::applyEmptyState() {
  if (m_grid == nullptr || m_emptyLabel == nullptr) {
    return;
  }
  const bool empty = m_results.empty();
  const bool detail = !empty && shouldUseDetailPresentation();
  m_grid->setVisible(!empty && !detail);
  m_grid->setParticipatesInLayout(!empty && !detail);
  if (m_detailScroll != nullptr) {
    m_detailScroll->setVisible(detail);
    m_detailScroll->setParticipatesInLayout(detail);
  }
  m_emptyLabel->setVisible(empty);
  m_emptyLabel->setParticipatesInLayout(empty);
  if (empty) {
    if (m_anyProviderLoading && !m_query.empty()) {
      m_emptyLabel->setText(i18n::tr("launcher.empty.loading"));
    } else {
      m_emptyLabel->setText(
          m_query.empty() ? i18n::tr("launcher.empty.type-to-search") : i18n::tr("launcher.empty.no-results")
      );
    }
  }
}

bool LauncherPanel::shouldUseDetailPresentation() const {
  return m_results.size() == 1 && isDetailPresentation(m_results.front());
}

void LauncherPanel::bindDetailResult() {
  if (!shouldUseDetailPresentation()
      || m_detailScroll == nullptr
      || m_detailSubtitle == nullptr
      || m_detailBody == nullptr) {
    return;
  }

  const LauncherResult& result = m_results.front();
  const bool hasSubtitle = !result.subtitle.empty();
  m_detailSubtitle->setVisible(hasSubtitle);
  m_detailSubtitle->setParticipatesInLayout(hasSubtitle);
  m_detailSubtitle->setText(singleLinePreview(result.subtitle));
  m_detailBody->setText(result.title.empty() ? result.id : result.title);
  m_detailScroll->setScrollOffset(0.0F);
}

bool LauncherPanel::openAppActionsMenu(std::size_t index, float anchorX, float anchorY) {
  if (index >= m_results.size()) {
    return false;
  }
  const LauncherResult& base = m_results[index];

  const DesktopEntry* match = nullptr;
  for (const auto& e : desktopEntries()) {
    if (e.path == base.desktopEntryPath) {
      match = &e;
      break;
    }
  }
  if (match == nullptr) {
    return false;
  }

  WaylandConnection* wl = PanelManager::instance().wayland();
  RenderContext* rc = PanelManager::instance().renderContext();
  if (wl == nullptr || rc == nullptr) {
    return false;
  }

  const auto parentCtx = PanelManager::instance().fallbackPopupParentContext();
  if (!parentCtx.has_value()) {
    return false;
  }

  if (m_actionsMenu == nullptr) {
    m_actionsMenu = std::make_unique<ContextMenuPopup>(*wl, *rc);
  }

  std::vector<DesktopAction> actionsCopy = match->actions;
  const bool launcherPinned =
      m_config != nullptr && shell::dock::pinned_apps::containsEntry(m_config->config().shell.launcher.pinned, *match);
  const bool canPin = m_config != nullptr && !launcherPinned;
  const bool canUnpin = m_config != nullptr && launcherPinned;

  constexpr std::int32_t kActionOpen = -1;
  constexpr std::int32_t kActionPin = -2;
  constexpr std::int32_t kActionUnpin = -3;

  std::vector<ContextMenuControlEntry> entries;
  entries.reserve(actionsCopy.size() + 2);
  entries.push_back(
      ContextMenuControlEntry{
          .id = kActionOpen,
          .label = i18n::tr("launcher.context-menu.open"),
          .enabled = true,
          .separator = false,
          .hasSubmenu = false,
      }
  );
  // Desktop actions sit above pin/unpin so pin stays last in the menu.
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(actionsCopy.size()); ++i) {
    entries.push_back(
        ContextMenuControlEntry{
            .id = i,
            .label = actionsCopy[static_cast<std::size_t>(i)].name,
            .enabled = true,
            .separator = false,
            .hasSubmenu = false,
        }
    );
  }
  if (canPin) {
    entries.push_back(
        ContextMenuControlEntry{
            .id = kActionPin,
            .label = i18n::tr("launcher.context-menu.pin"),
            .enabled = true,
            .separator = false,
            .hasSubmenu = false,
        }
    );
  } else if (canUnpin) {
    entries.push_back(
        ContextMenuControlEntry{
            .id = kActionUnpin,
            .label = i18n::tr("launcher.context-menu.unpin"),
            .enabled = true,
            .separator = false,
            .hasSubmenu = false,
        }
    );
  }

  const float scale = contentScale();
  constexpr float kMenuWidth = 240.0F;
  const float minMenuWidth = kMenuWidth * scale;

  if (m_config != nullptr) {
    m_actionsMenu->setShadowConfig(m_config->config().shell.shadow);
  }
  PanelManager::instance().beginAttachedPopup(parentCtx->surface);
  PanelManager::instance().setActivePopup(m_actionsMenu.get());

  m_actionsMenu->setOnDismissed([parentSurface = parentCtx->surface]() {
    PanelManager::instance().clearActivePopup();
    PanelManager::instance().endAttachedPopup(parentSurface);
  });

  m_actionsMenu->setOnActivate([this, base, actionsCopy = std::move(actionsCopy),
                                entryForPin = *match](const ContextMenuControlEntry& entry) {
    LauncherResult result = base;
    if (entry.id == kActionPin) {
      if (m_config == nullptr
          || entryForPin.id.empty()
          || shell::dock::pinned_apps::containsEntry(m_config->config().shell.launcher.pinned, entryForPin)) {
        return;
      }
      std::vector<std::string> pinned = m_config->config().shell.launcher.pinned;
      pinned.push_back(entryForPin.id);
      if (m_config->setOverride({"shell", "launcher", "pinned"}, std::move(pinned))) {
        reapplyCurrentQuery();
      }
      return;
    }
    if (entry.id == kActionUnpin) {
      if (m_config == nullptr) {
        return;
      }
      std::vector<std::string> pinned = m_config->config().shell.launcher.pinned;
      shell::dock::pinned_apps::removeEntry(pinned, entryForPin);
      if (m_config->setOverride({"shell", "launcher", "pinned"}, std::move(pinned))) {
        reapplyCurrentQuery();
      }
      return;
    }
    if (entry.id >= 0 && entry.id < static_cast<std::int32_t>(actionsCopy.size())) {
      const DesktopAction& action = actionsCopy[static_cast<std::size_t>(entry.id)];
      result.id = AppProvider::actionResultId(result.desktopEntryPath, action.id);
      result.desktopActionId = action.id;
    } else if (entry.id != kActionOpen) {
      return;
    }

    for (auto& provider : m_providers) {
      if (provider->id() != std::string_view(result.providerId)) {
        continue;
      }
      if (!provider->activate(result)) {
        return;
      }
      finishActivation(*provider, result.id, provider->supportsAutoPaste());
      return;
    }
    return;
  });

  const float inset = std::round(std::max(4.0F, Style::spaceXs * scale));
  const auto ax = static_cast<std::int32_t>(std::round(anchorX - inset));
  const auto ay = static_cast<std::int32_t>(std::round(anchorY - inset));
  const auto aw = static_cast<std::int32_t>(std::round(inset * 2.0F));
  const auto ah = static_cast<std::int32_t>(std::round(inset * 2.0F));

  m_actionsMenu->open(
      ContextMenuPopupRequest{
          .entries = std::move(entries),
          .minMenuWidth = minMenuWidth,
          .maxMenuWidth = Style::menuAutoMaxWidth * scale,
          .maxVisible = 12,
          .anchor =
              PopupAnchorRect{
                  .x = ax,
                  .y = ay,
                  .width = std::max(1, aw),
                  .height = std::max(1, ah),
              },
          .parent = PopupSurfaceParent{
              .layerSurface = parentCtx->layerSurface,
              .output = parentCtx->output,
          },
      }
  );
  return true;
}

void LauncherPanel::activateAt(std::size_t index) {
  if (index >= m_results.size() || isSectionHeader(m_results[index])) {
    return;
  }
  m_selectedIndex = index;
  activateSelected();
}

void LauncherPanel::activateSelected() {
  if (m_selectedIndex >= m_results.size()) {
    return;
  }

  const auto& result = m_results[m_selectedIndex];
  if (isSectionHeader(result)) {
    return;
  }
  if (result.providerId == kProviderOverviewProviderId && result.id.starts_with(kProviderOverviewResultPrefix)) {
    std::string prefix = result.id.substr(kProviderOverviewResultPrefix.size());
    if (!prefix.empty()) {
      prefix += ' ';
    }
    if (m_input != nullptr) {
      m_input->setValue(prefix);
    }
    if (m_grid != nullptr) {
      m_grid->scrollView().setScrollOffset(0.0F);
    }
    onInputChanged(prefix);
    return;
  }

  // Dispatch only to the provider that produced this result. Providers can use
  // overlapping id shapes, so probing every provider risks side effects.
  for (auto& provider : m_providers) {
    if (provider->id() != std::string_view(result.providerId)) {
      continue;
    }

    if (!provider->activate(result)) {
      return;
    }

    finishActivation(*provider, result.id, provider->supportsAutoPaste());
    return;
  }
}

bool LauncherPanel::handleKeyEvent(std::uint32_t sym, std::uint32_t modifiers) {
  const bool gridNav = m_usingAppGrid && m_grid != nullptr;
  const int columns = gridNav ? static_cast<int>(std::max<std::size_t>(1, m_grid->layoutColumnCount())) : 1;

  const auto moveSelection = [this](int delta) {
    if (m_results.empty()) {
      return;
    }
    const int last = static_cast<int>(m_results.size() - 1);
    int next = std::clamp(static_cast<int>(m_selectedIndex) + delta, 0, last);
    const int step = delta < 0 ? -1 : 1;
    while (next >= 0 && next <= last && isSectionHeader(m_results[static_cast<std::size_t>(next)])) {
      next += step;
    }
    if (next < 0 || next > last || next == static_cast<int>(m_selectedIndex)) {
      return;
    }
    m_selectedIndex = static_cast<std::size_t>(next);
    if (m_grid != nullptr) {
      m_grid->setSelectedIndex(m_selectedIndex);
    }
    syncCommandChrome();
  };

  const auto cycleCategory = [this](bool reverse) {
    if (m_categoryFilter == nullptr) {
      return false;
    }
    const std::size_t total = m_categoryFilterSlots.size();
    if (total == 0) {
      return false;
    }

    const bool wasVisible = m_categoryFilter->visible();
    m_categoryFilterVisible = true;
    setCategoryFilterVisible(true);
    if (!wasVisible) {
      return true;
    }

    const std::size_t selected = std::min(m_categoryFilter->selectedIndex(), total - 1);
    const std::size_t next =
        reverse ? (selected == 0 ? total - 1 : selected - 1) : (selected + 1 < total ? selected + 1 : 0);
    m_categoryFilter->setSelectedIndex(next);
    return true;
  };

  if (sym == XKB_KEY_F6 && (modifiers & ~(KeyMod::Shift)) == 0) {
    return cycleCategory((modifiers & KeyMod::Shift) != 0);
  }

  if (KeySymbol::isPageUp(sym)) {
    const int stride = m_grid != nullptr ? static_cast<int>(m_grid->pageItemStride()) : 1;
    moveSelection(-stride);
    return true;
  }

  if (KeySymbol::isPageDown(sym)) {
    const int stride = m_grid != nullptr ? static_cast<int>(m_grid->pageItemStride()) : 1;
    moveSelection(stride);
    return true;
  }

  if (KeybindMatcher::matches(KeybindAction::Up, sym, modifiers)) {
    moveSelection(gridNav ? -columns : -1);
    return true;
  }

  if (KeybindMatcher::matches(KeybindAction::Down, sym, modifiers)) {
    moveSelection(gridNav ? columns : 1);
    return true;
  }

  if (gridNav && KeybindMatcher::matches(KeybindAction::Left, sym, modifiers)) {
    moveSelection(-1);
    return true;
  }

  if (gridNav && KeybindMatcher::matches(KeybindAction::Right, sym, modifiers)) {
    moveSelection(1);
    return true;
  }

  // Validate+Shift opens the app context menu (Shift layered on the configured
  // Validate chord). Menu navigation uses the Up/Down/Validate/Cancel keybinds.
  if ((modifiers & KeyMod::Shift) != 0
      && KeybindMatcher::matches(KeybindAction::Validate, sym, modifiers & ~KeyMod::Shift)) {
    float anchorX = 0.0F;
    float anchorY = 0.0F;
    if (m_grid == nullptr || !m_grid->absoluteAnchorForIndex(m_selectedIndex, anchorX, anchorY)) {
      if (m_grid != nullptr) {
        Node::absolutePosition(m_grid, anchorX, anchorY);
        anchorX += m_grid->width() * 0.5F;
        anchorY += m_grid->height() * 0.5F;
      }
    }
    return openAppActionsMenu(m_selectedIndex, anchorX, anchorY);
  }

  if (KeybindMatcher::matches(KeybindAction::Validate, sym, modifiers)) {
    activateSelected();
    return true;
  }

  return false;
}
