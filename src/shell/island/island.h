#pragma once

#include "config/config_types.h"
#include "config/layout_resolver.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "shell/bar/widget_action.h"
#include "shell/island/island_activity.h"
#include "shell/surface/glass.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class CompositorPlatform;
class ConfigService;
class Flex;
class Glyph;
class InputArea;
class IpcService;
class Label;
class MprisService;
class Node;
class NotificationManager;
class PanelManager;
class ProgressBar;
class RenderContext;
class UPowerService;
struct OsdContent;
struct PointerEvent;
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
};

// The top-center capsule of the island and flow layouts. One layer surface per output, sized for the expanded
// state; the visible capsule animates inside it and the input and blur regions follow the capsule.
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
  // Workspace, window or overview change: re-evaluates whether each output shows home.
  void onWorkspaceChanged();
  void onSecondTick();
  bool onPointerEvent(const PointerEvent& event);
  // OSD redirect: true when the island shows this content instead of the OSD.
  [[nodiscard]] bool takeOsdContent(const OsdContent& content);
  void onNotificationPosted(const std::string& appName, const std::string& summary, const std::string& glyph);
  void onPowerChanged(bool charging, float percent);
  // MPRIS state changed: refreshes the ambient now-playing indicator.
  void onMediaChanged();
  [[nodiscard]] bool isActive() const noexcept;
  [[nodiscard]] std::vector<wl_surface*> surfaces() const;

private:
  struct Instance;

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
  void post(shell::island::Activity activity);
  void scheduleActivityTimer();
  void onActivityTimer();
  void activateClick(Instance& instance);
  [[nodiscard]] Instance* instanceForSurface(wl_surface* surface) const noexcept;
  [[nodiscard]] float capsuleWidth(const Instance& instance) const noexcept;
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
    shell::glass::GlassNodes glass;
    Node* capsule = nullptr;
    Label* clock = nullptr;
    Node* ambientIndicator = nullptr;
    Flex* expandedContent = nullptr;
    Glyph* activityGlyph = nullptr;
    Label* activityTitle = nullptr;
    Label* activitySubtitle = nullptr;
    ProgressBar* activityLevel = nullptr;
    // 0 = collapsed capsule, 1 = fully expanded; animated.
    float expansion = 0.0F;
    AnimationManager::Id expansionAnim = 0;
    // 0 = hidden (flow with an empty workspace), 1 = shown; animated.
    float visibility = 1.0F;
    AnimationManager::Id visibilityAnim = 0;
    bool occupied = false; // active workspace on this output has windows
    bool pointerInside = false;
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  PanelManager* m_panels = nullptr;
  MprisService* m_mpris = nullptr;
  NotificationManager* m_notifications = nullptr;
  UPowerService* m_upower = nullptr;

  std::optional<IslandConfig> m_islandConfig; // nullopt when the active layout has no island
  noctalia::config::IslandVisibility m_visibility = noctalia::config::IslandVisibility::Always;
  noctalia::bar::WidgetAction m_clickAction; // parsed IslandConfig::clickAction
  shell::island::ActivityTracker m_activities;
  shell::island::Presentation m_presentation = shell::island::Presentation::Collapsed;
  // Wakes the tracker when its transient activity expires; the remaining time itself lives in the tracker.
  Timer m_activityTimer;
  std::vector<std::unique_ptr<Instance>> m_instances;
};
