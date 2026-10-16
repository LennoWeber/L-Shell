#pragma once

#include "capture/screencopy_capture.h"
#include "config/config_service.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/core/blur_cache.h"
#include "render/core/color.h"
#include "render/core/lockscreen_transition_types.h"
#include "render/core/shared_texture_cache.h"
#include "render/core/texture_manager.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "shell/lockscreen/lockscreen_login_box.h"
#include "wayland/surface.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

struct ext_session_lock_surface_v1;
struct ext_session_lock_v1;
struct wl_output;

class Button;
class Box;
class Flex;
class Glyph;
class HttpClient;
class Image;
class Input;
class Label;
class LockscreenTransitionCover;
class MprisService;
class Renderer;
class SessionActionRunner;
class WallpaperNode;
class WeatherService;
struct KeyboardEvent;
struct PointerEvent;
struct SessionPanelActionConfig;

class LockscreenWidgetsHost;

class LockSurface : public Surface {
public:
  explicit LockSurface(WaylandConnection& connection, ConfigService* config = nullptr);
  ~LockSurface() override;

  using Surface::initialize;
  bool initialize() override { return false; }
  bool initialize(ext_session_lock_v1* lock, wl_output* output, std::int32_t scale);
  void setLockedState(bool locked);
  void setPromptState(std::string user, std::string password, std::string status, bool error, bool authenticating);
  void setKeyboardIndicators(bool capsLock, bool hasMultipleLayouts, bool layoutSwitchable, std::string layoutLabel);
  void setTextureCache(SharedTextureCache* cache) noexcept { m_textureCache = cache; }
  void setWallpaperPath(std::string wallpaperPath);
  void setWallpaperFillMode(WallpaperFillMode fillMode);
  void setWallpaperCoverage(TextureCoverage coverage);
  void setWallpaperFillColor(Color fillColor);
  void setDesktopCapture(std::optional<ScreencopyImage> capture, bool useAsBackground);
  void configureTransition(
      std::optional<LockscreenTransitionKind> transition, const LockscreenTransitionParams& params, float durationMs
  );
  void startEnterTransition();
  void startExitTransition();
  [[nodiscard]] bool transitionInputReady() const noexcept;
  [[nodiscard]] bool exitTransitionComplete() const noexcept;
  void setTransitionCallback(std::function<void()> callback) { m_transitionCallback = std::move(callback); }
  void setBackgroundStyle(float blurIntensity, float tintIntensity);
  void setBlackout(bool blackout);
  [[nodiscard]] bool isBlackout() const noexcept { return m_blackout; }
  void setOnLogin(std::function<void()> onLogin);
  void setOnCycleLayout(std::function<void()> onCycleLayout);
  void setOnPasswordChanged(std::function<void(const std::string&)> onPasswordChanged);
  void setLoginBoxServices(
      SessionActionRunner* sessionActions, MprisService* mpris, const WeatherService* weather, HttpClient* httpClient
  );
  void selectAllPassword();
  void clearPasswordSelection();
  void onThemeChanged();
  void onGpuResourcesInvalidated();
  void prepareForGraphicsReset() noexcept;
  void forceRepaintAfterResume();
  void onPointerEvent(const PointerEvent& event);
  void onKeyboardEvent(const KeyboardEvent& event);
  [[nodiscard]] wl_output* output() const noexcept { return m_output; }
  void syncOutputScale(std::int32_t bufferScale, std::uint32_t configuredScaleNumerator);
  [[nodiscard]] bool usesDesktopCaptureBackground() const noexcept;
  [[nodiscard]] Node* widgetLayer() noexcept { return m_widgetLayer; }
  void setOutputKey(std::string outputKey) { m_outputKey = std::move(outputKey); }
  void setWidgetsHost(LockscreenWidgetsHost* host) noexcept { m_widgetsHost = host; }

  [[nodiscard]] bool firstFrameRendered() const noexcept { return m_firstFrameRendered; }
  void setRenderCallback(std::function<void()> callback) { m_renderCallback = std::move(callback); }

  static void handleConfigure(
      void* data, ext_session_lock_surface_v1* lockSurface, std::uint32_t serial, std::uint32_t width,
      std::uint32_t height
  );

protected:
  void onFrameCallbackDone() override;

private:
  enum class TransitionPhase : std::uint8_t {
    Disabled,
    Cover,
    FinalPrime,
    Ready,
    Entering,
    Stable,
    Exiting,
    ExitEndpoint,
    ExitComplete,
  };

  void prepareFrame(bool needsUpdate, bool needsLayout);
  [[nodiscard]] bool ensureTransitionCaptureTexture();
  void layoutCoverOnly(std::uint32_t width, std::uint32_t height);
  void layoutTransitionCover();
  void syncTransitionCover();
  void cancelTransitionAnimation();
  AnimationManager::Id startTransitionAnimation(
      float from, float to, float durationMs, std::function<void(float)> setter, std::function<void()> onComplete,
      const void* owner
  );
  void beginEnterAnimation();
  void notifyTransitionStateChanged();
  void applyWallpaperTexture();
  void applyBlurredDesktopTexture();
  void releaseCaptureTextures();
  void layoutScene(std::uint32_t width, std::uint32_t height);
  void hideLoginChrome();
  void updateCopy();
  void refreshClock();
  void syncInfoExtras(Renderer& renderer);
  void syncAvatar(Renderer& renderer);
  void syncPillChrome(bool force);
  void startErrorShake();
  void rebuildSessionButtons();
  [[nodiscard]] std::vector<SessionPanelActionConfig> resolveSessionActions() const;
  [[nodiscard]] lockscreen_login_box::LoginBoxStyle resolveLoginStyle() const;
  [[nodiscard]] bool isLoginBoxEnabled() const;
  [[nodiscard]] bool cornerClock() const;
  [[nodiscard]] std::string resolveStatusText(const lockscreen_login_box::LoginBoxStyle& style, bool& isError) const;
  [[nodiscard]] bool passwordFieldContainsPoint(float sceneX, float sceneY) const;
  void focusPasswordField();

  ext_session_lock_surface_v1* m_lockSurface = nullptr;
  wl_output* m_output = nullptr;
  bool m_receivedConfigure = false;
  ConfigService* m_config = nullptr;
  bool m_ink = false; // lockscreen.ink: palette ink on paper instead of white on a dim
  // Declared before m_root so it outlives the scene: ~Node cancels its animations through this manager.
  AnimationManager m_animations;
  Node m_root;
  Node* m_backgroundLayer = nullptr;
  Node* m_widgetLayer = nullptr;
  LockscreenTransitionCover* m_transitionCover = nullptr;
  WallpaperNode* m_wallpaper = nullptr;
  Box* m_tintOverlay = nullptr;
  Box* m_backdrop = nullptr;
  // Top center: date line (with inline weather) over the large clock, and a now-playing capsule under it.
  Flex* m_clockColumn = nullptr;
  Label* m_dateLabel = nullptr;
  Glyph* m_weatherGlyph = nullptr;
  Label* m_weatherTemp = nullptr;
  Label* m_clockLabel = nullptr;
  Flex* m_nowPlaying = nullptr;
  Image* m_mediaArt = nullptr;
  Glyph* m_mediaFallbackGlyph = nullptr;
  Label* m_mediaTitle = nullptr;
  Label* m_mediaArtist = nullptr;
  // Bottom center: avatar, display name and the glass password pill (the login box).
  Flex* m_loginPanel = nullptr;
  Flex* m_avatarFrame = nullptr;
  Image* m_avatarImage = nullptr;
  Glyph* m_avatarGlyph = nullptr;
  Label* m_userName = nullptr;
  Flex* m_pill = nullptr;
  Input* m_passwordField = nullptr;
  Glyph* m_capsLockGlyph = nullptr;
  Button* m_loginButton = nullptr;
  Flex* m_statusChip = nullptr;
  Label* m_statusLabel = nullptr;
  // Bottom-right corner: keyboard layout chip and session action circles.
  Flex* m_cornerRow = nullptr;
  Button* m_layoutChip = nullptr;
  Flex* m_sessionRow = nullptr;
  std::vector<Button*> m_sessionButtons;
  SharedTextureCache* m_textureCache = nullptr;
  SharedTextureCache::Lease m_wallpaperLease;
  SharedTextureCache::Lease m_pendingWallpaperLease;
  TextureHandle m_blurredWallpaperTexture{};
  TextureHandle m_captureSourceTexture{};
  TextureHandle m_blurredDesktopTexture{};
  TextureHandle m_transitionCaptureTexture{};
  BlurCache m_blurCache;
  BlurCache m_wallpaperBlurCache;
  std::optional<ScreencopyImage> m_desktopCapture;
  LockscreenTransitionKind m_transition = LockscreenTransitionKind::Fade;
  LockscreenTransitionParams m_transitionParams;
  TransitionPhase m_transitionPhase = TransitionPhase::Disabled;
  AnimationManager::Id m_transitionAnimation = 0;
  float m_transitionProgress = 1.0F;
  float m_transitionDurationMs = 350.0F;
  float m_blurIntensity = 0.5F;
  float m_tintIntensity = 0.3F;
  bool m_blackout = false;
  bool m_useDesktopCaptureBackground = false;
  bool m_enterTransitionRequested = false;
  bool m_captureDirty = true;
  std::string m_wallpaperPath;
  std::string m_shownWallpaperPath;
  std::string m_pendingWallpaperPath;
  WallpaperFillMode m_wallpaperFillMode = WallpaperFillMode::Crop;
  TextureCoverage m_wallpaperCoverage;
  Color m_wallpaperFillColor = rgba(0.0F, 0.0F, 0.0F, 0.0F);
  bool m_wallpaperDirty = false;
  InputDispatcher m_inputDispatcher;
  std::function<void()> m_onLogin;
  std::function<void()> m_onCycleLayout;
  std::function<void(const std::string&)> m_onPasswordChanged;
  bool m_locked = false;
  std::string m_user;
  std::string m_password;
  std::string m_status;
  bool m_error = false;
  bool m_authenticating = false;
  bool m_capsLock = false;
  bool m_hasMultipleLayouts = false;
  bool m_layoutSwitchable = false;
  std::string m_layoutLabel;
  std::string m_outputKey;
  LockscreenWidgetsHost* m_widgetsHost = nullptr;
  bool m_firstFrameRendered = false;
  std::function<void()> m_renderCallback;
  std::function<void()> m_transitionCallback;

  SessionActionRunner* m_sessionActions = nullptr;
  MprisService* m_mpris = nullptr;
  const WeatherService* m_weather = nullptr;
  HttpClient* m_httpClient = nullptr;
  std::unordered_set<std::string> m_pendingArtDownloads;
  std::shared_ptr<void> m_aliveGuard = std::make_shared<int>(0);
  std::string m_lastArtUrl;
  std::vector<std::string> m_lastSessionActionKeys;
  Timer m_clockTimer;
  std::string m_avatarPath;
  bool m_avatarDirty = true;
  bool m_pillFocused = false;
  float m_pillRestX = 0.0F;
  float m_shakeOffset = 0.0F;
  AnimationManager::Id m_shakeAnimation = 0;
};
