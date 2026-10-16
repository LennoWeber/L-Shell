#include "shell/desktop/widgets/desktop_login_box_widget.h"

#include "render/core/renderer.h"
#include "render/scene/node.h"
#include "shell/lockscreen/lockscreen_login_box.h"
#include "ui/builders.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>

namespace {

  constexpr float kAvatarGlyphSize = lockscreen_login_box::kAvatarSize * 0.55F;
  constexpr float kSubmitGlyphSize = 14.0F;
  // Bars standing in for the display name and for the status hint under the pill.
  constexpr float kNameBarWidth = 96.0F;
  constexpr float kNameBarHeight = 10.0F;
  constexpr float kHintBarWidth = 140.0F;
  constexpr float kHintBarHeight = 8.0F;

  [[nodiscard]] bool isStyleSetting(std::string_view key) {
    return key == "background_color"
        || key == "background_opacity"
        || key == "background_radius"
        || key == lockscreen_login_box::kLayoutKey
        || key == lockscreen_login_box::kShowSessionButtonsKey
        || key == lockscreen_login_box::kShowMediaKey
        || key == lockscreen_login_box::kShowWeatherKey
        || key == lockscreen_login_box::kShowLoginButtonKey
        || key == lockscreen_login_box::kShowCapsLockKey
        || key == lockscreen_login_box::kShowUnlockHintKey
        || key == lockscreen_login_box::kInputOpacityKey
        || key == lockscreen_login_box::kInputRadiusKey;
  }

  [[nodiscard]] Color white(float alpha) { return rgba(1.0F, 1.0F, 1.0F, alpha); }

} // namespace

void DesktopLoginBoxWidget::create() {
  auto rootNode = ui::node({});

  rootNode->addChild(ui::box({.out = &m_avatarGhost}));
  rootNode->addChild(
      ui::glyph({
          .out = &m_avatarGlyph,
          .glyph = "user-filled",
          .glyphSize = kAvatarGlyphSize,
          .color = fixedColorSpec(white(0.9F)),
      })
  );
  rootNode->addChild(ui::box({.out = &m_nameGhost}));
  rootNode->addChild(ui::box({.out = &m_pillGhost}));
  rootNode->addChild(ui::box({.out = &m_submitGhost}));
  rootNode->addChild(
      ui::glyph({
          .out = &m_submitGlyph,
          .glyph = "arrow-right",
          .glyphSize = kSubmitGlyphSize,
          .color = colorSpecFromRole(ColorRole::OnSurface, 0.9F),
      })
  );
  rootNode->addChild(ui::box({.out = &m_hintGhost}));

  setRoot(std::move(rootNode));
}

void DesktopLoginBoxWidget::layout(Renderer& renderer) {
  DesktopWidget::layout(renderer);
  // Unlock-hint ghost sits above/below the panel (outside the box); don't clip it away.
  if (Node* outer = presentationRoot()) {
    outer->setClipChildren(false);
  }
}

void DesktopLoginBoxWidget::setSettings(const std::unordered_map<std::string, WidgetSettingValue>& settings) {
  m_settings = settings;
  lockscreen_login_box::normalizeSettings(m_settings);
}

bool DesktopLoginBoxWidget::applySetting(
    const std::string& key, const WidgetSettingValue& value,
    const std::unordered_map<std::string, WidgetSettingValue>& allSettings, Renderer& renderer
) {
  (void)value;
  m_settings = allSettings;
  lockscreen_login_box::normalizeSettings(m_settings);
  if (!isStyleSetting(key)) {
    return false;
  }
  doLayout(renderer);
  if (Node* outer = presentationRoot()) {
    outer->setClipChildren(false);
  }
  return true;
}

void DesktopLoginBoxWidget::doLayout(Renderer& renderer) {
  namespace login = lockscreen_login_box;
  const float screenWidth = m_screenWidth > 0.0F ? m_screenWidth : 1920.0F;
  const float screenHeight = m_screenHeight > 0.0F ? m_screenHeight : 1080.0F;
  const login::LoginBoxStyle style = login::resolveStyle(m_settings);
  const bool regular = style.layout == login::LayoutMode::Regular;
  const float width = login::resolvePanelWidth(screenWidth, m_boxWidth);
  const float height = login::defaultPanelHeight(style.layout);

  const float avatarX = std::round((width - login::kAvatarSize) * 0.5F);
  m_avatarGhost->setVisible(regular);
  m_avatarGlyph->setVisible(regular);
  m_nameGhost->setVisible(regular);
  if (regular) {
    m_avatarGhost->setPosition(avatarX, 0.0F);
    m_avatarGhost->setSize(login::kAvatarSize, login::kAvatarSize);
    m_avatarGhost->setStyle(
        RoundedRectStyle{
            .fill = white(0.22F),
            .fillMode = FillMode::Solid,
            .radius = login::kAvatarSize * 0.5F,
        }
    );
    m_avatarGlyph->setPosition(
        avatarX + (login::kAvatarSize - kAvatarGlyphSize) * 0.5F, (login::kAvatarSize - kAvatarGlyphSize) * 0.5F
    );
    m_avatarGlyph->measure(renderer);

    const float nameWidth = std::min(kNameBarWidth, width);
    m_nameGhost->setPosition(
        std::round((width - nameWidth) * 0.5F),
        login::kAvatarSize + login::kGroupGap + (login::kNameLineHeight - kNameBarHeight) * 0.5F
    );
    m_nameGhost->setSize(nameWidth, kNameBarHeight);
    m_nameGhost->setStyle(
        RoundedRectStyle{.fill = white(0.7F), .fillMode = FillMode::Solid, .radius = kNameBarHeight * 0.5F}
    );
  }

  const float pillY = height - login::kPillHeight;
  m_pillGhost->setPosition(0.0F, pillY);
  m_pillGhost->setSize(width, login::kPillHeight);
  m_pillGhost->setStyle(
      RoundedRectStyle{
          .fill = login::pillFillColor(),
          .border = login::pillEdgeColor(),
          .fillMode = FillMode::Solid,
          .radius = login::kPillHeight * 0.5F,
          .softness = 1.0F,
          .borderWidth = Style::borderWidth,
      }
  );

  m_submitGhost->setVisible(style.showLoginButton);
  m_submitGlyph->setVisible(style.showLoginButton);
  if (style.showLoginButton) {
    const float inset = (login::kPillHeight - login::kSubmitButtonSize) * 0.5F;
    const float buttonX = width - inset - login::kSubmitButtonSize;
    const float buttonY = pillY + inset;
    m_submitGhost->setPosition(buttonX, buttonY);
    m_submitGhost->setSize(login::kSubmitButtonSize, login::kSubmitButtonSize);
    m_submitGhost->setStyle(
        RoundedRectStyle{
            .fill = colorForRole(ColorRole::OnSurface, 0.14F),
            .fillMode = FillMode::Solid,
            .radius = login::kSubmitButtonSize * 0.5F,
        }
    );
    m_submitGlyph->setPosition(
        buttonX + (login::kSubmitButtonSize - kSubmitGlyphSize) * 0.5F,
        buttonY + (login::kSubmitButtonSize - kSubmitGlyphSize) * 0.5F
    );
    m_submitGlyph->measure(renderer);
  }

  // The status hint sits under the pill, outside the box, and flips above the block near the bottom edge.
  m_hintGhost->setVisible(style.showUnlockHint);
  if (style.showUnlockHint) {
    const float centerY = m_screenHeight > 0.0F ? m_panelCenterY : screenHeight * 0.5F;
    const float blockBottom = centerY + height * 0.5F;
    const bool placeBelow = blockBottom + login::kHintGap + kHintBarHeight <= screenHeight - Style::spaceLg;
    const float hintWidth = std::min(kHintBarWidth, width);
    m_hintGhost->setPosition(
        std::round((width - hintWidth) * 0.5F),
        placeBelow ? height + login::kHintGap : -(login::kHintGap + kHintBarHeight)
    );
    m_hintGhost->setSize(hintWidth, kHintBarHeight);
    m_hintGhost->setZIndex(2);
    m_hintGhost->setStyle(
        RoundedRectStyle{.fill = white(0.45F), .fillMode = FillMode::Solid, .radius = kHintBarHeight * 0.5F}
    );
  }

  if (Node* rootNode = root()) {
    rootNode->setSize(width, height);
  }
}
