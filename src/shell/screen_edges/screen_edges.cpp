#include "shell/screen_edges/screen_edges.h"

#include "compositors/compositor_platform.h"
#include "config/config_service.h"
#include "core/log.h"
#include "render/scene/input_area.h"
#include "shell/bar/widget_action_dispatcher.h"
#include "shell/panel/panel_manager.h"
#include "ui/builders.h"
#include "wayland/surface.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>

namespace {

  constexpr Logger kLog("edges");

  // Below this an edge strip is too short to find; above the whole edge it would cover the corners.
  constexpr float kMinLength = 0.05F;

} // namespace

ScreenEdges::ScreenEdges() = default;

ScreenEdges::~ScreenEdges() { destroyInstances(); }

void ScreenEdges::initialize(
    CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
    PanelManager* panels
) {
  m_platform = &platform;
  m_config = &config;
  m_renderContext = renderContext;
  m_ipc = ipc;
  m_panels = panels;
  // Strips are created by onOutputChange(), last, like the hot corners.
  m_edgesConfig = config.resolvedLayout().edges;
}

void ScreenEdges::reload() {
  if (m_config == nullptr) {
    return;
  }
  const auto& next = m_config->resolvedLayout().edges;
  if (next == m_edgesConfig) {
    return;
  }
  m_edgesConfig = next;
  destroyInstances();
  syncInstances();
}

void ScreenEdges::setSuppressed(bool suppressed) {
  if (suppressed == m_suppressed) {
    return;
  }
  m_suppressed = suppressed;
  destroyInstances();
  syncInstances();
}

void ScreenEdges::onOutputChange() {
  if (m_config == nullptr) {
    return;
  }
  destroyInstances();
  syncInstances();
}

void ScreenEdges::syncInstances() {
  if (m_suppressed) {
    return;
  }
  if (m_platform == nullptr || !m_edgesConfig.has_value()) {
    return;
  }
  struct EdgeSpec {
    Edge edge;
    const std::string* value;
  };
  const EdgesConfig& config = *m_edgesConfig;
  const EdgeSpec specs[] = {
      {Edge::Top, &config.top},
      {Edge::Bottom, &config.bottom},
      {Edge::Left, &config.left},
      {Edge::Right, &config.right},
  };

  for (const auto& out : m_platform->outputs()) {
    if (!out.done || out.connectorName.empty() || out.output == nullptr) {
      continue;
    }
    auto instance = std::make_unique<OutputInstance>();
    instance->outputName = out.name;
    instance->output = out.output;
    instance->logicalWidth = out.logicalWidth > 0 ? out.logicalWidth : out.width;
    instance->logicalHeight = out.logicalHeight > 0 ? out.logicalHeight : out.height;
    for (const auto& spec : specs) {
      auto parsed = noctalia::bar::parseWidgetAction(*spec.value);
      if (!parsed.has_value()) {
        kLog.warn("layout edge action \"{}\": {}", *spec.value, parsed.error());
        continue;
      }
      if (!parsed->isBound()) {
        continue;
      }
      buildStrip(*instance, spec.edge, *parsed);
    }
    m_instances.push_back(std::move(instance));
  }
}

void ScreenEdges::destroyInstances() { m_instances.clear(); }

InputRect
ScreenEdges::stripRect(Edge edge, std::int32_t outputWidth, std::int32_t outputHeight, const EdgesConfig& config) {
  assert(outputWidth > 0 && outputHeight > 0 && "strip on an output without a size");
  assert(config.length > 0.0F && config.length <= 1.0F && "edge length is a fraction of the edge");
  const float fraction = std::clamp(config.length, kMinLength, 1.0F);
  const std::int32_t thickness = std::max(config.triggerSize, 1);
  const bool horizontal = edge == Edge::Top || edge == Edge::Bottom;
  const std::int32_t span = horizontal ? outputWidth : outputHeight;
  const auto length = std::max(static_cast<std::int32_t>(static_cast<float>(span) * fraction), 1);
  const std::int32_t offset = (span - length) / 2;
  switch (edge) {
  case Edge::Top:
    return InputRect{offset, 0, length, thickness};
  case Edge::Bottom:
    return InputRect{offset, outputHeight - thickness, length, thickness};
  case Edge::Left:
    return InputRect{0, offset, thickness, length};
  case Edge::Right:
    return InputRect{outputWidth - thickness, offset, thickness, length};
  }
  return {};
}

std::string_view ScreenEdges::panelPositionForEdge(Edge edge) noexcept {
  switch (edge) {
  case Edge::Top:
    return "top_center";
  case Edge::Bottom:
    return "bottom_center";
  case Edge::Left:
    return "center_left";
  case Edge::Right:
    return "center_right";
  }
  return "center";
}

void ScreenEdges::buildStrip(OutputInstance& instance, Edge edge, const noctalia::bar::WidgetAction& action) {
  const InputRect rect = stripRect(edge, instance.logicalWidth, instance.logicalHeight, *m_edgesConfig);
  if (rect.width <= 0 || rect.height <= 0) {
    return;
  }

  auto strip = std::make_unique<Strip>();
  strip->edge = edge;
  strip->action = action;

  // A single anchor centers the strip along its edge (layer-shell centers on the free axis).
  std::uint32_t anchor = 0;
  switch (edge) {
  case Edge::Top:
    anchor = LayerShellAnchor::Top;
    break;
  case Edge::Bottom:
    anchor = LayerShellAnchor::Bottom;
    break;
  case Edge::Left:
    anchor = LayerShellAnchor::Left;
    break;
  case Edge::Right:
    anchor = LayerShellAnchor::Right;
    break;
  }

  LayerSurfaceConfig surfaceConfig{
      .nameSpace = "noctalia-edge",
      .layer = LayerShellLayer::Top,
      .anchor = anchor,
      .width = static_cast<std::uint32_t>(rect.width),
      .height = static_cast<std::uint32_t>(rect.height),
      // Ignore exclusive zones so the strip sits on the physical screen edge, like a hot corner.
      .exclusiveZone = -1,
  };
  strip->surface = std::make_unique<LayerSurface>(m_platform->wayland(), surfaceConfig);
  strip->surface->initialize(instance.output);

  Strip* stripPtr = strip.get();
  OutputInstance* instancePtr = &instance;
  auto inputArea = ui::inputArea({});
  inputArea->setPosition(0.0F, 0.0F);
  inputArea->setSize(static_cast<float>(rect.width), static_cast<float>(rect.height));
  inputArea->setOnEnter([this, instancePtr, stripPtr](const InputArea::PointerData&) {
    armStrip(*instancePtr, *stripPtr);
  });
  inputArea->setOnLeave([this, stripPtr]() { disarmStrip(*stripPtr); });

  strip->sceneRoot = std::move(inputArea);
  strip->inputDispatcher.setSceneRoot(strip->sceneRoot.get());
  strip->surface->setRenderContext(m_renderContext);
  strip->surface->setSceneRoot(strip->sceneRoot.get());
  strip->surface->requestRedraw();
  instance.strips.push_back(std::move(strip));
}

void ScreenEdges::armStrip(OutputInstance& instance, Strip& strip) {
  if (!m_edgesConfig.has_value()) {
    return;
  }
  const std::int32_t delayMs = std::max(0, m_edgesConfig->delayMs);
  if (delayMs == 0) {
    strip.dwellTimer.stop();
    triggerStrip(instance, strip);
    return;
  }
  OutputInstance* instancePtr = &instance;
  Strip* stripPtr = &strip;
  strip.dwellTimer.start(std::chrono::milliseconds(delayMs), [this, instancePtr, stripPtr]() {
    triggerStrip(*instancePtr, *stripPtr);
  });
}

void ScreenEdges::disarmStrip(Strip& strip) { strip.dwellTimer.stop(); }

void ScreenEdges::triggerStrip(OutputInstance& instance, Strip& strip) {
  const auto& action = strip.action;
  if (action.kind == noctalia::bar::WidgetAction::Kind::Ipc && noctalia::bar::isAnchoredPanelVerb(action.verb)) {
    if (m_panels == nullptr) {
      return;
    }
    const auto args = noctalia::bar::parsePanelVerbArgs(action.args);
    if (args.panelId.empty()) {
      kLog.warn("layout edge action \"{}\" needs a panel id", action.verb);
      return;
    }
    const std::string panelId(args.panelId);
    // Resting on the edge again while its own drawer is open does nothing: re-opening would flicker.
    if (m_panels->isOpenPanel(panelId)) {
      return;
    }
    const PanelOpenRequest request{
        .output = instance.output,
        .context = args.panelContext,
        .screenPosition = panelPositionForEdge(strip.edge),
        .dismissOnPointerLeave = m_edgesConfig.has_value() && m_edgesConfig->closeOnLeave,
    };
    m_panels->openPanel(panelId, request);
    return;
  }
  noctalia::bar::WidgetActionDispatcher dispatcher;
  dispatcher.setIpcService(m_ipc);
  (void)dispatcher.run(action, IpcInvocationContext{.output = instance.output});
}

bool ScreenEdges::onPointerEvent(const PointerEvent& event) {
  if (event.surface == nullptr) {
    return false;
  }
  for (const auto& instance : m_instances) {
    for (const auto& strip : instance->strips) {
      if (!strip->surface || event.surface != strip->surface->wlSurface()) {
        continue;
      }
      const auto sx = static_cast<float>(event.sx);
      const auto sy = static_cast<float>(event.sy);
      switch (event.type) {
      case PointerEvent::Type::Enter:
        strip->inputDispatcher.pointerEnter(sx, sy, event.serial);
        return true;
      case PointerEvent::Type::Leave:
        strip->inputDispatcher.pointerLeave();
        return true;
      case PointerEvent::Type::Motion:
        strip->inputDispatcher.pointerMotion(sx, sy, event.serial);
        return true;
      case PointerEvent::Type::Button:
        return strip->inputDispatcher.pointerButton(
            sx, sy, event.button, event.pressed, event.serial, event.time, event.touch
        );
      case PointerEvent::Type::Axis:
        return strip->inputDispatcher.pointerAxis(
            sx, sy, event.axis, event.axisSource, event.axisValue, event.axisDiscrete, event.axisValue120,
            event.axisLines
        );
      }
      return false;
    }
  }
  return false;
}

std::vector<wl_surface*> ScreenEdges::surfaces() const {
  std::vector<wl_surface*> out;
  for (const auto& instance : m_instances) {
    for (const auto& strip : instance->strips) {
      if (strip->surface && strip->surface->wlSurface() != nullptr) {
        out.push_back(strip->surface->wlSurface());
      }
    }
  }
  return out;
}

std::vector<InputRect> ScreenEdges::surfaceRectsForOutput(wl_output* output) const {
  std::vector<InputRect> out;
  if (!m_edgesConfig.has_value()) {
    return out;
  }
  for (const auto& instance : m_instances) {
    if (instance->output != output) {
      continue;
    }
    for (const auto& strip : instance->strips) {
      out.push_back(stripRect(strip->edge, instance->logicalWidth, instance->logicalHeight, *m_edgesConfig));
    }
  }
  return out;
}
