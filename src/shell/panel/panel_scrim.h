#pragma once

#include "render/animation/animation_manager.h"
#include "render/scene/node.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class Box;
class ConfigService;
class RenderContext;
class WaylandConnection;
struct PointerEvent;
struct WaylandOutput;
struct wl_output;
struct wl_surface;

// The modal panels' veil ([shell.panel].scrim): one fullscreen, translucent layer surface per output
// that lets the compositor's blur rule do the work — the desktop dims and blurs behind a launcher,
// control center, clipboard or session that floats above it. Showing and hiding are quick fades; a
// click on the veil closes the panel that opened it.
class PanelScrim {
public:
  using CloseCallback = std::function<void()>;

  PanelScrim() = default;
  ~PanelScrim() = default;

  PanelScrim(const PanelScrim&) = delete;
  PanelScrim& operator=(const PanelScrim&) = delete;

  void initialize(WaylandConnection& wayland, ConfigService* config, RenderContext* renderContext);
  void onOutputChange();
  // True while a veil is up on any output (the panel manager consults this before opening another one).
  [[nodiscard]] bool isVisible() const noexcept { return m_visible; }

  void show(wl_output* output);
  void hide();
  // A press on the veil closes the panel; only the veil's own surfaces are consumed.
  [[nodiscard]] bool onPointerEvent(const PointerEvent& event);
  void setCloseCallback(CloseCallback callback) { m_onClose = std::move(callback); }

private:
  struct Instance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    AnimationManager animations;
    std::unique_ptr<Node> root;
    Box* fill = nullptr;
    float visibility = 0.0F;
  };

  void syncInstances();
  void createInstance(const WaylandOutput& output);
  void animateInstance(Instance& instance, bool show);

  WaylandConnection* m_wayland = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
  std::unordered_map<wl_surface*, Instance*> m_surfaceMap;
  bool m_visible = false;
  CloseCallback m_onClose;
};
