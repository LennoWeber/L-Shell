#pragma once

#include "config/config_types.h"
#include "config/layout_resolver.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
#include "shell/island/island_activity.h"
#include "shell/surface/glass.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class CompositorPlatform;
class ConfigService;
class Flex;
class Glyph;
class IpcService;
class Label;
class MprisService;
class Node;
class NotificationManager;
class ProgressBar;
class RenderContext;
class UPowerService;
struct wl_output;

struct IslandServices {
  CompositorPlatform& platform;
  ConfigService& config;
  RenderContext* renderContext = nullptr;
  IpcService* ipc = nullptr;
  MprisService* mpris = nullptr;
  NotificationManager* notifications = nullptr;
  UPowerService* upower = nullptr;
};

// The top-center capsule of the island and flow layouts. One layer surface per output, sized for the expanded
// state; the visible capsule animates inside it and the input and blur regions follow the capsule.
class Island {
private:
  struct Instance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    // sceneRoot must be destroyed before `animations` — ~Node() calls cancelForOwner().
    AnimationManager animations;
    std::unique_ptr<Node> sceneRoot;
    InputDispatcher inputDispatcher;
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
    // 0 = hidden (flow with an empty workspace), 1 = shown; animated.
    float visibility = 1.0F;
    bool occupied = false; // active workspace on this output has windows
    bool pointerInside = false;
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  MprisService* m_mpris = nullptr;
  NotificationManager* m_notifications = nullptr;
  UPowerService* m_upower = nullptr;

  std::optional<IslandConfig> m_islandConfig; // nullopt when the active layout has no island
  noctalia::layout::IslandVisibility m_visibility = noctalia::layout::IslandVisibility::Always;
  shell::island::ActivityTracker m_activities;
  shell::island::Presentation m_presentation = shell::island::Presentation::Collapsed;
  Timer m_activityTimer;
  Timer m_clockTimer;
  std::vector<std::unique_ptr<Instance>> m_instances;
};
