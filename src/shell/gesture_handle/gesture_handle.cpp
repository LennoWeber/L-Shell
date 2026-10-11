#include "shell/gesture_handle/gesture_handle.h"

#include "compositors/compositor_platform.h"
#include "config/config_service.h"
#include "config/layout_resolver.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "render/render_context.h"
#include "shell/bar/bar.h"
#include "shell/panel/panel_manager.h"
#include "shell/status_overlay/status_overlay_panel.h"
#include "shell/workspace_occupancy.h"
#include "ui/controls/box.h"
#include "ui/palette.h"
#include "wayland/surface.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <linux/input-event-codes.h>

namespace {

  constexpr Logger kLog("handle");

  // Forgiving hit area around the line (logical px), above and to the sides.
  constexpr std::int32_t kHitPad = 10;
  // Bottom rows left free under the handle, so the dock's and a bar's edge reveal still work underneath it.
  constexpr std::int32_t kEdgeGap = 2;
  constexpr float kFadeMs = 220.0F;
  constexpr float kSettleMs = 260.0F;
  // The line follows a drag a little, like the iOS home indicator, up to this many logical px.
  constexpr float kDragFollow = 0.25F;
  constexpr float kDragFollowMax = 10.0F;
  constexpr auto kPeekRelease = std::chrono::milliseconds(1500);
  constexpr auto kPeekWatch = std::chrono::milliseconds(250);

} // namespace

GestureHandle::GestureHandle() = default;

GestureHandle::~GestureHandle() { destroyInstances(); }

void GestureHandle::initialize(
    CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
    PanelManager* panels, Bar* bar
) {
  // reveal = "bar" peeks through the Bar; reveal = "overlay" opens a panel.
  assert(renderContext != nullptr && bar != nullptr && panels != nullptr && "handle dependencies");
  m_platform = &platform;
  m_config = &config;
  m_renderContext = renderContext;
  m_ipc = ipc;
  m_panels = panels;
  m_bar = bar;
  // The handle surface is created by onOutputChange(), in the shell's canonical stacking order.
  (void)applyConfig();
}

void GestureHandle::reload() {
  if (m_config == nullptr) {
    return;
  }
  if (applyConfig()) {
    destroyInstances();
    syncInstances();
  }
}

bool GestureHandle::applyConfig() {
  const auto& layout = m_config->resolvedLayout();
  const bool hideAtHome =
      layout.mode == LayoutMode::Flow || (layout.handle.has_value() && layout.handle->reveal == HandleReveal::Bar);
  if (layout.handle == m_handleConfig && layout.handleBarName == m_revealBarName && hideAtHome == m_hideAtHome) {
    return false;
  }
  m_handleConfig = layout.handle;
  m_revealBarName = layout.handleBarName;
  m_hideAtHome = hideAtHome;
  return true;
}

void GestureHandle::setSuppressed(bool suppressed) {
  if (suppressed == m_suppressed) {
    return;
  }
  m_suppressed = suppressed;
  destroyInstances();
  syncInstances();
}

void GestureHandle::onOutputChange() {
  if (m_config == nullptr) {
    return;
  }
  destroyInstances();
  syncInstances();
}

void GestureHandle::onWorkspaceChanged() {
  for (auto& instance : m_instances) {
    updateVisibility(*instance, true);
  }
}

void GestureHandle::syncInstances() {
  if (m_suppressed) {
    return;
  }
  if (m_platform == nullptr || !m_handleConfig.has_value()) {
    return;
  }
  for (const auto& out : m_platform->outputs()) {
    if (!out.done || out.connectorName.empty() || out.output == nullptr) {
      continue;
    }
    createInstance(out);
  }
}

void GestureHandle::createInstance(const WaylandOutput& output) {
  const HandleConfig& config = *m_handleConfig;
  const auto width = static_cast<std::uint32_t>(std::max(config.width, 8) + (2 * kHitPad));
  const auto height =
      static_cast<std::uint32_t>(std::max(config.marginBottom - kEdgeGap, 0) + std::max(config.thickness, 1) + kHitPad);

  auto instance = std::make_unique<Instance>();
  instance->outputName = output.name;
  instance->output = output.output;
  instance->recognizer.configure(
      shell::gesture_handle::GestureSettings{.pullUp = config.pullUp, .swipe = config.swipeWorkspaces}
  );

  LayerSurfaceConfig surfaceConfig{
      .nameSpace = "noctalia-handle",
      .layer = LayerShellLayer::Top,
      .anchor = LayerShellAnchor::Bottom,
      .width = width,
      .height = height,
      // On the physical bottom edge, not pushed up by a bar's or the dock's exclusive zone.
      .exclusiveZone = -1,
      .marginBottom = std::min(kEdgeGap, std::max(config.marginBottom, 0)),
      .defaultWidth = width,
      .defaultHeight = height,
  };
  instance->surface = std::make_unique<LayerSurface>(m_platform->wayland(), surfaceConfig);
  instance->surface->setRenderContext(m_renderContext);
  Instance* instancePtr = instance.get();
  instance->surface->setConfigureCallback([instancePtr](std::uint32_t /*width*/, std::uint32_t /*height*/) {
    instancePtr->surface->requestLayout();
  });
  instance->surface->setPrepareFrameCallback([this, instancePtr](bool needsUpdate, bool needsLayout) {
    prepareFrame(*instancePtr, needsUpdate, needsLayout);
  });
  instance->surface->setAnimationManager(&instance->animations);
  if (!instance->surface->initialize(output.output)) {
    kLog.warn("failed to create the handle surface on {}", output.connectorName);
    return;
  }
  // Hidden until the first visibility pass decides otherwise.
  instance->surface->setInputRegion({});
  m_instances.push_back(std::move(instance));
  updateVisibility(*instancePtr, false);
}

void GestureHandle::destroyInstances() {
  for (auto& instance : m_instances) {
    instance->dwellTimer.stop();
    instance->peekReleaseTimer.stop();
    instance->peekWatchTimer.stop();
    if (instance->peeking && m_bar != nullptr) {
      m_bar->setBarPeek(instance->output, m_revealBarName, false);
    }
    instance->animations.cancelAll();
  }
  m_instances.clear();
}

void GestureHandle::prepareFrame(Instance& instance, bool needsUpdate, bool needsLayout) {
  if (m_renderContext == nullptr || instance.surface == nullptr) {
    return;
  }
  const auto width = instance.surface->width();
  const auto height = instance.surface->height();
  if (width == 0 || height == 0) {
    return;
  }
  m_renderContext->makeCurrent(instance.surface->renderTarget());
  const bool rebuild = instance.sceneRoot == nullptr
      || static_cast<std::uint32_t>(std::round(instance.sceneRoot->width())) != width
      || static_cast<std::uint32_t>(std::round(instance.sceneRoot->height())) != height;
  if (rebuild) {
    UiPhaseScope layoutPhase(UiPhase::Layout);
    buildScene(instance, width, height);
  } else if (needsUpdate || needsLayout) {
    UiPhaseScope layoutPhase(UiPhase::Layout);
    applyLineGeometry(instance);
  }
}

void GestureHandle::buildScene(Instance& instance, std::uint32_t width, std::uint32_t height) {
  instance.animations.cancelAll();
  instance.sceneRoot = std::make_unique<Node>();
  instance.sceneRoot->setSize(static_cast<float>(width), static_cast<float>(height));
  auto line = std::make_unique<Box>();
  line->setHitTestVisible(false);
  instance.line = static_cast<Box*>(instance.sceneRoot->addChild(std::move(line)));
  instance.surface->setSceneRoot(instance.sceneRoot.get());
  applyLineGeometry(instance);
}

void GestureHandle::applyLineGeometry(Instance& instance) {
  if (instance.line == nullptr || !m_handleConfig.has_value()) {
    return;
  }
  const HandleConfig& config = *m_handleConfig;
  const auto length = static_cast<float>(std::max(config.width, 8));
  const auto thickness = static_cast<float>(std::max(config.thickness, 1));
  // Tahoe/iOS home indicator: a neutral pill that reads on light and dark wallpapers alike.
  instance.line->setFill(colorSpecFromRole(ColorRole::OnSurface, isResolvedLightTheme() ? 0.55F : 0.7F));
  instance.line->setRadius(thickness * 0.5F);
  instance.line->setSize(length, thickness);
  instance.line->setPosition(
      static_cast<float>(kHitPad) + instance.dragOffsetX, static_cast<float>(kHitPad) + instance.dragOffsetY
  );
  instance.line->setOpacity(instance.visibility);
}

void GestureHandle::updateVisibility(Instance& instance, bool animate) {
  if (m_platform == nullptr || instance.surface == nullptr) {
    return;
  }
  instance.occupied = shell::activeWorkspaceHasWindows(*m_platform, instance.output);
  const bool atHome = shell::showsHome(*m_platform, instance.output);
  const bool shown = !instance.peeking && !(m_hideAtHome && atHome);
  const float target = shown ? 1.0F : 0.0F;

  if (shown) {
    instance.surface->setInputRegion(
        {InputRect{0, 0, static_cast<int>(instance.surface->width()), static_cast<int>(instance.surface->height())}}
    );
  } else {
    instance.dwellTimer.stop();
    instance.surface->setInputRegion({});
  }

  if (std::abs(instance.visibility - target) < 1.0e-3F) {
    return;
  }
  if (instance.visibilityAnim != 0) {
    instance.animations.cancel(instance.visibilityAnim);
    instance.visibilityAnim = 0;
  }
  Instance* instancePtr = &instance;
  if (!animate) {
    instance.visibility = target;
    applyLineGeometry(instance);
    instance.surface->requestRedraw();
    return;
  }
  instance.visibilityAnim = instance.animations.animate(
      instance.visibility, target, kFadeMs, Easing::EaseOutCubic,
      [instancePtr](float value) {
        instancePtr->visibility = value;
        if (instancePtr->line != nullptr) {
          instancePtr->line->setOpacity(value);
        }
      },
      [instancePtr]() { instancePtr->visibilityAnim = 0; }, instance.line
  );
  instance.surface->requestRedraw();
}

bool GestureHandle::onPointerEvent(const PointerEvent& event) {
  Instance* instance = instanceForSurface(event.surface);
  if (instance == nullptr || !m_handleConfig.has_value()) {
    return false;
  }
  using shell::gesture_handle::Outcome;
  const auto x = static_cast<float>(event.sx);
  const auto y = static_cast<float>(event.sy);
  switch (event.type) {
  case PointerEvent::Type::Enter: {
    instance->pointerInside = true;
    instance->recognizer.enter();
    const auto delay = std::chrono::milliseconds(std::max(m_handleConfig->hoverDelayMs, 0));
    instance->dwellTimer.start(delay, [this, instance]() {
      handleOutcome(*instance, instance->recognizer.dwellElapsed());
    });
    return true;
  }
  case PointerEvent::Type::Leave:
    instance->pointerInside = false;
    instance->dwellTimer.stop();
    instance->recognizer.leave();
    return true;
  case PointerEvent::Type::Motion: {
    const auto phase = instance->recognizer.phase();
    if (phase == shell::gesture_handle::Phase::Hovering || phase == shell::gesture_handle::Phase::Idle) {
      return true;
    }
    const Outcome outcome = instance->recognizer.motion(x, y);
    const float dx = x - instance->recognizer.pressX();
    const float dy = y - instance->recognizer.pressY();
    instance->dragOffsetX = std::clamp(dx * kDragFollow, -kDragFollowMax, kDragFollowMax);
    instance->dragOffsetY = std::clamp(dy * kDragFollow, -kDragFollowMax, 0.0F);
    applyLineGeometry(*instance);
    instance->surface->requestRedraw();
    handleOutcome(*instance, outcome);
    return true;
  }
  case PointerEvent::Type::Button:
    if (event.button != BTN_LEFT) {
      return true;
    }
    if (event.pressed) {
      instance->dwellTimer.stop();
      instance->recognizer.press(x, y);
    } else {
      const Outcome outcome = instance->recognizer.release(x, y);
      settleLine(*instance);
      handleOutcome(*instance, outcome);
    }
    return true;
  case PointerEvent::Type::Axis:
    return true;
  }
  return false;
}

void GestureHandle::handleOutcome(Instance& instance, shell::gesture_handle::Outcome outcome) {
  using shell::gesture_handle::Outcome;
  switch (outcome) {
  case Outcome::None:
    return;
  case Outcome::Reveal:
    reveal(instance);
    return;
  case Outcome::SwipePrevious:
    switchWorkspace(instance, -1);
    return;
  case Outcome::SwipeNext:
    switchWorkspace(instance, 1);
    return;
  }
}

void GestureHandle::reveal(Instance& instance) {
  if (!m_handleConfig.has_value()) {
    return;
  }
  if (m_handleConfig->reveal == HandleReveal::Overlay) {
    if (m_panels != nullptr && !m_panels->isOpenPanel(StatusOverlayPanel::kPanelId)) {
      m_panels->openPanel(std::string(StatusOverlayPanel::kPanelId), PanelOpenRequest{.output = instance.output});
    }
    return;
  }
  if (m_bar == nullptr || m_revealBarName.empty()) {
    return;
  }
  m_bar->setBarPeek(instance.output, m_revealBarName, true);
  instance.peeking = true;
  updateVisibility(instance, true);

  Instance* instancePtr = &instance;
  // The peek holds the bar until the pointer has used it; if the pointer never gets there, let it go.
  instance.peekReleaseTimer.start(kPeekRelease, [this, instancePtr]() {
    if (m_bar != nullptr) {
      m_bar->setBarPeek(instancePtr->output, m_revealBarName, false);
    }
  });
  instance.peekWatchTimer.startRepeating(kPeekWatch, [this, instancePtr]() {
    if (m_bar != nullptr && m_bar->isBarShown(instancePtr->output, m_revealBarName)) {
      return;
    }
    instancePtr->peekWatchTimer.stop();
    instancePtr->peekReleaseTimer.stop();
    instancePtr->peeking = false;
    updateVisibility(*instancePtr, true);
  });
}

void GestureHandle::switchWorkspace(Instance& instance, int direction) {
  if (m_platform == nullptr) {
    return;
  }
  const auto workspaces = m_platform->workspaces(instance.output);
  const auto active = std::ranges::find_if(workspaces, [](const Workspace& ws) { return ws.active; });
  if (active == workspaces.end()) {
    return;
  }
  const auto index = static_cast<std::ptrdiff_t>(active - workspaces.begin()) + direction;
  if (index < 0 || index >= static_cast<std::ptrdiff_t>(workspaces.size())) {
    return;
  }
  m_platform->activateWorkspace(instance.output, workspaces[static_cast<std::size_t>(index)]);
}

void GestureHandle::settleLine(Instance& instance) {
  if (instance.dragSettleAnim != 0) {
    instance.animations.cancel(instance.dragSettleAnim);
    instance.dragSettleAnim = 0;
  }
  const float startX = instance.dragOffsetX;
  const float startY = instance.dragOffsetY;
  if (std::abs(startX) < 0.01F && std::abs(startY) < 0.01F) {
    return;
  }
  Instance* instancePtr = &instance;
  instance.dragSettleAnim = instance.animations.animate(
      1.0F, 0.0F, kSettleMs, Easing::EaseOutBack,
      [this, instancePtr, startX, startY](float t) {
        instancePtr->dragOffsetX = startX * t;
        instancePtr->dragOffsetY = startY * t;
        applyLineGeometry(*instancePtr);
      },
      [instancePtr]() { instancePtr->dragSettleAnim = 0; }, instance.line
  );
  instance.surface->requestRedraw();
}

void GestureHandle::requestLayout() {
  for (auto& instance : m_instances) {
    if (instance->surface != nullptr) {
      instance->surface->requestLayout();
    }
  }
}

void GestureHandle::requestRedraw() {
  for (auto& instance : m_instances) {
    if (instance->surface != nullptr) {
      applyLineGeometry(*instance);
      instance->surface->requestRedraw();
    }
  }
}

std::vector<wl_surface*> GestureHandle::surfaces() const {
  std::vector<wl_surface*> out;
  for (const auto& instance : m_instances) {
    if (instance->surface != nullptr && instance->surface->wlSurface() != nullptr) {
      out.push_back(instance->surface->wlSurface());
    }
  }
  return out;
}

std::vector<InputRect> GestureHandle::surfaceRectsForOutput(wl_output* output) const {
  std::vector<InputRect> out;
  if (m_platform == nullptr) {
    return out;
  }
  const auto& outputs = m_platform->outputs();
  for (const auto& instance : m_instances) {
    if (instance->output != output || instance->surface == nullptr || instance->visibility <= 0.0F) {
      continue;
    }
    const auto it = std::ranges::find(outputs, instance->outputName, &WaylandOutput::name);
    if (it == outputs.end()) {
      continue;
    }
    const int outputW = it->logicalWidth > 0 ? it->logicalWidth : it->width;
    const int outputH = it->logicalHeight > 0 ? it->logicalHeight : it->height;
    const auto w = static_cast<int>(instance->surface->width());
    const auto h = static_cast<int>(instance->surface->height());
    out.push_back(InputRect{(outputW - w) / 2, outputH - kEdgeGap - h, w, h});
  }
  return out;
}

GestureHandle::Instance* GestureHandle::instanceForSurface(wl_surface* surface) const noexcept {
  if (surface == nullptr) {
    return nullptr;
  }
  for (const auto& instance : m_instances) {
    if (instance->surface != nullptr && instance->surface->wlSurface() == surface) {
      return instance.get();
    }
  }
  return nullptr;
}
