#include "shell/lockscreen/lockscreen_login_box.h"

#include "shell/desktop/desktop_widget_layout.h"
#include "shell/desktop/desktop_widget_settings_registry.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <unordered_set>

namespace lockscreen_login_box {

  namespace {

    [[nodiscard]] bool
    readBool(const std::unordered_map<std::string, WidgetSettingValue>& settings, std::string_view key, bool fallback) {
      const auto it = settings.find(std::string(key));
      if (it == settings.end()) {
        return fallback;
      }
      if (const auto* value = std::get_if<bool>(&it->second)) {
        return *value;
      }
      return fallback;
    }

    [[nodiscard]] std::string readString(
        const std::unordered_map<std::string, WidgetSettingValue>& settings, std::string_view key,
        std::string_view fallback
    ) {
      const auto it = settings.find(std::string(key));
      if (it == settings.end()) {
        return std::string(fallback);
      }
      if (const auto* value = std::get_if<std::string>(&it->second)) {
        return *value;
      }
      return std::string(fallback);
    }

    void clampOpacitySetting(std::unordered_map<std::string, WidgetSettingValue>& settings, std::string_view key) {
      const std::string keyStr(key);
      const auto it = settings.find(keyStr);
      if (it == settings.end()) {
        return;
      }
      if (const auto* doubleValue = std::get_if<double>(&it->second)) {
        settings.insert_or_assign(keyStr, std::clamp(*doubleValue, 0.0, 1.0));
        return;
      }
      if (const auto* intValue = std::get_if<std::int64_t>(&it->second)) {
        settings.insert_or_assign(keyStr, std::clamp(static_cast<double>(*intValue), 0.0, 1.0));
      }
    }

    void clampRadiusSetting(std::unordered_map<std::string, WidgetSettingValue>& settings, std::string_view key) {
      const std::string keyStr(key);
      const auto it = settings.find(keyStr);
      if (it == settings.end()) {
        return;
      }
      if (const auto* doubleValue = std::get_if<double>(&it->second)) {
        settings.insert_or_assign(keyStr, std::clamp(*doubleValue, 0.0, 32.0));
        return;
      }
      if (const auto* intValue = std::get_if<std::int64_t>(&it->second)) {
        settings.insert_or_assign(keyStr, std::clamp(static_cast<double>(*intValue), 0.0, 32.0));
      }
    }

    struct OutputSize {
      float width = 1920.0F;
      float height = 1080.0F;
      bool connected = false; // false: the output is not there, the size is a guess
    };

    [[nodiscard]] OutputSize outputSizeFor(const WaylandConnection& wayland, std::string_view outputName) {
      for (const auto& output : wayland.outputs()) {
        if (!output.done || output.output == nullptr || !output.hasUsableGeometry()) {
          continue;
        }
        if (desktop_widgets::outputKey(output) == outputName) {
          return {
              .width = desktop_widgets::outputLogicalWidth(output),
              .height = desktop_widgets::outputLogicalHeight(output),
              .connected = true,
          };
        }
      }
      return {};
    }

  } // namespace

  bool isLoginBoxWidget(const DesktopWidgetState& state) { return state.type == kWidgetType; }

  bool isLoginBoxWidgetType(std::string_view type) { return type == kWidgetType; }

  bool isLoginBoxWidgetId(std::string_view id) { return id.starts_with(kWidgetIdPrefix); }

  std::string widgetIdForOutput(std::string_view outputKey) { return std::format("{}{}", kWidgetIdPrefix, outputKey); }

  LayoutMode resolveLayout(std::string_view layout) {
    if (layout == kLayoutCompact) {
      return LayoutMode::Compact;
    }
    return LayoutMode::Regular;
  }

  LayoutMode resolveLayout(const std::unordered_map<std::string, WidgetSettingValue>& settings) {
    return resolveLayout(readString(settings, kLayoutKey, kLayoutRegular));
  }

  bool styleShowsInfoExtras(const LoginBoxStyle& style) noexcept { return style.showMedia || style.showWeather; }

  float defaultPanelWidth() { return kPillDefaultWidth; }

  float defaultPanelHeight(LayoutMode layout, bool /*showSessionButtons*/, bool /*showInfoExtras*/) {
    if (layout != LayoutMode::Regular) {
      return kPillHeight;
    }
    return kAvatarSize + kGroupGap + kNameLineHeight + kGroupGap + kPillHeight;
  }

  bool isLegacyCardBox(const DesktopWidgetState& state) noexcept { return state.boxWidth > kPillMaxWidth; }

  float resolvePanelWidth(float screenWidth, float boxWidth) {
    const float width =
        boxWidth > 0.0F && boxWidth <= kPillMaxWidth ? std::max(boxWidth, kPillMinWidth) : defaultPanelWidth();
    return std::min(width, std::max(kPillMinWidth, screenWidth - Style::spaceLg * 2.0F));
  }

  void defaultPanelSize(
      float /*screenWidth*/, float& boxWidth, float& boxHeight, LayoutMode layout, bool showSessionButtons,
      bool showInfoExtras
  ) {
    boxWidth = defaultPanelWidth();
    boxHeight = defaultPanelHeight(layout, showSessionButtons, showInfoExtras);
  }

  void clampPanelSize(
      float screenWidth, float& boxWidth, float& boxHeight, LayoutMode layout, bool showSessionButtons,
      bool showInfoExtras
  ) {
    // Editor resizes clamp to the pill range instead of snapping back to the default.
    boxWidth = resolvePanelWidth(screenWidth, std::min(boxWidth, kPillMaxWidth));
    boxHeight = defaultPanelHeight(layout, showSessionButtons, showInfoExtras);
  }

  void defaultPanelCenter(float screenWidth, float screenHeight, float& cx, float& cy, LayoutMode layout) {
    cx = std::round(screenWidth * 0.5F);
    cy = std::round(screenHeight * (1.0F - kGroupBottomInset) - defaultPanelHeight(layout) * 0.5F);
  }

  Color pillFillColor() { return rgba(1.0F, 1.0F, 1.0F, isLightPalette() ? 0.62F : 0.18F); }

  Color pillEdgeColor() { return colorForRole(ColorRole::OnSurface, 0.16F); }

  const DesktopWidgetState* findForOutput(const std::vector<DesktopWidgetState>& widgets, std::string_view outputKey) {
    for (const auto& widget : widgets) {
      if (!isLoginBoxWidget(widget)) {
        continue;
      }
      if (widget.outputName == outputKey) {
        return &widget;
      }
    }
    return nullptr;
  }

  LoginBoxStyle resolveStyle(const std::unordered_map<std::string, WidgetSettingValue>& settings) {
    LoginBoxStyle style;
    style.layout = resolveLayout(settings);
    style.centerPasswordText = readBool(settings, kCenterPasswordTextKey, style.centerPasswordText);
    style.showLoginButton = readBool(settings, kShowLoginButtonKey, style.showLoginButton);
    style.showCapsLock = readBool(settings, kShowCapsLockKey, style.showCapsLock);
    style.showKeyboardLayout = readBool(settings, kShowKeyboardLayoutKey, style.showKeyboardLayout);
    style.showSessionButtons = readBool(settings, kShowSessionButtonsKey, style.showSessionButtons);
    style.showMedia = readBool(settings, kShowMediaKey, style.showMedia);
    style.showWeather = readBool(settings, kShowWeatherKey, style.showWeather);
    style.showUnlockHint = readBool(settings, kShowUnlockHintKey, style.showUnlockHint);
    return style;
  }

  void applyDefaultSettings(
      std::unordered_map<std::string, WidgetSettingValue>& settings, desktop_settings::DesktopWidgetSettingsScope scope
  ) {
    if (scope == desktop_settings::DesktopWidgetSettingsScope::Widget) {
      settings.insert_or_assign(std::string(kLayoutKey), std::string(kLayoutRegular));
      settings.insert_or_assign(std::string(kShowSessionButtonsKey), true);
      settings.insert_or_assign(std::string(kShowMediaKey), true);
      settings.insert_or_assign(std::string(kShowWeatherKey), true);
      settings.insert_or_assign(std::string(kShowLoginButtonKey), true);
      settings.insert_or_assign(std::string(kShowCapsLockKey), true);
      settings.insert_or_assign(std::string(kShowKeyboardLayoutKey), true);
      settings.insert_or_assign(std::string(kShowUnlockHintKey), true);
      settings.insert_or_assign(std::string(kInputOpacityKey), 1.0);
      settings.insert_or_assign(std::string(kInputRadiusKey), 6.0);
      settings.insert_or_assign(std::string(kCenterPasswordTextKey), false);
    }
    if (scope == desktop_settings::DesktopWidgetSettingsScope::Background) {
      settings.insert_or_assign("background_color", std::string("surface_variant"));
      settings.insert_or_assign("background_opacity", 0.88);
      settings.insert_or_assign("background_radius", 12.0);
    }
  }

  void applyAllDefaultSettings(std::unordered_map<std::string, WidgetSettingValue>& settings) {
    applyDefaultSettings(settings, desktop_settings::DesktopWidgetSettingsScope::Widget);
    applyDefaultSettings(settings, desktop_settings::DesktopWidgetSettingsScope::Background);
  }

  void normalizeSettings(std::unordered_map<std::string, WidgetSettingValue>& settings) {
    if (!settings.contains(std::string(kLayoutKey))) {
      settings.insert_or_assign(std::string(kLayoutKey), std::string(kLayoutRegular));
    } else {
      const std::string layout = readString(settings, kLayoutKey, kLayoutRegular);
      if (layout != kLayoutCompact && layout != kLayoutRegular) {
        settings.insert_or_assign(std::string(kLayoutKey), std::string(kLayoutRegular));
      }
    }
    if (!settings.contains(std::string(kShowSessionButtonsKey))) {
      settings.insert_or_assign(std::string(kShowSessionButtonsKey), true);
    }
    if (!settings.contains(std::string(kShowMediaKey))) {
      settings.insert_or_assign(std::string(kShowMediaKey), true);
    }
    if (!settings.contains(std::string(kShowWeatherKey))) {
      settings.insert_or_assign(std::string(kShowWeatherKey), true);
    }
    if (!settings.contains(std::string(kShowLoginButtonKey))) {
      settings.insert_or_assign(std::string(kShowLoginButtonKey), true);
    }
    if (!settings.contains(std::string(kShowCapsLockKey))) {
      settings.insert_or_assign(std::string(kShowCapsLockKey), true);
    }
    if (!settings.contains(std::string(kShowKeyboardLayoutKey))) {
      settings.insert_or_assign(std::string(kShowKeyboardLayoutKey), true);
    }
    if (!settings.contains(std::string(kShowUnlockHintKey))) {
      settings.insert_or_assign(std::string(kShowUnlockHintKey), true);
    }
    if (!settings.contains(std::string(kCenterPasswordTextKey))) {
      settings.insert_or_assign(std::string(kCenterPasswordTextKey), false);
    }
    clampOpacitySetting(settings, "background_opacity");
    clampRadiusSetting(settings, "background_radius");
    clampOpacitySetting(settings, kInputOpacityKey);
    clampRadiusSetting(settings, kInputRadiusKey);
  }

  void ensureWidgets(std::vector<DesktopWidgetState>& widgets, const WaylandConnection& wayland) {
    std::unordered_set<std::string> outputsWithLoginBox;
    std::erase_if(widgets, [&](const DesktopWidgetState& widget) {
      if (!isLoginBoxWidget(widget)) {
        return false;
      }
      if (widget.outputName.empty() || outputsWithLoginBox.contains(widget.outputName)) {
        return true;
      }
      outputsWithLoginBox.insert(widget.outputName);
      return false;
    });

    for (auto& widget : widgets) {
      if (!isLoginBoxWidget(widget)) {
        continue;
      }
      widget.rotationRad = 0.0F;
      widget.type = std::string(kWidgetType);
      normalizeSettings(widget.settings);
      const LoginBoxStyle style = resolveStyle(widget.settings);
      const OutputSize screen = outputSizeFor(wayland, widget.outputName);
      // Keep a pill width and spot the user chose. The old boxed card's are restarted from the defaults; the height
      // always follows the layout.
      if (isLegacyCardBox(widget)) {
        // Only re-center on a connected output: a guessed size would park the box off-center for good once saved.
        // A disconnected output's legacy box is placed at render time and migrated when it is back.
        if (!screen.connected) {
          continue;
        }
        defaultPanelCenter(screen.width, screen.height, widget.cx, widget.cy, style.layout);
        widget.boxWidth = defaultPanelWidth();
      } else if (widget.boxWidth <= 0.0F) {
        widget.boxWidth = defaultPanelWidth();
      }
      clampPanelSize(screen.width, widget.boxWidth, widget.boxHeight, style.layout);
      desktop_widgets::clampStateToOutput(wayland, widget, widget.boxWidth, widget.boxHeight);
    }

    for (const auto& output : wayland.outputs()) {
      if (!output.done || output.output == nullptr || !output.hasUsableGeometry()) {
        continue;
      }
      const std::string outputKey = desktop_widgets::outputKey(output);
      if (outputsWithLoginBox.contains(outputKey)) {
        continue;
      }

      DesktopWidgetState widget;
      widget.id = widgetIdForOutput(outputKey);
      widget.type = std::string(kWidgetType);
      widget.outputName = outputKey;
      widget.rotationRad = 0.0F;
      widget.enabled = true;
      const float screenWidth = desktop_widgets::outputLogicalWidth(output);
      applyDefaultSettings(widget.settings, desktop_settings::DesktopWidgetSettingsScope::Widget);
      applyDefaultSettings(widget.settings, desktop_settings::DesktopWidgetSettingsScope::Background);
      const LoginBoxStyle style = resolveStyle(widget.settings);
      defaultPanelCenter(screenWidth, desktop_widgets::outputLogicalHeight(output), widget.cx, widget.cy, style.layout);
      defaultPanelSize(screenWidth, widget.boxWidth, widget.boxHeight, style.layout);
      widgets.insert(widgets.begin(), std::move(widget));
      outputsWithLoginBox.insert(outputKey);
    }

    bool anyEnabled = false;
    for (const auto& widget : widgets) {
      if (isLoginBoxWidget(widget) && widget.enabled) {
        anyEnabled = true;
        break;
      }
    }
    if (!anyEnabled) {
      for (auto& widget : widgets) {
        if (isLoginBoxWidget(widget)) {
          widget.enabled = true;
          break;
        }
      }
    }
  }

} // namespace lockscreen_login_box
