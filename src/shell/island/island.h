#pragma once

#include "config/config_types.h"
#include "config/layout_resolver.h"
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
  // The bar engine: hovering the island unfolds its mode's bar through a peek.
  Bar* bar = nullptr;
};

// The top-center capsule of the island and flow layouts. One layer surface per output, sized for the expanded
// state; the visible capsule animates inside it and the input and blur regions follow the capsule. Hovering it
// unfolds the mode's bar over the same strip (every bar widget, one gesture away); events expand it on its own.
class Island {
public:
  Island();
  ~Island();

  Island(const Island&) = delete;
  Island& operator=(const Island&) = delete;

  void initialize(const IslandServices& services);
  // Re-reads the resolved layout: creates, retunes or removes the island.
  void reload();
  void onOutputChange();
  // Hidden while the lock-screen widget editor owns the screen, like the bar and the dock.
  void setSuppressed(bool suppressed);
  // Workspace, window or overview change: re-evaluates whether each output shows home.
  void onWorkspaceChanged();
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

  // Reads the resolved layout; true when the island appeared, vanished or changed.
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
  // True when hovering unfolds a bar instead of expanding the capsule.
  [[nodiscard]] bool unfoldsIntoBar() const noexcept;
  // Peeks the mode's bar over the island's strip and hides the capsule until the bar slides away again.
  void unfold(Instance& instance);
  void stopUnfold(Instance& instance);
  void refreshAll();
  [[nodiscard]] bool shownOn(const Instance& instance) const;
  // The active workspace's name (its index when unnamed) on the instance's output, empty when unknown.
  [[nodiscard]] std::string activeWorkspaceText(const Instance& instance) const;
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
    // Collapsed-pill status around the clock: the active workspace at the leading end, the battery at the
    // trailing one. A playing track or a privacy dot takes their place while it shows.
    Label* workspace = nullptr;
    Glyph* battery = nullptr;
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
    // 0 = hidden (flow with an empty workspace), 1 = shown; animated.
    float visibility = 1.0F;
    AnimationManager::Id visibilityAnim = 0;
    // Where `visibility` is heading; nullopt until the first pass. Repeated workspace events then cost nothing.
    std::optional<bool> shownTarget;
    bool pointerInside = false;
    // While the unfolded bar is on screen the capsule steps aside.
    bool unfolded = false;
    Timer unfoldDwell;   // a short rest on the capsule before it unfolds, so passing pointers do not
    Timer unfoldRelease; // hands the peek back to the bar's own auto-hide once the pointer had time to reach it
    Timer unfoldWatch;   // notices the bar sliding away again
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
  std::string m_islandBarName; // ResolvedLayout::islandBarName

  struct Privacy {
    bool mic = false;
    bool camera = false;
    bool screen = false;

    [[nodiscard]] bool any() const noexcept { return mic || camera || screen; }
    bool operator==(const Privacy&) const = default;
  };
  Privacy m_privacy;

  std::optional<IslandConfig> m_islandConfig; // nullopt when the active layout has no island
  noctalia::config::IslandVisibility m_visibility = noctalia::config::IslandVisibility::Always;
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
  bool m_visibilityPassQueued = false; // coalesces bursts of workspace/window events into one pass
  std::string m_clockText;             // last clock text; the island repaints when the minute turns
  std::vector<std::unique_ptr<Instance>> m_instances;
};
