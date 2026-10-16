#include "shell/lockscreen/lock_surface.h"

#include "capture/screencopy_capture.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "dbus/mpris/mpris_art.h"
#include "dbus/mpris/mpris_service.h"
#include "ext-session-lock-v1-client-protocol.h"
#include "i18n/i18n.h"
#include "render/animation/motion_service.h"
#include "render/core/blur_cache.h"
#include "render/core/render_styles.h"
#include "render/core/shared_texture_cache.h"
#include "render/render_context.h"
#include "render/scene/wallpaper_node.h"
#include "shell/lockscreen/lockscreen_login_box.h"
#include "shell/lockscreen/lockscreen_session_actions.h"
#include "shell/lockscreen/lockscreen_widgets_host.h"
#include "shell/profile/avatar_path.h"
#include "shell/session/session_action_meta.h"
#include "shell/session/session_action_runner.h"
#include "system/distro_info.h"
#include "system/weather_service.h"
#include "time/time_format.h"
#include "ui/builders.h"
#include "ui/controls/button.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"
#include "ui/controls/lockscreen_transition_cover.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "util/clamp.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <numbers>
#include <string_view>
#include <system_error>
#include <utility>
#include <wayland-client-core.h>

namespace {

  constexpr Logger kLog("lockscreen");
  // Unlock runs at this share of the lock transition's duration.
  constexpr float kUnlockDurationScale = 0.7F;

  const ext_session_lock_surface_v1_listener kLockSurfaceListener = {
      .configure = &LockSurface::handleConfigure,
  };

  namespace login = lockscreen_login_box;

  // macOS Tahoe lock screen: a date line over a display-size clock at the top, the user block at the bottom.
  constexpr float kClockFontSize = 104.0F;
  constexpr float kDateFontSize = 20.0F;
  constexpr float kWeatherGlyphSize = 18.0F;
  // Pulls the date down onto the clock's cap height instead of a full line box above it.
  constexpr float kDateClockGap = -14.0F;
  // Top of the date line, as a fraction of the output height.
  constexpr float kClockTopInset = 0.06F;
  // The clock is sized for ~900 logical px of height; shorter outputs (1280x720, 2x HiDPI laptops) shrink it so
  // it keeps clear of the user block.
  constexpr float kClockReferenceHeight = 900.0F;
  constexpr float kClockMinScale = 0.6F;
  // Corner clock (Monolith): a huge time resting in the bottom-left corner under a small versal date.
  constexpr float kCornerClockFontSize = 168.0F;
  constexpr float kCornerDateFontSize = 13.0F;
  constexpr float kCornerDateTracking = 0.12F;
  constexpr float kCornerClockInset = 64.0F;
  constexpr float kNowPlayingArtSize = 28.0F;
  constexpr float kNowPlayingMaxWidth = 360.0F;
  constexpr float kAvatarGlyphSize = login::kAvatarSize * 0.55F;
  constexpr float kPillTextInset = 14.0F;
  constexpr float kPillFocusRingWidth = 1.5F;
  constexpr float kSubmitGlyphSize = 14.0F;
  constexpr float kCapsLockGlyphSize = 14.0F;
  constexpr float kStatusMaxWidth = 420.0F;
  constexpr float kCornerButtonSize = 28.0F;
  constexpr float kCornerGlyphSize = 14.0F;
  constexpr float kCornerInset = 24.0F;
  constexpr float kLayoutChipMaxWidth = 120.0F;
  // A refused password swings the pill sideways a few times, decaying, like macOS.
  constexpr float kShakeAmplitude = 10.0F;
  constexpr float kShakeSwings = 3.0F;
  constexpr float kShakeDurationMs = 420.0F;

  bool parseColorWallpaperPath(std::string_view path, Color& out) {
    constexpr std::string_view kPrefix = "color:";
    if (!path.starts_with(kPrefix)) {
      return false;
    }
    return tryParseHexColor(path.substr(kPrefix.size()), out);
  }

  [[nodiscard]] Color white(float alpha) { return rgba(1.0F, 1.0F, 1.0F, alpha); }

  // Text and glass on the lock screen: white over the dimmed wallpaper, or the palette's ink straight on a paper
  // wallpaper (lockscreen.ink).
  [[nodiscard]] ColorSpec tone(bool ink, float alpha) {
    return ink ? colorSpecFromRole(ColorRole::OnSurface, alpha) : fixedColorSpec(white(alpha));
  }

  // The lock screen must hide the session in every pixel: the blurred wallpaper fades out at the screen edges
  // and a fill color may be translucent, so the base under it is always opaque (the fill over black).
  [[nodiscard]] Color opaqueBackdropColor(const Color& fill) {
    return rgba(fill.r * fill.a, fill.g * fill.a, fill.b * fill.a, 1.0F);
  }

  // Neutral glass for controls that sit directly on the dimmed wallpaper.
  [[nodiscard]] Button::ButtonPalette wallpaperGlassPalette(bool ink) {
    const auto state = [ink](float fill, float label) {
      return Button::ButtonStateColors{
          .bg = fixedColorSpec(white(fill)),
          .border = clearColorSpec(),
          .label = fixedColorSpec(white(label)),
      };
    };
    return Button::ButtonPalette{
        .normal = state(0.16F, 0.9F),
        .hover = state(0.26F, 1.0F),
        .pressed = state(0.34F, 1.0F),
        .disabled = state(0.08F, 0.4F),
    };
  }

  // The submit arrow follows the field's text color, so it reads on the pill whichever glass density it uses.
  [[nodiscard]] Button::ButtonPalette pillButtonPalette() {
    const auto state = [](float fill, float label) {
      return Button::ButtonStateColors{
          .bg = colorSpecFromRole(ColorRole::OnSurface, fill),
          .border = clearColorSpec(),
          .label = colorSpecFromRole(ColorRole::OnSurface, label),
      };
    };
    return Button::ButtonPalette{
        .normal = state(0.14F, 0.9F),
        .hover = state(0.24F, 1.0F),
        .pressed = state(0.32F, 1.0F),
        .disabled = state(0.06F, 0.35F),
    };
  }

  // shell.time_format picks 12 or 24 hours like every other clock. The AM/PM marker is dropped because the lock
  // clock is a display-size numeral, as on macOS.
  [[nodiscard]] std::string lockClockFormat(const ConfigService* config) {
    std::string format = config != nullptr && !config->config().shell.timeFormat.empty()
        ? config->config().shell.timeFormat
        : std::string("{:%H:%M}");
    for (const std::string_view marker : {" %p", "%p", " %P", "%P"}) {
      for (auto pos = format.find(marker); pos != std::string::npos; pos = format.find(marker)) {
        format.erase(pos, marker.size());
      }
    }
    return format;
  }

  // A clock lockscreen widget on this output (the way to get a lock-screen clock before L-Shell drew its own)
  // replaces the built-in one, so the screen never shows two clocks.
  [[nodiscard]] bool outputHasClockWidget(const ConfigService* config, std::string_view outputKey) {
    if (config == nullptr || !config->config().lockscreenWidgets.enabled) {
      return false;
    }
    return std::ranges::any_of(config->config().lockscreenWidgets.widgets, [outputKey](const DesktopWidgetState& w) {
      return w.enabled && w.type == "clock" && (w.outputName.empty() || w.outputName == outputKey);
    });
  }

  [[nodiscard]] bool formatShowsSeconds(const std::string& format) {
    return format.contains("%S") || format.contains("%T") || format.contains("%X");
  }

  // shell.avatar_path first (the avatar Settings shows), then the AccountsService icon and ~/.face that display
  // managers read: the lock screen has no AccountsService connection of its own.
  [[nodiscard]] std::string resolveAvatarPath(const ConfigService* config, const std::string& user) {
    if (config != nullptr) {
      if (std::string path = shell::resolvedAvatarPath(nullptr, config->config()); !path.empty()) {
        return path;
      }
    }
    std::error_code ec;
    if (!user.empty()) {
      const auto icon = std::filesystem::path("/var/lib/AccountsService/icons") / user;
      if (std::filesystem::is_regular_file(icon, ec)) {
        return icon.string();
      }
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
      const auto face = std::filesystem::path(home) / ".face";
      if (std::filesystem::is_regular_file(face, ec)) {
        return face.string();
      }
    }
    return {};
  }

} // namespace

LockSurface::LockSurface(WaylandConnection& connection, ConfigService* config) : Surface(connection), m_config(config) {
  {
    auto backgroundLayer = ui::node({});
    backgroundLayer->setZIndex(0);
    m_backgroundLayer = m_root.addChild(std::move(backgroundLayer));
  }

  auto wallpaper = std::make_unique<WallpaperNode>();
  m_wallpaper = static_cast<WallpaperNode*>(m_backgroundLayer->addChild(std::move(wallpaper)));
  m_wallpaper->setZIndex(0);

  m_backgroundLayer->addChild(
      ui::box({
          .out = &m_tintOverlay,
          .visible = false,
          .configure = [](Box& box) { box.setZIndex(1); },
      })
  );

  m_backgroundLayer->addChild(
      ui::box({
          .out = &m_backdrop,
          .configure = [](Box& box) { box.setZIndex(-1); },
      })
  );

  {
    auto widgetLayer = ui::node({});
    widgetLayer->setZIndex(2);
    m_widgetLayer = m_root.addChild(std::move(widgetLayer));
  }

  // Everything below is white on the dimmed wallpaper; a faint drop shadow keeps it legible on bright ones. Ink on
  // paper needs no shadow.
  m_ink = m_config != nullptr && m_config->config().lockscreen.ink;
  const Color textShadow = rgba(0.0F, 0.0F, 0.0F, m_ink ? 0.0F : 0.22F);

  m_root.addChild(
      ui::column(
          {
              .out = &m_clockColumn,
              .align = FlexAlign::Center,
              .gap = kDateClockGap,
              .configure = [](Flex& flex) { flex.setZIndex(2); },
          },
          ui::row(
              {.align = FlexAlign::Center, .gap = Style::spaceXs + 2.0F},
              ui::label({
                  .out = &m_dateLabel,
                  .fontSize = kDateFontSize,
                  .fontWeight = FontWeight::Medium,
                  .color = tone(m_ink, 0.86F),
                  .maxLines = 1,
                  .configure = [textShadow](Label& label) { label.setShadow(textShadow, 0.0F, 1.0F); },
              }),
              ui::glyph({
                  .out = &m_weatherGlyph,
                  .glyph = "weather-cloud",
                  .glyphSize = kWeatherGlyphSize,
                  .color = tone(m_ink, 0.86F),
                  .visible = false,
              }),
              ui::label({
                  .out = &m_weatherTemp,
                  .fontSize = kDateFontSize,
                  .fontWeight = FontWeight::Medium,
                  .color = tone(m_ink, 0.86F),
                  .maxLines = 1,
                  .visible = false,
                  .configure = [textShadow](Label& label) { label.setShadow(textShadow, 0.0F, 1.0F); },
              })
          ),
          ui::label({
              .out = &m_clockLabel,
              .fontSize = kClockFontSize,
              .fontWeight = FontWeight::Bold,
              .color = tone(m_ink, 0.96F),
              .maxLines = 1,
              .textAlign = TextAlign::Center,
              .configure = [textShadow](Label& label) { label.setShadow(textShadow, 0.0F, 2.0F); },
          })
      )
  );

  // Only a player with something loaded earns a capsule; there is no "Nothing Playing" state.
  m_root.addChild(
      ui::row(
          {
              .out = &m_nowPlaying,
              .align = FlexAlign::Center,
              .gap = Style::spaceSm,
              .paddingV = Style::spaceXs + 1.0F,
              .paddingH = Style::spaceSm + 2.0F,
              .fill = tone(m_ink, 0.16F),
              .radius = Style::pillRadius(kNowPlayingArtSize * 2.0F),
              .clipChildren = true,
              .visible = false,
              .configure = [](Flex& flex) { flex.setZIndex(2); },
          },
          ui::image({
              .out = &m_mediaArt,
              .fit = ImageFit::Cover,
              .radius = Style::pillRadius(kNowPlayingArtSize),
              .width = kNowPlayingArtSize,
              .height = kNowPlayingArtSize,
              .visible = false,
          }),
          ui::glyph({
              .out = &m_mediaFallbackGlyph,
              .glyph = "music",
              .glyphSize = 16.0F,
              .color = tone(m_ink, 0.9F),
          }),
          ui::column(
              {.align = FlexAlign::Start, .gap = 1.0F, .clipChildren = true},
              ui::label({
                  .out = &m_mediaTitle,
                  .fontSize = Style::fontSizeBody,
                  .fontWeight = FontWeight::SemiBold,
                  .color = tone(m_ink, 0.95F),
                  .maxLines = 1,
                  .ellipsize = TextEllipsize::End,
              }),
              ui::label({
                  .out = &m_mediaArtist,
                  .fontSize = Style::fontSizeCaption,
                  .color = tone(m_ink, 0.7F),
                  .maxLines = 1,
                  .ellipsize = TextEllipsize::End,
              })
          )
      )
  );

  m_root.addChild(
      ui::column(
          {
              .out = &m_loginPanel,
              .align = FlexAlign::Center,
              .gap = login::kGroupGap,
              .configure = [](Flex& flex) { flex.setZIndex(2); },
          },
          ui::column(
              {
                  .out = &m_avatarFrame,
                  .align = FlexAlign::Center,
                  .justify = FlexJustify::Center,
                  .fill = tone(m_ink, 0.22F),
                  .radius = Style::pillRadius(login::kAvatarSize),
                  .clipChildren = true,
                  .width = login::kAvatarSize,
                  .height = login::kAvatarSize,
              },
              ui::image({
                  .out = &m_avatarImage,
                  .fit = ImageFit::Cover,
                  .radius = Style::pillRadius(login::kAvatarSize),
                  .width = login::kAvatarSize,
                  .height = login::kAvatarSize,
                  .visible = false,
              }),
              ui::glyph({
                  .out = &m_avatarGlyph,
                  .glyph = "user-filled",
                  .glyphSize = kAvatarGlyphSize,
                  .color = tone(m_ink, 0.9F),
              })
          ),
          ui::label({
              .out = &m_userName,
              .fontSize = login::kNameFontSize,
              .fontWeight = FontWeight::SemiBold,
              .color = tone(m_ink, 0.96F),
              .maxLines = 1,
              .textAlign = TextAlign::Center,
              .ellipsize = TextEllipsize::End,
              .configure = [textShadow](Label& label) { label.setShadow(textShadow, 0.0F, 1.0F); },
          }),
          ui::row(
              {
                  .out = &m_pill,
                  .align = FlexAlign::Center,
                  .gap = Style::spaceXs + 2.0F,
                  .radius = Style::pillRadius(login::kPillHeight),
                  .minHeight = login::kPillHeight,
                  .maxHeight = login::kPillHeight,
                  .configure =
                      [](Flex& flex) {
                        // The field brings its own left inset; the right inset centers the submit circle in the
                        // pill's rounded end.
                        flex.setPadding(0.0F, (login::kPillHeight - login::kSubmitButtonSize) * 0.5F, 0.0F, 0.0F);
                        flex.setSoftness(1.0F);
                      },
              },
              ui::input({
                  .out = &m_passwordField,
                  .placeholder = i18n::tr("lockscreen.password-placeholder"),
                  .fontSize = Style::fontSizeBody,
                  .controlHeight = login::kPillHeight,
                  .horizontalPadding = kPillTextInset,
                  .passwordMode = true,
                  .frameVisible = false,
                  .flexGrow = 1.0F,
                  .onChange =
                      [this](const std::string& value) {
                        if (m_onPasswordChanged) {
                          m_onPasswordChanged(value);
                        }
                      },
                  .onSubmit =
                      [this](const std::string& /*value*/) {
                        if (m_onLogin) {
                          m_onLogin();
                        }
                      },
              }),
              ui::glyph({
                  .out = &m_capsLockGlyph,
                  .glyph = "arrow-big-up-line",
                  .glyphSize = kCapsLockGlyphSize,
                  .color = colorSpecFromRole(ColorRole::OnSurface, 0.7F),
                  .visible = false,
              }),
              ui::button({
                  .out = &m_loginButton,
                  .glyph = "arrow-right",
                  .glyphSize = kSubmitGlyphSize,
                  .customPalette = pillButtonPalette(),
                  .minWidth = login::kSubmitButtonSize,
                  .minHeight = login::kSubmitButtonSize,
                  .maxWidth = login::kSubmitButtonSize,
                  .maxHeight = login::kSubmitButtonSize,
                  .padding = 0.0F,
                  .radius = Style::pillRadius(login::kSubmitButtonSize),
                  .onClick =
                      [this]() {
                        if (m_onLogin) {
                          m_onLogin();
                        }
                      },
                  .configure = [](Button& button) { button.setJustify(FlexJustify::Center); },
              })
          )
      )
  );

  m_root.addChild(
      ui::row(
          {
              .out = &m_statusChip,
              .align = FlexAlign::Center,
              .justify = FlexJustify::Center,
              .radius = Style::scaledRadius(Style::spaceMd),
              .visible = false,
              .configure = [](Flex& flex) { flex.setZIndex(7); },
          },
          ui::label({
              .out = &m_statusLabel,
              .fontSize = Style::fontSizeCaption,
              .fontWeight = FontWeight::Medium,
              .maxLines = 1,
              .textAlign = TextAlign::Center,
              .ellipsize = TextEllipsize::End,
              .configure = [textShadow](Label& label) { label.setShadow(textShadow, 0.0F, 1.0F); },
          })
      )
  );

  m_root.addChild(
      ui::row(
          {
              .out = &m_cornerRow,
              .align = FlexAlign::Center,
              .gap = Style::spaceSm + 2.0F,
              .visible = false,
              .configure = [](Flex& flex) { flex.setZIndex(2); },
          },
          ui::button({
              .out = &m_layoutChip,
              .glyph = "keyboard",
              .fontSize = Style::fontSizeCaption,
              .glyphSize = kCornerGlyphSize,
              .customPalette = wallpaperGlassPalette(m_ink),
              .minHeight = kCornerButtonSize,
              .maxWidth = kLayoutChipMaxWidth,
              .maxHeight = kCornerButtonSize,
              .paddingV = 0.0F,
              .paddingH = Style::spaceSm + 2.0F,
              .radius = Style::pillRadius(kCornerButtonSize),
              .visible = false,
              .onClick =
                  [this]() {
                    if (m_onCycleLayout) {
                      m_onCycleLayout();
                    }
                  },
          }),
          ui::row({
              .out = &m_sessionRow,
              .align = FlexAlign::Center,
              .gap = Style::spaceSm + 2.0F,
              .visible = false,
          })
      )
  );

  {
    auto transitionCover = std::make_unique<LockscreenTransitionCover>();
    transitionCover->setVisible(false);
    transitionCover->setZIndex(1000);
    m_transitionCover = static_cast<LockscreenTransitionCover*>(m_root.addChild(std::move(transitionCover)));
  }

  m_inputDispatcher.setSceneRoot(&m_root);
  m_inputDispatcher.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    m_connection.setCursorShape(serial, shape);
  });

  setSceneRoot(&m_root);
  setAnimationManager(&m_animations);
  m_root.setAnimationManager(&m_animations);
  setConfigureCallback([this](std::uint32_t /*width*/, std::uint32_t /*height*/) { requestLayout(); });
  setPrepareFrameCallback([this](bool needsUpdate, bool needsLayout) { prepareFrame(needsUpdate, needsLayout); });
  requestUpdate();
}

LockSurface::~LockSurface() {
  m_aliveGuard.reset();
  releaseCaptureTextures();
  m_connection.unregisterSurface(m_surface);
  if (m_lockSurface != nullptr) {
    // If get_lock_surface raced with the output disappearing server-side, some
    // compositors (e.g. Hyprland) silently drop the request without binding the
    // new object id. The compositor otherwise sends the first configure event
    // immediately, so "output gone and no configure ever received" identifies
    // such a zombie proxy: sending its destructor request would be answered
    // with a fatal invalid-object protocol error. Destroy it client-side only.
    const bool outputGone = m_connection.findOutputByWl(m_output) == nullptr;
    if (!m_receivedConfigure && outputGone) {
      wl_proxy_destroy(reinterpret_cast<wl_proxy*>(m_lockSurface));
    } else {
      ext_session_lock_surface_v1_destroy(m_lockSurface);
    }
    m_lockSurface = nullptr;
  }
}

void LockSurface::setLoginBoxServices(
    SessionActionRunner* sessionActions, MprisService* mpris, const WeatherService* weather, HttpClient* httpClient
) {
  m_sessionActions = sessionActions;
  m_mpris = mpris;
  m_weather = weather;
  m_httpClient = httpClient;
  rebuildSessionButtons();
  requestUpdate();
}

bool LockSurface::initialize(ext_session_lock_v1* lock, wl_output* output, std::int32_t scale) {
  if (lock == nullptr || output == nullptr || renderContext() == nullptr) {
    return false;
  }

  if (const auto* outputInfo = m_connection.findOutputByWl(output); outputInfo != nullptr) {
    setConfiguredScaleNumerator(
        outputInfo->configuredScaleNumerator > 0 ? static_cast<std::uint32_t>(outputInfo->configuredScaleNumerator) : 1U
    );
    setBufferScale(outputInfo->scale);
  } else {
    setBufferScale(scale);
  }

  // Upload the immutable cover before the lock request is flushed. The first
  // configured lock-surface frame can then be the captured desktop immediately.
  if (m_transitionPhase == TransitionPhase::Cover && !ensureTransitionCaptureTexture()) {
    m_transitionPhase = TransitionPhase::Disabled;
    m_transitionProgress = 1.0F;
    syncTransitionCover();
  }

  if (!createWlSurface()) {
    return false;
  }

  // Keep the lock surface out of text-input-v3; it can leave Chromium's fcitx
  // context inactive after unlock. Password input still uses wl_keyboard.

  m_output = output;
  m_connection.registerSurfaceOutput(m_surface, output);
  m_lockSurface = ext_session_lock_v1_get_lock_surface(lock, m_surface, output);
  if (m_lockSurface == nullptr) {
    destroySurface();
    return false;
  }

  if (ext_session_lock_surface_v1_add_listener(m_lockSurface, &kLockSurfaceListener, this) != 0) {
    ext_session_lock_surface_v1_destroy(m_lockSurface);
    m_lockSurface = nullptr;
    destroySurface();
    return false;
  }

  setRunning(true);
  return true;
}

void LockSurface::syncOutputScale(std::int32_t bufferScale, std::uint32_t configuredScaleNumerator) {
  updateOutputScale(bufferScale, configuredScaleNumerator);
}

void LockSurface::setLockedState(bool locked) {
  if (m_locked == locked) {
    return;
  }
  m_locked = locked;
  if (m_locked) {
    // Re-read the avatar and display name on every lock so profile changes show up.
    m_avatarDirty = true;
    focusPasswordField();
  } else {
    m_inputDispatcher.setFocus(nullptr);
    m_clockTimer.stop();
  }
  requestUpdate();
}

bool LockSurface::passwordFieldContainsPoint(float sceneX, float sceneY) const {
  return m_passwordField != nullptr && m_passwordField->containsScenePoint(sceneX, sceneY);
}

void LockSurface::focusPasswordField() {
  if (!m_locked || m_blackout || m_passwordField == nullptr) {
    return;
  }
  m_inputDispatcher.setFocus(m_passwordField->inputArea());
}

void LockSurface::setPromptState(
    std::string user, std::string password, std::string status, bool error, bool authenticating
) {
  if (m_user == user
      && m_password == password
      && m_status == status
      && m_error == error
      && m_authenticating == authenticating) {
    return;
  }
  const bool newError = error && !m_error;
  m_user = std::move(user);
  m_password = std::move(password);
  m_status = std::move(status);
  m_error = error;
  m_authenticating = authenticating;
  if (newError && m_locked) {
    startErrorShake();
  }
  requestUpdate();
}

void LockSurface::setKeyboardIndicators(
    bool capsLock, bool hasMultipleLayouts, bool layoutSwitchable, std::string layoutLabel
) {
  if (m_capsLock == capsLock
      && m_hasMultipleLayouts == hasMultipleLayouts
      && m_layoutSwitchable == layoutSwitchable
      && m_layoutLabel == layoutLabel) {
    return;
  }
  m_capsLock = capsLock;
  m_hasMultipleLayouts = hasMultipleLayouts;
  m_layoutSwitchable = layoutSwitchable;
  m_layoutLabel = std::move(layoutLabel);
  requestUpdate();
}

void LockSurface::setWallpaperPath(std::string wallpaperPath) {
  if (m_wallpaperPath == wallpaperPath) {
    return;
  }

  if (m_blurredWallpaperTexture.id != 0 && renderContext() != nullptr) {
    renderContext()->backend().makeCurrentNoSurface();
    renderContext()->textureManager().unload(m_blurredWallpaperTexture);
    m_blurredWallpaperTexture = {};
  }

  // Keep the current wallpaper visible until applyWallpaperTexture() loads the new path.
  m_wallpaperPath = std::move(wallpaperPath);
  m_wallpaperDirty = true;
  requestLayout();
}

void LockSurface::setWallpaperFillMode(WallpaperFillMode fillMode) {
  if (m_wallpaperFillMode == fillMode) {
    return;
  }
  m_wallpaperFillMode = fillMode;
  if (m_wallpaper != nullptr) {
    m_wallpaper->setFillMode(m_wallpaperFillMode);
  }
  requestRedraw();
}

void LockSurface::setWallpaperCoverage(TextureCoverage coverage) {
  if (m_wallpaperCoverage == coverage) {
    return;
  }
  m_wallpaperCoverage = coverage;
  m_wallpaperDirty = true;
  requestLayout();
}

void LockSurface::setWallpaperFillColor(Color fillColor) {
  if (m_wallpaperFillColor == fillColor) {
    return;
  }
  m_wallpaperFillColor = fillColor;
  if (m_wallpaper != nullptr) {
    m_wallpaper->setFillColor(m_wallpaperFillColor);
  }
  if (m_backdrop != nullptr) {
    m_backdrop->setVisible(true);
    m_backdrop->setStyle(
        RoundedRectStyle{
            .fill = opaqueBackdropColor(m_wallpaperFillColor),
            .fillMode = FillMode::Solid,
        }
    );
  }
  requestRedraw();
}

void LockSurface::setDesktopCapture(std::optional<ScreencopyImage> capture, bool useAsBackground) {
  m_desktopCapture = std::move(capture);
  m_useDesktopCaptureBackground = useAsBackground && m_desktopCapture.has_value() && !m_desktopCapture->rgba.empty();
  m_captureDirty = true;
  releaseCaptureTextures();
  requestLayout();
}

bool LockSurface::usesDesktopCaptureBackground() const noexcept {
  return m_useDesktopCaptureBackground && m_desktopCapture.has_value() && !m_desktopCapture->rgba.empty();
}

void LockSurface::configureTransition(
    std::optional<LockscreenTransitionKind> transition, const LockscreenTransitionParams& params, float durationMs
) {
  cancelTransitionAnimation();
  m_transitionParams = params;
  m_transitionDurationMs = std::max(0.0F, durationMs);
  m_transitionProgress = 1.0F;
  m_enterTransitionRequested = false;

  if (!transition.has_value() || m_blackout || !m_desktopCapture.has_value() || m_desktopCapture->rgba.empty()) {
    m_transitionPhase = TransitionPhase::Disabled;
    syncTransitionCover();
    return;
  }

  m_transition = *transition;
  m_transitionProgress = 0.0F;
  m_transitionPhase = TransitionPhase::Cover;
  syncTransitionCover();
  requestLayout();
}

void LockSurface::startEnterTransition() {
  m_enterTransitionRequested = true;
  if (m_transitionPhase == TransitionPhase::Ready) {
    beginEnterAnimation();
  }
}

void LockSurface::startExitTransition() {
  m_enterTransitionRequested = false;
  if (m_transitionPhase == TransitionPhase::ExitComplete
      || m_transitionPhase == TransitionPhase::ExitEndpoint
      || m_transitionPhase == TransitionPhase::Exiting) {
    return;
  }

  if (m_transitionPhase != TransitionPhase::Disabled && m_transitionCaptureTexture.id == 0) {
    (void)ensureTransitionCaptureTexture();
  }
  if (m_transitionPhase == TransitionPhase::Disabled || m_transitionCaptureTexture.id == 0) {
    m_transitionPhase = TransitionPhase::ExitComplete;
    notifyTransitionStateChanged();
    return;
  }

  cancelTransitionAnimation();
  syncTransitionCover();
  if (m_transitionProgress <= 0.0F) {
    m_transitionProgress = 0.0F;
    m_transitionPhase = TransitionPhase::ExitEndpoint;
    syncTransitionCover();
    requestRedraw();
    return;
  }

  m_transitionPhase = TransitionPhase::Exiting;
  syncTransitionCover();
  // Unlocking is a little quicker than locking (exits are shorter than entrances).
  const float durationMs = m_transitionDurationMs * kUnlockDurationScale * m_transitionProgress;
  m_transitionAnimation = startTransitionAnimation(
      m_transitionProgress, 0.0F, durationMs,
      [this](float progress) {
        m_transitionProgress = progress;
        syncTransitionCover();
      },
      [this]() {
        m_transitionAnimation = 0;
        m_transitionProgress = 0.0F;
        m_transitionPhase = TransitionPhase::ExitEndpoint;
        syncTransitionCover();
        requestRedraw();
      },
      this
  );
  requestRedraw();
}

bool LockSurface::transitionInputReady() const noexcept {
  return m_transitionPhase == TransitionPhase::Disabled || m_transitionPhase == TransitionPhase::Stable;
}

bool LockSurface::exitTransitionComplete() const noexcept {
  return m_transitionPhase == TransitionPhase::Disabled || m_transitionPhase == TransitionPhase::ExitComplete;
}

void LockSurface::setBackgroundStyle(float blurIntensity, float tintIntensity) {
  if (m_blurIntensity == blurIntensity && m_tintIntensity == tintIntensity) {
    return;
  }
  m_blurIntensity = blurIntensity;
  m_tintIntensity = tintIntensity;
  m_captureDirty = true;
  m_blurCache.invalidate();
  m_wallpaperDirty = true;
  m_wallpaperBlurCache.invalidate();
  requestLayout();
}

void LockSurface::setBlackout(bool blackout) {
  if (m_blackout == blackout) {
    return;
  }
  m_blackout = blackout;
  if (m_blackout) {
    m_inputDispatcher.setFocus(nullptr);
    m_clockTimer.stop();
    cancelTransitionAnimation();
    m_transitionPhase = TransitionPhase::Disabled;
    m_transitionProgress = 1.0F;
    syncTransitionCover();
    notifyTransitionStateChanged();
  }
  // An update, not just a layout: the clock restarts its tick from the update pass.
  requestUpdate();
}

void LockSurface::setOnLogin(std::function<void()> onLogin) { m_onLogin = std::move(onLogin); }

void LockSurface::setOnCycleLayout(std::function<void()> onCycleLayout) { m_onCycleLayout = std::move(onCycleLayout); }

void LockSurface::setOnPasswordChanged(std::function<void(const std::string&)> onPasswordChanged) {
  m_onPasswordChanged = std::move(onPasswordChanged);
}

void LockSurface::selectAllPassword() {
  if (m_passwordField == nullptr) {
    return;
  }
  m_passwordField->selectAll();
  requestLayout();
}

void LockSurface::clearPasswordSelection() {
  if (m_passwordField == nullptr) {
    return;
  }
  m_passwordField->clearSelection();
  requestLayout();
}

void LockSurface::onPointerEvent(const PointerEvent& event) {
  if (m_blackout) {
    return;
  }

  switch (event.type) {
  case PointerEvent::Type::Enter:
    m_inputDispatcher.pointerEnter(static_cast<float>(event.sx), static_cast<float>(event.sy), event.serial);
    break;
  case PointerEvent::Type::Leave:
    m_inputDispatcher.pointerLeave();
    break;
  case PointerEvent::Type::Motion:
    m_inputDispatcher.pointerMotion(static_cast<float>(event.sx), static_cast<float>(event.sy), event.serial);
    break;
  case PointerEvent::Type::Button: {
    const bool pressed = event.pressed;
    const auto x = static_cast<float>(event.sx);
    const auto y = static_cast<float>(event.sy);
    if (m_locked && pressed && passwordFieldContainsPoint(x, y)) {
      focusPasswordField();
    }
    m_inputDispatcher.pointerButton(x, y, event.button, pressed, event.serial, event.time, event.touch);
    if (m_locked && pressed && passwordFieldContainsPoint(x, y)) {
      focusPasswordField();
      requestRedraw();
    }
    break;
  }
  case PointerEvent::Type::Axis:
    m_inputDispatcher.pointerAxis(
        static_cast<float>(event.sx), static_cast<float>(event.sy), event.axis, event.axisSource, event.axisValue,
        event.axisDiscrete, event.axisValue120, event.axisLines
    );
    break;
  }

  syncPillChrome(false);
  if (m_root.paintDirty() || m_root.layoutDirty()) {
    if (m_root.layoutDirty()) {
      requestLayout();
    } else {
      requestRedraw();
    }
  }
}

void LockSurface::onThemeChanged() {
  m_captureDirty = true;
  requestLayout();
}

void LockSurface::onKeyboardEvent(const KeyboardEvent& event) {
  if (m_blackout) {
    return;
  }

  if (m_locked
      && event.pressed
      && m_passwordField != nullptr
      && m_inputDispatcher.focusedArea() != m_passwordField->inputArea()) {
    focusPasswordField();
  }
  m_inputDispatcher.keyEvent(event.sym, event.utf32, event.modifiers, event.pressed, event.preedit);
  syncPillChrome(false);
  if (m_root.paintDirty() || m_root.layoutDirty()) {
    if (m_root.layoutDirty()) {
      requestLayout();
    } else {
      requestRedraw();
    }
  }
}

void LockSurface::handleConfigure(
    void* data, ext_session_lock_surface_v1* lockSurface, std::uint32_t serial, std::uint32_t width,
    std::uint32_t height
) {
  auto* self = static_cast<LockSurface*>(data);
  self->m_receivedConfigure = true;
  if (self->width() != width || self->height() != height) {
    self->m_firstFrameRendered = false;
  }
  ext_session_lock_surface_v1_ack_configure(lockSurface, serial);
  self->Surface::onConfigure(width, height);
}

bool LockSurface::ensureTransitionCaptureTexture() {
  if (m_transitionCaptureTexture.id != 0) {
    return true;
  }
  if (renderContext() == nullptr
      || !m_desktopCapture.has_value()
      || m_desktopCapture->rgba.empty()
      || m_desktopCapture->width <= 0
      || m_desktopCapture->height <= 0) {
    return false;
  }

  auto& backend = renderContext()->backend();
  if (!backend.makeCurrentNoSurface()) {
    return false;
  }
  const auto& capture = *m_desktopCapture;
  m_transitionCaptureTexture =
      renderContext()->textureManager().loadFromRgba(capture.rgba.data(), capture.width, capture.height, false);
  syncTransitionCover();
  return m_transitionCaptureTexture.id != 0;
}

void LockSurface::layoutCoverOnly(std::uint32_t width, std::uint32_t height) {
  const auto sw = static_cast<float>(width);
  const auto sh = static_cast<float>(height);
  m_root.setSize(sw, sh);
  m_backgroundLayer->setVisible(false);
  m_widgetLayer->setVisible(false);
  hideLoginChrome();
  layoutTransitionCover();
  syncTransitionCover();
}

void LockSurface::layoutTransitionCover() {
  if (m_transitionCover == nullptr) {
    return;
  }
  const auto sw = static_cast<float>(width());
  const auto sh = static_cast<float>(height());
  m_transitionCover->setPosition(0.0F, 0.0F);
  m_transitionCover->setSize(sw, sh);
  m_transitionParams.aspectRatio = sh > 0.0F ? sw / sh : 1.0F;
}

void LockSurface::syncTransitionCover() {
  if (m_transitionCover == nullptr) {
    return;
  }

  const bool visible = m_transitionCaptureTexture.id != 0
      && m_transitionPhase != TransitionPhase::Disabled
      && m_transitionPhase != TransitionPhase::Stable;
  m_transitionCover->setVisible(visible);
  m_transitionCover->setTexture(m_transitionCaptureTexture.id);
  if (!visible) {
    return;
  }
  m_transitionCover->setTransition(m_transition, m_transitionProgress, m_transitionParams);
}

void LockSurface::cancelTransitionAnimation() {
  if (const auto animation = std::exchange(m_transitionAnimation, 0); animation != 0) {
    m_animations.cancel(animation);
  }
}

void LockSurface::beginEnterAnimation() {
  if (m_transitionPhase != TransitionPhase::Ready) {
    return;
  }

  cancelTransitionAnimation();
  m_transitionPhase = TransitionPhase::Entering;
  m_transitionProgress = 0.0F;
  // Reduce Motion: wipes, discs and zooms become a crossfade.
  if (MotionService::instance().reduced()) {
    m_transition = LockscreenTransitionKind::Fade;
  }
  syncTransitionCover();
  m_transitionAnimation = startTransitionAnimation(
      0.0F, 1.0F, m_transitionDurationMs,
      [this](float progress) {
        m_transitionProgress = progress;
        syncTransitionCover();
      },
      [this]() {
        m_transitionAnimation = 0;
        m_transitionProgress = 1.0F;
        m_transitionPhase = TransitionPhase::Stable;
        syncTransitionCover();
        requestRedraw();
        notifyTransitionStateChanged();
      },
      this
  );
  requestRedraw();
}

AnimationManager::Id LockSurface::startTransitionAnimation(
    float from, float to, float durationMs, std::function<void(float)> setter, std::function<void()> onComplete,
    const void* owner
) {
  // On the motion clock, so [shell.animation] speed and Reduce Motion apply. The cover is drawn
  // by this opaque lock surface from a frozen capture: the live session is never shown.
  if (m_transition == LockscreenTransitionKind::Fade) {
    return m_animations.animateFade(
        from, to, durationMs, Easing::EaseInOutCubic, std::move(setter), std::move(onComplete), owner
    );
  }
  return m_animations.animate(
      from, to, durationMs, Easing::EaseInOutCubic, std::move(setter), std::move(onComplete), owner
  );
}

void LockSurface::notifyTransitionStateChanged() {
  if (m_transitionCallback) {
    m_transitionCallback();
  }
}

void LockSurface::prepareFrame(bool needsUpdate, bool needsLayout) {
  auto* context = renderContext();
  if (context == nullptr || width() == 0 || height() == 0) {
    return;
  }

  context->makeCurrent(renderTarget());
  Renderer& renderer = renderTarget().renderer();

  if (m_transitionPhase == TransitionPhase::Cover) {
    if (ensureTransitionCaptureTexture()) {
      UiPhaseScope layoutPhase(UiPhase::Layout);
      layoutCoverOnly(width(), height());
      return;
    }
    m_transitionPhase = TransitionPhase::Disabled;
    m_transitionProgress = 1.0F;
    syncTransitionCover();
  }

  if (m_widgetsHost != nullptr) {
    m_widgetsHost->prepareFrame(*this, needsUpdate, needsLayout);
  }

  if (needsUpdate) {
    UiPhaseScope updatePhase(UiPhase::Update);
    updateCopy();
    syncInfoExtras(renderer);
    syncAvatar(renderer);
  }

  if (needsUpdate || needsLayout) {
    UiPhaseScope layoutPhase(UiPhase::Layout);
    layoutScene(width(), height());
  }
}

void LockSurface::layoutScene(std::uint32_t width, std::uint32_t height) {
  if (renderContext() == nullptr) {
    return;
  }
  Renderer& renderer = renderTarget().renderer();

  const auto sw = static_cast<float>(width);
  const auto sh = static_cast<float>(height);

  if (m_blackout) {
    m_root.setSize(sw, sh);
    m_backgroundLayer->setVisible(true);
    m_backgroundLayer->setPosition(0.0F, 0.0F);
    m_backgroundLayer->setSize(sw, sh);
    m_wallpaper->setVisible(false);
    m_tintOverlay->setVisible(false);
    m_backdrop->setPosition(0.0F, 0.0F);
    m_backdrop->setSize(sw, sh);
    m_backdrop->setVisible(true);
    m_backdrop->setStyle(
        RoundedRectStyle{
            .fill = rgba(0.0F, 0.0F, 0.0F, 1.0F),
            .fillMode = FillMode::Solid,
        }
    );
    m_widgetLayer->setVisible(false);
    hideLoginChrome();
    layoutTransitionCover();
    syncTransitionCover();
    return;
  }

  applyWallpaperTexture();

  m_root.setSize(sw, sh);
  m_backgroundLayer->setVisible(true);
  m_backgroundLayer->setPosition(0.0F, 0.0F);
  m_backgroundLayer->setSize(sw, sh);
  m_widgetLayer->setVisible(true);

  m_wallpaper->setVisible(true);
  m_wallpaper->setPosition(0.0F, 0.0F);
  m_wallpaper->setSize(sw, sh);
  m_wallpaper->setFillMode(m_wallpaperFillMode);
  m_wallpaper->setFillColor(m_wallpaperFillColor);

  m_backdrop->setPosition(0.0F, 0.0F);
  m_backdrop->setSize(sw, sh);
  m_backdrop->setVisible(true);
  m_backdrop->setStyle(
      RoundedRectStyle{
          .fill = opaqueBackdropColor(m_wallpaperFillColor),
          .fillMode = FillMode::Solid,
      }
  );

  if (m_tintOverlay != nullptr) {
    m_tintOverlay->setPosition(0.0F, 0.0F);
    m_tintOverlay->setSize(sw, sh);
    const bool showTint = m_tintIntensity > 0.0F;
    m_tintOverlay->setVisible(showTint);
    if (showTint) {
      // Always a dim, never a theme-colored wash: the clock and user block are white in every theme.
      m_tintOverlay->setStyle(
          RoundedRectStyle{
              .fill = rgba(0.0F, 0.0F, 0.0F, m_tintIntensity),
              .fillMode = FillMode::Solid,
          }
      );
    }
  }

  if (!isLoginBoxEnabled()) {
    hideLoginChrome();
    layoutTransitionCover();
    syncTransitionCover();
    return;
  }

  const lockscreen_login_box::LoginBoxStyle style = resolveLoginStyle();
  const bool regular = style.layout == lockscreen_login_box::LayoutMode::Regular;
  const float edge = Style::spaceLg;

  // Clock: the date line over the large time, centered at the top (unless a clock widget already shows one), or
  // in the corner style resting in the bottom-left corner.
  const bool builtInClock = !outputHasClockWidget(m_config, m_outputKey);
  const bool corner = cornerClock();
  const float clockScale = std::clamp(sh / kClockReferenceHeight, kClockMinScale, 1.0F);
  m_clockLabel->setFontSize((corner ? kCornerClockFontSize : kClockFontSize) * clockScale);
  m_clockLabel->setTextAlign(corner ? TextAlign::Start : TextAlign::Center);
  m_dateLabel->setFontSize(corner ? kCornerDateFontSize : kDateFontSize);
  m_dateLabel->setLetterSpacing(corner ? kCornerDateTracking : 0.0F);
  m_clockColumn->setAlign(corner ? FlexAlign::Start : FlexAlign::Center);
  m_clockColumn->setGap(corner ? 0.0F : kDateClockGap);
  m_clockColumn->setVisible(builtInClock);
  float clockY = std::round(std::max(edge * 2.0F, sh * kClockTopInset));
  LayoutSize clockSize{};
  if (builtInClock) {
    clockSize = m_clockColumn->measure(renderer, LayoutConstraints::available(sw, sh));
    const float inset = std::round(kCornerClockInset * clockScale);
    const float clockX = corner ? inset : std::round((sw - clockSize.width) * 0.5F);
    if (corner) {
      clockY = std::round(sh - inset - clockSize.height);
    }
    m_clockColumn->arrange(renderer, LayoutRect{clockX, clockY, clockSize.width, clockSize.height});
  }

  // Visibility comes from the update pass; layout only places it under the clock (at the top beside a corner one).
  if (m_nowPlaying->visible()) {
    m_nowPlaying->setMaxWidth(std::min(kNowPlayingMaxWidth, sw - edge * 2.0F));
    const LayoutSize size = m_nowPlaying->measure(renderer, LayoutConstraints::available(sw - edge * 2.0F, sh));
    const float nowPlayingY = corner ? std::round(std::max(edge * 2.0F, sh * kClockTopInset))
                                     : clockY + clockSize.height + Style::spaceMd;
    m_nowPlaying->arrange(
        renderer, LayoutRect{std::round((sw - size.width) * 0.5F), nowPlayingY, size.width, size.height}
    );
  }

  // User block: avatar, name and pill, resting low and centered unless the editor moved it.
  const DesktopWidgetState* loginBox = m_config != nullptr
      ? lockscreen_login_box::findForOutput(m_config->config().lockscreenWidgets.widgets, m_outputKey)
      : nullptr;
  const float pillWidth = lockscreen_login_box::resolvePanelWidth(sw, loginBox != nullptr ? loginBox->boxWidth : 0.0F);
  m_avatarFrame->setVisible(regular);
  m_userName->setVisible(regular);
  m_userName->setMaxWidth(pillWidth);
  m_pill->setMinWidth(pillWidth);
  m_pill->setMaxWidth(pillWidth);
  m_passwordField->setTextAlign(style.centerPasswordText ? TextAlign::Center : TextAlign::Start);
  m_loginButton->setVisible(style.showLoginButton);
  syncPillChrome(true);
  m_loginPanel->setVisible(true);

  const LayoutSize groupSize = m_loginPanel->measure(renderer, LayoutConstraints::available(sw, sh));
  float cx = 0.0F;
  float cy = 0.0F;
  lockscreen_login_box::defaultPanelCenter(sw, sh, cx, cy, style.layout);
  if (loginBox != nullptr && !lockscreen_login_box::isLegacyCardBox(*loginBox)) {
    cx = loginBox->cx;
    cy = loginBox->cy;
  }
  const float groupX = util::clampOrdered(std::round(cx - groupSize.width * 0.5F), edge, sw - groupSize.width - edge);
  const float groupY = util::clampOrdered(std::round(cy - groupSize.height * 0.5F), edge, sh - groupSize.height - edge);
  m_loginPanel->arrange(renderer, LayoutRect{groupX, groupY, groupSize.width, groupSize.height});
  // On a short output now-playing gives way to the user block instead of running into it.
  if (m_nowPlaying->visible() && m_nowPlaying->y() + m_nowPlaying->height() + Style::spaceMd > groupY) {
    m_nowPlaying->setVisible(false);
  }
  m_pillRestX = m_pill->x();
  if (m_shakeOffset != 0.0F) {
    m_pill->setPosition(m_pillRestX + m_shakeOffset, m_pill->y());
  }

  // Status hint under the pill; it flips above the block when the block sits too close to the bottom edge.
  if (m_statusChip->visible()) {
    const float maxWidth = std::min(kStatusMaxWidth, sw - edge * 2.0F);
    m_statusLabel->setMaxWidth(maxWidth);
    const LayoutSize size = m_statusChip->measure(renderer, LayoutConstraints::available(maxWidth, sh));
    float hintY = groupY + groupSize.height + lockscreen_login_box::kHintGap;
    if (hintY + size.height > sh - edge) {
      hintY = groupY - lockscreen_login_box::kHintGap - size.height;
    }
    const float hintX =
        util::clampOrdered(std::round(groupX + (groupSize.width - size.width) * 0.5F), edge, sw - size.width - edge);
    m_statusChip->arrange(renderer, LayoutRect{hintX, hintY, size.width, size.height});
  }

  // Session actions and the keyboard layout chip: quiet glass in the bottom-right corner.
  // Compact keeps only the pill, as before: no session buttons (the editor hides their switch there too).
  const bool showSessionButtons = regular && style.showSessionButtons;
  if (showSessionButtons) {
    rebuildSessionButtons();
  }
  m_sessionRow->setVisible(showSessionButtons && !m_sessionButtons.empty());
  m_cornerRow->setVisible(m_sessionRow->visible() || m_layoutChip->visible());
  if (m_cornerRow->visible()) {
    const LayoutSize size = m_cornerRow->measure(renderer, LayoutConstraints::available(sw, sh));
    m_cornerRow->arrange(
        renderer, LayoutRect{sw - kCornerInset - size.width, sh - kCornerInset - size.height, size.width, size.height}
    );
  }

  layoutTransitionCover();
  syncTransitionCover();
}

void LockSurface::hideLoginChrome() {
  m_clockColumn->setVisible(false);
  m_nowPlaying->setVisible(false);
  m_loginPanel->setVisible(false);
  m_statusChip->setVisible(false);
  m_cornerRow->setVisible(false);
}

void LockSurface::updateCopy() {
  m_passwordField->setValue(m_password);
  m_passwordField->setEnabled(!m_authenticating);
  m_loginButton->setEnabled(!m_authenticating);
  // The arrow stays quiet until there is something to submit.
  m_loginButton->setOpacity(m_password.empty() ? 0.5F : 1.0F);
  for (Button* button : m_sessionButtons) {
    if (button != nullptr) {
      button->setEnabled(!m_authenticating);
    }
  }

  refreshClock();

  const lockscreen_login_box::LoginBoxStyle style = resolveLoginStyle();
  const bool active = m_locked && !m_blackout && isLoginBoxEnabled();
  m_capsLockGlyph->setVisible(active && style.showCapsLock && m_capsLock);

  bool isError = false;
  const std::string status = active ? resolveStatusText(style, isError) : std::string{};
  m_statusChip->setVisible(!status.empty());
  if (!status.empty()) {
    m_statusLabel->setText(status);
    // An error gets a red-tinted glass chip; anything else is plain quiet text.
    if (isError) {
      m_statusChip->setFill(colorForRole(ColorRole::Error, 0.45F));
      m_statusChip->setPadding(Style::spaceXs + 1.0F, Style::spaceMd);
      m_statusLabel->setColor(white(1.0F));
    } else {
      m_statusChip->clearFill();
      m_statusChip->setPadding(0.0F);
      m_statusLabel->setColor(tone(m_ink, 0.8F));
    }
  }

  const bool showLayout = active && style.showKeyboardLayout && m_hasMultipleLayouts;
  m_layoutChip->setVisible(showLayout);
  if (showLayout) {
    m_layoutChip->setText(m_layoutLabel.empty() ? "—" : m_layoutLabel);
    m_layoutChip->setEnabled(m_layoutSwitchable);
  }
}

void LockSurface::refreshClock() {
  const std::string format = lockClockFormat(m_config);
  m_clockLabel->setText(formatLocalTime(format.c_str()));
  // Same translated long date as the status overlay ("Sunday, 11 October").
  const std::string date = formatLocalTime(i18n::tr("lockscreen.date-format").c_str());
  m_dateLabel->setText(cornerClock() ? i18n::upper(date) : date);

  // Nothing to tick for while the clock is not on screen (unlocked, blacked out, no login box, a clock widget).
  if (!m_locked || m_blackout || !isLoginBoxEnabled() || outputHasClockWidget(m_config, m_outputKey)) {
    m_clockTimer.stop();
    return;
  }
  // Wake on the next minute (or second) boundary rather than polling.
  using namespace std::chrono;
  const milliseconds period = formatShowsSeconds(format) ? milliseconds{1000} : milliseconds{60000};
  const auto sinceEpoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  m_clockTimer.start(period - sinceEpoch % period + milliseconds{20}, [this]() { requestUpdate(); });
}

std::vector<SessionPanelActionConfig> LockSurface::resolveSessionActions() const {
  const std::vector<SessionPanelActionConfig> src =
      m_config != nullptr ? m_config->config().shell.session.actions : defaultSessionPanelActions();
  return lockscreen::resolveSessionActions(src, defaultSessionPanelActions());
}

void LockSurface::rebuildSessionButtons() {
  if (m_sessionRow == nullptr) {
    return;
  }

  const auto actions = resolveSessionActions();
  std::vector<std::string> keys;
  keys.reserve(actions.size());
  for (const auto& action : actions) {
    keys.push_back(action.action + "|" + (action.label.value_or("")) + "|" + (action.glyph.value_or("")));
  }
  if (keys == m_lastSessionActionKeys && !m_sessionButtons.empty()) {
    return;
  }
  m_lastSessionActionKeys = std::move(keys);

  while (!m_sessionRow->children().empty()) {
    m_sessionRow->removeChild(m_sessionRow->children().front().get());
  }
  m_sessionButtons.clear();

  if (m_sessionActions == nullptr) {
    return;
  }

  for (const auto& cfg : actions) {
    auto button = ui::button({
        .glyph = cfg.glyph.has_value() && !cfg.glyph->empty() ? *cfg.glyph : session_action::defaultGlyph(cfg.action),
        .glyphSize = kCornerGlyphSize,
        .customPalette = wallpaperGlassPalette(m_ink),
        .minWidth = kCornerButtonSize,
        .minHeight = kCornerButtonSize,
        .maxWidth = kCornerButtonSize,
        .maxHeight = kCornerButtonSize,
        .padding = 0.0F,
        .radius = Style::pillRadius(kCornerButtonSize),
        .onClick =
            [this, cfg]() {
              if (m_sessionActions != nullptr) {
                m_sessionActions->invoke(cfg);
              }
            },
        .configure = [](Button& control) { control.setJustify(FlexJustify::Center); },
    });
    m_sessionButtons.push_back(button.get());
    m_sessionRow->addChild(std::move(button));
  }
}

void LockSurface::syncInfoExtras(Renderer& renderer) {
  const lockscreen_login_box::LoginBoxStyle style = resolveLoginStyle();
  // Compact shows no media or weather, as before (its editor hides both switches): a privacy choice for some.
  const bool active =
      m_locked && !m_blackout && isLoginBoxEnabled() && style.layout == lockscreen_login_box::LayoutMode::Regular;

  // Weather joins the date line as a glyph and the current temperature.
  const bool showWeather =
      active && style.showWeather && m_weather != nullptr && m_weather->enabled() && m_weather->hasData();
  m_weatherGlyph->setVisible(showWeather);
  m_weatherTemp->setVisible(showWeather);
  if (showWeather) {
    const auto& current = m_weather->snapshot().current;
    m_weatherGlyph->setGlyph(WeatherService::glyphForCode(current.weatherCode, current.isDay));
    const long temperature = std::lround(m_weather->displayTemperature(current.temperatureC));
    m_weatherTemp->setText(std::format("{}{}", temperature, m_weather->displayTemperatureUnit()));
  }

  const auto player = active && style.showMedia && m_mpris != nullptr ? m_mpris->activePlayer() : std::nullopt;
  const bool showMedia = player.has_value() && !player->title.empty();
  m_nowPlaying->setVisible(showMedia);
  if (!showMedia) {
    return;
  }
  m_mediaTitle->setText(player->title);
  const std::string artist = mpris::joinArtists(player->artists);
  m_mediaArtist->setText(artist);
  m_mediaArtist->setVisible(!artist.empty());

  const std::string artUrl = mpris::effectiveArtUrl(*player);
  if (artUrl == m_lastArtUrl && (artUrl.empty() || m_mediaArt->hasImage())) {
    return;
  }
  m_lastArtUrl = artUrl;
  bool hasArt = false;
  if (!artUrl.empty()) {
    const std::string artPath = mpris::resolveArtworkSource(
        m_httpClient, m_pendingArtDownloads, artUrl, [this] { requestUpdate(); }, m_aliveGuard
    );
    if (!artPath.empty()) {
      hasArt = m_mediaArt->setSourceFile(
          renderer, artPath, static_cast<int>(std::round(kNowPlayingArtSize * 2.0F)), true, true
      );
    }
  }
  if (!hasArt) {
    m_mediaArt->clear(renderer);
  }
  m_mediaArt->setVisible(hasArt);
  m_mediaFallbackGlyph->setVisible(!hasArt);
}

void LockSurface::syncAvatar(Renderer& renderer) {
  if (!m_avatarDirty) {
    return;
  }
  m_avatarDirty = false;

  const std::string displayName = sessionDisplayName();
  m_userName->setText(displayName.empty() ? m_user : displayName);

  m_avatarPath = resolveAvatarPath(m_config, m_user);
  const bool hasImage = !m_avatarPath.empty()
      && m_avatarImage->setSourceFile(
          renderer, m_avatarPath, static_cast<int>(std::round(login::kAvatarSize * 2.0F)), false, true
      );
  if (!hasImage) {
    m_avatarImage->clear(renderer);
  }
  m_avatarImage->setVisible(hasImage);
  m_avatarGlyph->setVisible(!hasImage);
}

void LockSurface::syncPillChrome(bool force) {
  // Pointer and key events land here on every motion; only a focus change is worth a repaint.
  const bool focused = m_locked && m_inputDispatcher.focusedArea() == m_passwordField->inputArea();
  if (!force && focused == m_pillFocused) {
    return;
  }
  m_pillFocused = focused;
  m_pill->setFill(lockscreen_login_box::pillFillColor());
  // The focused field's ring is the only accent on the lock screen.
  if (focused) {
    m_pill->setBorder(resolveColorSpec(focusRingColorSpec()), kPillFocusRingWidth);
  } else {
    m_pill->setBorder(lockscreen_login_box::pillEdgeColor(), Style::borderWidth);
  }
}

void LockSurface::startErrorShake() {
  if (const auto animation = std::exchange(m_shakeAnimation, 0); animation != 0) {
    m_animations.cancel(animation);
  }
  m_shakeAnimation = m_animations.animate(
      0.0F, 1.0F, kShakeDurationMs, Easing::Linear,
      [this](float progress) {
        m_shakeOffset =
            kShakeAmplitude * std::sin(progress * kShakeSwings * 2.0F * std::numbers::pi_v<float>) * (1.0F - progress);
        m_pill->setPosition(m_pillRestX + m_shakeOffset, m_pill->y());
      },
      [this]() {
        m_shakeAnimation = 0;
        m_shakeOffset = 0.0F;
        m_pill->setPosition(m_pillRestX, m_pill->y());
      },
      this
  );
  requestRedraw();
}

lockscreen_login_box::LoginBoxStyle LockSurface::resolveLoginStyle() const {
  if (m_config == nullptr) {
    return lockscreen_login_box::LoginBoxStyle{};
  }
  if (const DesktopWidgetState* loginBox =
          lockscreen_login_box::findForOutput(m_config->config().lockscreenWidgets.widgets, m_outputKey);
      loginBox != nullptr) {
    return lockscreen_login_box::resolveStyle(loginBox->settings);
  }
  return lockscreen_login_box::LoginBoxStyle{};
}

bool LockSurface::cornerClock() const {
  return m_config != nullptr && m_config->config().lockscreen.clockStyle == LockscreenClockStyle::Corner;
}

bool LockSurface::isLoginBoxEnabled() const {
  if (m_config == nullptr) {
    return true;
  }
  if (const DesktopWidgetState* loginBox =
          lockscreen_login_box::findForOutput(m_config->config().lockscreenWidgets.widgets, m_outputKey);
      loginBox != nullptr) {
    return loginBox->enabled;
  }
  return true;
}

std::string LockSurface::resolveStatusText(const lockscreen_login_box::LoginBoxStyle& style, bool& isError) const {
  isError = false;
  // Errors always show. Authenticating, PAM and fingerprint prompts are gated by showUnlockHint, Caps Lock by its
  // own toggle. There is no idle hint: the pill's placeholder already asks for the password.
  if (m_error) {
    isError = true;
    return m_status;
  }
  if (style.showUnlockHint && (m_authenticating || !m_status.empty())) {
    return m_status;
  }
  if (m_capsLock && style.showCapsLock) {
    return i18n::tr("lockscreen.caps-lock-on");
  }
  return {};
}

void LockSurface::applyWallpaperTexture() {
  if (usesDesktopCaptureBackground()) {
    applyBlurredDesktopTexture();
    if (m_blurredDesktopTexture.id != 0) {
      return;
    }
  }

  if (!m_wallpaperDirty) {
    return;
  }

  bool loaded = true;
  Color color = rgba(0.0F, 0.0F, 0.0F, 1.0F);
  if (parseColorWallpaperPath(m_wallpaperPath, color)) {
    m_wallpaperLease.reset();
    m_pendingWallpaperLease.reset();
    m_shownWallpaperPath.clear();
    m_pendingWallpaperPath.clear();
    if (m_blurredWallpaperTexture.id != 0 && renderContext() != nullptr) {
      renderContext()->backend().makeCurrentNoSurface();
      renderContext()->textureManager().unload(m_blurredWallpaperTexture);
      m_blurredWallpaperTexture = {};
    }
    m_wallpaper->setSources(
        WallpaperSourceKind::Color, {}, color, WallpaperSourceKind::Image, {}, rgba(0.0F, 0.0F, 0.0F, 1.0F), 0.0F, 0.0F,
        0.0F, 0.0F
    );
    m_wallpaper->setTransition(WallpaperTransition::Fade, 0.0F, TransitionParams{});
    m_wallpaper->setFillMode(m_wallpaperFillMode);
    m_wallpaper->setFillColor(m_wallpaperFillColor);
  } else if (m_textureCache != nullptr && !m_wallpaperPath.empty()) {
    m_wallpaperLease.setCoverage(m_wallpaperCoverage);
    m_pendingWallpaperLease.setCoverage(m_wallpaperCoverage);
    if (m_shownWallpaperPath == m_wallpaperPath) {
      m_pendingWallpaperLease.reset();
      m_pendingWallpaperPath.clear();
    } else if (m_pendingWallpaperPath != m_wallpaperPath && renderContext() != nullptr) {
      m_pendingWallpaperLease = m_textureCache->acquire(
          SharedTextureRequest{.path = m_wallpaperPath, .coverage = m_wallpaperCoverage}, &renderContext()->backend(),
          [this]() {
            m_wallpaperDirty = true;
            requestLayout();
          }
      );
      m_pendingWallpaperPath = m_pendingWallpaperLease.empty() ? std::string{} : m_wallpaperPath;
    }

    if (!m_pendingWallpaperLease.empty()) {
      if (m_pendingWallpaperLease.failed()) {
        kLog.warn("failed to load lockscreen wallpaper {}", m_pendingWallpaperPath);
        m_pendingWallpaperLease.reset();
      } else if (m_pendingWallpaperLease.texture().id != 0) {
        m_wallpaperLease = std::move(m_pendingWallpaperLease);
        m_shownWallpaperPath = std::move(m_pendingWallpaperPath);
        m_pendingWallpaperPath.clear();
      }
    }

    const TextureHandle wallpaperTexture = m_wallpaperLease.texture();
    if (wallpaperTexture.id == 0) {
      loaded = false;
    } else {
      loaded = m_pendingWallpaperLease.empty();

      if (m_blurredWallpaperTexture.id != 0 && renderContext() != nullptr) {
        renderContext()->backend().makeCurrentNoSurface();
        renderContext()->textureManager().unload(m_blurredWallpaperTexture);
        m_blurredWallpaperTexture = {};
      }
      if (m_blurIntensity > 0.0F && renderContext() != nullptr) {
        auto* renderer = renderContext();
        renderer->makeCurrent(renderTarget());
        static constexpr int kBlurRounds = 3;
        const float blurRadius = m_blurIntensity * 40.0F;
        const std::uint32_t blurWidth = renderTarget().bufferWidth();
        const std::uint32_t blurHeight = renderTarget().bufferHeight();
        m_blurredWallpaperTexture = m_wallpaperBlurCache.get(
            renderer->backend(), wallpaperTexture, blurWidth, blurHeight, blurRadius, kBlurRounds
        );
      }
      const TextureHandle textureToDisplay =
          m_blurredWallpaperTexture.id != 0 ? m_blurredWallpaperTexture : wallpaperTexture;
      m_wallpaper->setTextures(
          textureToDisplay.id, {}, static_cast<float>(textureToDisplay.width),
          static_cast<float>(textureToDisplay.height), 0.0F, 0.0F
      );
      m_wallpaper->setTransition(WallpaperTransition::Fade, 0.0F, TransitionParams{});
      m_wallpaper->setFillMode(m_wallpaperFillMode);
      m_wallpaper->setFillColor(m_wallpaperFillColor);
    }
  } else if (m_wallpaperPath.empty()) {
    m_wallpaperLease.reset();
    m_pendingWallpaperLease.reset();
    m_shownWallpaperPath.clear();
    m_pendingWallpaperPath.clear();
    m_wallpaper->setTextures({}, {}, 0.0F, 0.0F, 0.0F, 0.0F);
  } else {
    loaded = false;
  }

  m_wallpaperDirty = !loaded;
}

void LockSurface::releaseCaptureTextures() {
  if (renderContext() == nullptr) {
    m_blurredWallpaperTexture = {};
    m_captureSourceTexture = {};
    m_blurredDesktopTexture = {};
    m_transitionCaptureTexture = {};
    m_blurCache.destroy();
    m_wallpaperBlurCache.destroy();
    return;
  }

  auto& tm = renderContext()->textureManager();
  renderContext()->backend().makeCurrentNoSurface();
  if (m_blurredWallpaperTexture.id != 0) {
    tm.unload(m_blurredWallpaperTexture);
    m_blurredWallpaperTexture = {};
  }
  if (m_captureSourceTexture.id != 0) {
    tm.unload(m_captureSourceTexture);
    m_captureSourceTexture = {};
  }
  if (m_blurredDesktopTexture.id != 0) {
    tm.unload(m_blurredDesktopTexture);
    m_blurredDesktopTexture = {};
  }
  if (m_transitionCaptureTexture.id != 0) {
    tm.unload(m_transitionCaptureTexture);
    m_transitionCaptureTexture = {};
  }
  m_blurCache.destroy();
  m_wallpaperBlurCache.destroy();
}

void LockSurface::applyBlurredDesktopTexture() {
  if (!m_captureDirty || !usesDesktopCaptureBackground()) {
    return;
  }

  auto* renderer = renderContext();
  if (renderer == nullptr) {
    return;
  }

  const ScreencopyImage& capture = *m_desktopCapture;
  const int texW = capture.width;
  const int texH = capture.height;
  if (texW <= 0 || texH <= 0) {
    return;
  }

  renderer->makeCurrent(renderTarget());
  auto& tm = renderer->textureManager();
  if (m_captureSourceTexture.id != 0) {
    tm.unload(m_captureSourceTexture);
    m_captureSourceTexture = {};
  }
  if (m_blurredDesktopTexture.id != 0) {
    tm.unload(m_blurredDesktopTexture);
    m_blurredDesktopTexture = {};
  }

  m_captureSourceTexture = tm.loadFromRgba(capture.rgba.data(), texW, texH, false);
  if (m_captureSourceTexture.id == 0) {
    return;
  }

  static constexpr int kBlurRounds = 3;
  const float blurRadius = m_blurIntensity * 40.0F;
  const std::uint32_t blurWidth = renderTarget().bufferWidth();
  const std::uint32_t blurHeight = renderTarget().bufferHeight();
  m_blurredDesktopTexture =
      m_blurCache.get(renderer->backend(), m_captureSourceTexture, blurWidth, blurHeight, blurRadius, kBlurRounds);
  if (m_blurredDesktopTexture.id == 0) {
    return;
  }

  m_wallpaper->setTextures(
      m_blurredDesktopTexture.id, {}, static_cast<float>(m_blurredDesktopTexture.width),
      static_cast<float>(m_blurredDesktopTexture.height), 0.0F, 0.0F
  );
  m_wallpaper->setTransition(WallpaperTransition::Fade, 0.0F, TransitionParams{});
  m_wallpaper->setFillMode(m_wallpaperFillMode);
  m_wallpaper->setFillColor(rgba(0.0F, 0.0F, 0.0F, 0.0F));
  // Black under the blurred capture: its edges fade out, and the live session must never show through.
  m_backdrop->setVisible(true);
  m_backdrop->setStyle(RoundedRectStyle{.fill = rgba(0.0F, 0.0F, 0.0F, 1.0F), .fillMode = FillMode::Solid});
  m_captureDirty = false;
  m_wallpaperDirty = false;
}

void LockSurface::onGpuResourcesInvalidated() {
  const bool exitWasActive =
      m_transitionPhase == TransitionPhase::Exiting || m_transitionPhase == TransitionPhase::ExitEndpoint;
  const bool entryWasActive = m_transitionPhase == TransitionPhase::Cover
      || m_transitionPhase == TransitionPhase::FinalPrime
      || m_transitionPhase == TransitionPhase::Ready
      || m_transitionPhase == TransitionPhase::Entering;
  cancelTransitionAnimation();
  releaseCaptureTextures();

  if (exitWasActive) {
    m_transitionPhase = TransitionPhase::ExitComplete;
    m_transitionProgress = 0.0F;
    notifyTransitionStateChanged();
  } else if (entryWasActive) {
    m_transitionPhase = TransitionPhase::Stable;
    m_transitionProgress = 1.0F;
  }

  m_captureDirty = true;
  m_wallpaperDirty = true;
  requestLayout();
}

void LockSurface::prepareForGraphicsReset() noexcept {
  m_blurCache.abandon();
  m_wallpaperBlurCache.abandon();
  m_blurredWallpaperTexture = {};
  m_captureSourceTexture = {};
  m_blurredDesktopTexture = {};
  m_transitionCaptureTexture = {};
  m_captureDirty = true;
  m_wallpaperDirty = true;
}

void LockSurface::forceRepaintAfterResume() {
  discardPendingFrameCallback();
  const bool exiting =
      m_transitionPhase == TransitionPhase::Exiting || m_transitionPhase == TransitionPhase::ExitEndpoint;
  cancelTransitionAnimation();
  if (exiting) {
    m_transitionPhase = TransitionPhase::ExitComplete;
    m_transitionProgress = 0.0F;
    notifyTransitionStateChanged();
  } else if (m_transitionPhase != TransitionPhase::Disabled && m_transitionPhase != TransitionPhase::ExitComplete) {
    m_transitionPhase = TransitionPhase::Stable;
    m_transitionProgress = 1.0F;
  }
  syncTransitionCover();
  requestUpdate();
  requestRedraw();
}

void LockSurface::onFrameCallbackDone() {
  if (!m_firstFrameRendered) {
    m_firstFrameRendered = true;
    if (m_renderCallback) {
      m_renderCallback();
    }
  }

  if (m_transitionPhase == TransitionPhase::Cover) {
    m_transitionPhase = TransitionPhase::FinalPrime;
    requestUpdate();
    return;
  }
  if (m_transitionPhase == TransitionPhase::FinalPrime) {
    m_transitionPhase = TransitionPhase::Ready;
    if (m_enterTransitionRequested) {
      beginEnterAnimation();
    }
    notifyTransitionStateChanged();
    return;
  }
  if (m_transitionPhase == TransitionPhase::ExitEndpoint) {
    m_transitionPhase = TransitionPhase::ExitComplete;
    notifyTransitionStateChanged();
  }
}
