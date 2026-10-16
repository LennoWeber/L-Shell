#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "shell/bar/widget_action.h"
#include "shell/island/island_activity.h"
#include "shell/surface/material.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Bar;
class CompositorPlatform;
class ConfigService;
class Glyph;
class InputArea;
class IpcService;
class Label;
class MprisService;
class Node;
class NotificationManager;
class PanelManager;
class ProgressBar;
class RectNode;
class RenderContext;
class UPowerService;
enum class NotificationEvent;
struct InputRect;
struct Notification;
struct OsdContent;
struct PointerEvent;
struct UPowerChange;
struct WaylandOutput;
struct wl_output;
struct wl_surface;

struct IslandServices {
  CompositorPlatform& platform;
  ConfigService& config;
  RenderContext* renderContext = nullptr;
  IpcService* ipc = nullptr;
  PanelManager* panels = nullptr;
  MprisService* mpris = nullptr;
  NotificationManager* notifications = nullptr;
  UPowerService* upower = nullptr;
  // The bar engine: the island centers itself in the top bar's strip.
  Bar* bar = nullptr;
};

// The live middle of the menu bar ([island]). It sits centered in the top bar and shows the date and time;
// events (volume, brightness, media, charger, …) expand it below the bar for a moment, hovering shows what is
// playing, a click opens Notification Center under it. One layer surface per output, sized for the expanded state;
// the visible capsule animates inside it and the input and blur regions follow the capsule.
class Island {
public:
  Island();
  ~Island();

  Island(const Island&) = delete;
  Island& operator=(const Island&) = delete;

  void initialize(const IslandServices& services);
  // Re-reads [island]: creates, retunes or removes the island.
  void reload();
  void onOutputChange();
  // Hidden while the lock-screen widget editor owns the screen, like the bar and the dock.
  void setSuppressed(bool suppressed);
  void onSecondTick();
  bool onPointerEvent(const PointerEvent& event);
  // OSD redirect: true when the island shows this content instead of the OSD.
  [[nodiscard]] bool takeOsdContent(const OsdContent& content);
  // UPower changed (Application owns UPower's single callback): charger plugged/unplugged, low battery.
  void onPowerChanged(const UPowerChange& change);
  // MPRIS state changed (Application owns MPRIS's single callback): refreshes the now-playing indicator.
  void onMediaChanged();
  // Microphone, camera or screen capture in use (filtered like the privacy OSD): a coloured dot at the
  // pill's trailing end, like the macOS recording indicators.
  void setPrivacy(bool mic, bool camera, bool screen);
  void requestLayout();
  void requestRedraw();
  [[nodiscard]] bool isActive() const noexcept;
  [[nodiscard]] std::vector<wl_surface*> surfaces() const;
  // The capsule's rectangle per output, for the panel click shield's exclusions.
  [[nodiscard]] std::vector<InputRect> surfaceRectsForOutput(wl_output* output) const;

private:
  struct Instance;

  // Reads [island]; true when the island appeared, vanished or changed.
  [[nodiscard]] bool applyConfig();
  void syncInstances();
  void createInstance(const WaylandOutput& output);
  void destroyInstances();
  void buildScene(Instance& instance, std::uint32_t width, std::uint32_t height);
  void prepareFrame(Instance& instance, bool needsUpdate, bool needsLayout);
  void layoutCapsule(Instance& instance);
  void updateContent(Instance& instance);
  void animatePresentation(Instance& instance);
  void animateVisibility(Instance& instance);
  void syncInputRegion(Instance& instance);
  void syncBlurRegion(Instance& instance);
  // False when the island leaves the activity out (nothing to show, kind off, a higher-priority one showing).
  bool post(shell::island::Activity activity);
  // NotificationManager subscription (multi-listener), registered in initialize().
  void onNotificationEvent(const Notification& notification, NotificationEvent event);
  void scheduleActivityTimer();
  void onActivityTimer();
  void activateClick(Instance& instance);
  void refreshAll();
  // The first enabled top bar on the output (with its per-monitor overrides), the strip the island sits in.
  [[nodiscard]] std::optional<BarConfig> topBarOn(wl_output* output) const;
  [[nodiscard]] Instance* instanceForSurface(wl_surface* surface) const noexcept;
  [[nodiscard]] float capsuleWidth(const Instance& instance) const noexcept;
  void animateAmbientRoom(Instance& instance);
  // 1 while the compact pill needs its extra room (a playing track or a privacy dot), else 0.
  [[nodiscard]] float ambientRoomTarget() const noexcept;
  [[nodiscard]] float capsuleHeight(const Instance& instance) const noexcept;

  struct Instance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    // sceneRoot must be destroyed before `animations` — ~Node() calls cancelForOwner().
    AnimationManager animations;
    std::unique_ptr<Node> sceneRoot;
    InputDispatcher inputDispatcher;
    InputArea* inputArea = nullptr; // covers the capsule; the surface's input region follows it
    shell::material::Nodes material;
    Node* capsule = nullptr;
    Label* clock = nullptr;
    Node* ambientIndicator = nullptr;
    RectNode* privacyDot = nullptr;
    Node* expandedContent = nullptr;
    Glyph* activityGlyph = nullptr;
    Label* activityTitle = nullptr;
    Label* activitySubtitle = nullptr;
    ProgressBar* activityLevel = nullptr;
    // 0 = collapsed capsule, 1 = fully expanded; animated.
    float expansion = 0.0F;
    float expansionTarget = 0.0F;
    AnimationManager::Id expansionAnim = 0;
    // 0..1 share of the compact state's extra room for the ambient indicator; animated.
    float ambientExtra = 0.0F;
    float ambientTarget = 0.0F;
    AnimationManager::Id ambientAnim = 0;
    // 0 = hidden (while suppressed, and fading in on creation), 1 = shown; animated.
    float visibility = 1.0F;
    AnimationManager::Id visibilityAnim = 0;
    // Where `visibility` is heading; nullopt until the first pass. Repeated workspace events then cost nothing.
    std::optional<bool> shownTarget;
    bool pointerInside = false;
    // Top of the collapsed capsule (logical px): centered in the top bar, or the floating margin without one.
    float stripTop = 0.0F;
    float uiScale = 1.0F;
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  PanelManager* m_panels = nullptr;
  MprisService* m_mpris = nullptr;
  NotificationManager* m_notifications = nullptr;
  UPowerService* m_upower = nullptr;
  Bar* m_bar = nullptr;

  struct Privacy {
    bool mic = false;
    bool camera = false;
    bool screen = false;

    [[nodiscard]] bool any() const noexcept { return mic || camera || screen; }
    bool operator==(const Privacy&) const = default;
  };
  Privacy m_privacy;

  std::optional<IslandConfig> m_islandConfig; // nullopt while [island] is disabled
  noctalia::bar::WidgetAction m_clickAction; // parsed IslandConfig::clickAction
  shell::island::ActivityTracker m_activities;
  shell::island::Presentation m_presentation = shell::island::Presentation::Collapsed;
  // Wakes the tracker when its transient activity expires; the remaining time itself lives in the tracker.
  Timer m_activityTimer;
  int m_notificationToken = -1;
  // Last seen power state, to tell a charger change or a low-battery crossing from a plain refresh.
  std::optional<bool> m_lastCharging;
  std::optional<float> m_lastBatteryPercent;
  bool m_suppressed = false;
  std::string m_clockText;             // last clock text; the island repaints when the minute turns
  std::vector<std::unique_ptr<Instance>> m_instances;
};
