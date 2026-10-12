#pragma once

#include "config/config_types.h"
#include "render/core/color.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class WaylandConnection;

namespace desktop_settings {
  enum class DesktopWidgetSettingsScope;
}

namespace lockscreen_login_box {

  constexpr std::string_view kWidgetType = "login_box";
  constexpr std::string_view kWidgetIdPrefix = "lockscreen-login-box@";

  constexpr std::string_view kLayoutKey = "layout";
  constexpr std::string_view kLayoutCompact = "compact";
  constexpr std::string_view kLayoutRegular = "regular";
  constexpr std::string_view kShowSessionButtonsKey = "show_session_buttons";
  constexpr std::string_view kShowMediaKey = "show_media";
  constexpr std::string_view kShowWeatherKey = "show_weather";
  constexpr std::string_view kInputOpacityKey = "input_opacity";
  constexpr std::string_view kInputRadiusKey = "input_radius";
  constexpr std::string_view kCenterPasswordTextKey = "center_password_text";
  constexpr std::string_view kShowLoginButtonKey = "show_login_button";
  constexpr std::string_view kShowCapsLockKey = "show_caps_lock";
  constexpr std::string_view kShowKeyboardLayoutKey = "show_keyboard_layout";
  constexpr std::string_view kShowUnlockHintKey = "show_unlock_hint";

  enum class LayoutMode : std::uint8_t {
    Compact,
    Regular,
  };

  // Settings of the old boxed card. The glass look reads none of them: normalizing drops them and the
  // config validator accepts them silently, so configs written before the redesign load without warnings.
  constexpr std::string_view kLegacyStyleKeys[] = {
      "background_color", "background_opacity", "background_radius", kInputOpacityKey, kInputRadiusKey,
  };

  struct LoginBoxStyle {
    LayoutMode layout = LayoutMode::Regular;
    bool centerPasswordText = false;
    bool showLoginButton = true;
    bool showCapsLock = true;
    bool showKeyboardLayout = true;
    bool showSessionButtons = true;
    bool showMedia = true;
    bool showWeather = true;
    bool showUnlockHint = true;
  };

  [[nodiscard]] bool isLoginBoxWidget(const DesktopWidgetState& state);
  [[nodiscard]] bool isLoginBoxWidgetType(std::string_view type);
  [[nodiscard]] bool isLoginBoxWidgetId(std::string_view id);
  [[nodiscard]] std::string widgetIdForOutput(std::string_view outputKey);

  [[nodiscard]] LayoutMode resolveLayout(const std::unordered_map<std::string, WidgetSettingValue>& settings);
  [[nodiscard]] LayoutMode resolveLayout(std::string_view layout);

  // The login box is the macOS Tahoe user block: a round avatar, the display name and a glass password pill,
  // stacked and centered (Compact keeps only the pill). Its width is the pill width. Session buttons sit in the
  // bottom-right corner and media and weather next to the clock, so they no longer size the box.
  constexpr float kAvatarSize = 64.0F;
  constexpr float kNameFontSize = 15.0F;
  constexpr float kNameLineHeight = 20.0F;
  constexpr float kGroupGap = 12.0F;
  constexpr float kPillHeight = 30.0F;
  constexpr float kPillDefaultWidth = 220.0F;
  constexpr float kPillMinWidth = 180.0F;
  constexpr float kPillMaxWidth = 360.0F;
  constexpr float kSubmitButtonSize = 22.0F;
  // Gap between the pill and the status hint under it.
  constexpr float kHintGap = 10.0F;
  // The block's bottom edge rests this fraction of the output height above the bottom.
  constexpr float kGroupBottomInset = 0.13F;

  [[nodiscard]] float defaultPanelWidth();
  // The session/info flags are accepted for the editor's call sites; neither changes the block anymore.
  [[nodiscard]] float defaultPanelHeight(LayoutMode layout, bool showSessionButtons = true, bool showInfoExtras = true);
  // Login boxes wider than the pill range were sized and placed for the old boxed card.
  [[nodiscard]] bool isLegacyCardBox(const DesktopWidgetState& state) noexcept;
  // Widths past kPillMaxWidth fall back to the default pill.
  [[nodiscard]] float resolvePanelWidth(float screenWidth, float boxWidth);
  void defaultPanelSize(
      float screenWidth, float& boxWidth, float& boxHeight, LayoutMode layout, bool showSessionButtons = true,
      bool showInfoExtras = true
  );
  void clampPanelSize(
      float screenWidth, float& boxWidth, float& boxHeight, LayoutMode layout, bool showSessionButtons = true,
      bool showInfoExtras = true
  );
  void defaultPanelCenter(float screenWidth, float screenHeight, float& cx, float& cy, LayoutMode layout);

  // Glass fill and edge of the password pill. The field draws its text in the palette's on-surface color, so a
  // light palette gets denser glass to keep that dark text legible.
  [[nodiscard]] Color pillFillColor();
  [[nodiscard]] Color pillEdgeColor();

  // Whether media or weather is configured; the editor passes it to the sizing helpers.
  [[nodiscard]] bool styleShowsInfoExtras(const LoginBoxStyle& style) noexcept;

  [[nodiscard]] const DesktopWidgetState*
  findForOutput(const std::vector<DesktopWidgetState>& widgets, std::string_view outputKey);

  [[nodiscard]] LoginBoxStyle resolveStyle(const std::unordered_map<std::string, WidgetSettingValue>& settings);
  void applyDefaultSettings(
      std::unordered_map<std::string, WidgetSettingValue>& settings, desktop_settings::DesktopWidgetSettingsScope scope
  );
  void applyAllDefaultSettings(std::unordered_map<std::string, WidgetSettingValue>& settings);
  void normalizeSettings(std::unordered_map<std::string, WidgetSettingValue>& settings);

  void ensureWidgets(std::vector<DesktopWidgetState>& widgets, const WaylandConnection& wayland);

} // namespace lockscreen_login_box
