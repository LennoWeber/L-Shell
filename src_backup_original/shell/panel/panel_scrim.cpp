#include "shell/panel/panel_scrim.h"

#include "config/config_service.h"
#include "core/log.h"
#include "render/animation/animation.h"
#include "render/render_context.h"
#include "render/scene/rect_node.h"
#include "ui/builders.h"
#include "ui/controls/box.h"
#include "ui/style.h"
#include "wayland/layer_surface.h"
#include "wayland/wayland_connection.h"

#include <algorithm>
#include <cmath>

namespace {

  constexpr Logger kLog("panel-scrim");

  // Above the blur rule's ignore_alpha cut-off (0.4), so the compositor really blurs behind the veil.
  constexpr float kScrimAlpha = 0.42F;
  constexpr Color kScrimColor{0.0F, 0.0F, 0.0F, kScrimAlpha};

} // namespace

void PanelScrim::initialize(WaylandConnection& wayland, ConfigService* config, RenderContext* renderContext) {
  m_wayland = &wayland;
  m_config = config;
  m_renderContext = renderContext;
  syncInstances();
}

void PanelScrim::onOutputChange() {
  syncInstances();
}

void PanelScrim::syncInstances() {
  if (m_wayland == nullptr) {
    return;
  }
  const auto& outputs = m_wayland->outputs();
  std::unordered_map<std::uint32_t, WaylandOutput> current;
  for (const auto& output : outputs) {
    current.emplace(output.name, output);
  }

  m_surfaceMap.clear();
  for (auto it = m_instances.begin(); it != m_instances.end();) {
    if (current.contains((*it)->outputName)) {
      if ((*it)->surface != nullptr) {
        m_surfaceMap.emplace((*it)->surface->wlSurface(), it->get());
      }
      ++it;
    } else {
      it = m_instances.erase(it);
    }
  }

  for (const auto& [name, output] : current) {
    if (std::ranges::none_of(m_instances, [&name](const auto& inst) { return inst->outputName == name; })) {
      createInstance(output);
    }
  }
}

void PanelScrim::createInstance(const WaylandOutput& output) {
  if (m_renderContext == nullptr) {
    return;
  }

  auto instance = std::make_unique<Instance>();
  instance->outputName = output.name;
  instance->output = output.output;

  // The veil rides the "noctalia-panel" namespace so the compositor's blur rule catches it; the
  // layer stays below the floating panels, which live on the overlay layer.
  const LayerSurfaceConfig surfaceConfig{
      .nameSpace = "noctalia-panel",
      .layer = LayerShellLayer::Top,
      .anchor = LayerShellAnchor::Top | LayerShellAnchor::Bottom | LayerShellAnchor::Left
          | LayerShellAnchor::Right,
      .width = 0,
      .height = 0,
      .exclusiveZone = -1,
      .defaultWidth = 0,
      .defaultHeight = 0,
      .prewarmBlur = true,
  };
  instance->surface = std::make_unique<LayerSurface>(*m_wayland, surfaceConfig);
  instance->surface->setRenderContext(m_renderContext);
  if (!instance->surface->initialize(output.output)) {
    kLog.warn("failed to create the panel scrim on {}", output.connectorName);
    return;
  }

  instance->root = ui::node({});
  instance->root->setAnimationManager(&instance->animations);
  instance->fill = static_cast<Box*>(instance->root->addChild(ui::box()));
  instance->fill->setFill(fixedColorSpec(kScrimColor));
  instance->root->setOpacity(0.0F);
  instance->surface->setSceneRoot(instance->root.get());

  instance->surface->setPrepareFrameCallback([inst = instance.get()](bool /*needsUpdate*/, bool needsLayout) {
    if (needsLayout) {
      const auto w = static_cast<float>(inst->surface->width());
      const auto h = static_cast<float>(inst->surface->height());
      inst->fill->setSize(w, h);
    }
  });
  instance->surface->setAnimationManager(&instance->animations);

  // Hidden until a panel asks for it: no compositor work while no modal panel is open.
  instance->surface->setInputRegion({});
  instance->surface->setKeyboardInteractivity(LayerShellKeyboard::None);
  instance->surface->requestLayout();

  m_surfaceMap.emplace(instance->surface->wlSurface(), instance.get());
  m_instances.push_back(std::move(instance));
}


void PanelScrim::animateInstance(Instance& instance, bool show) {
  if (show) {
    // The veil is part of the panel's opening, not its own animation: it must be in place the
    // moment the menu draws its first frame, so the mode is instant and only the way back fades.
    instance.animations.cancelForOwner(instance.root.get());
    instance.visibility = 1.0F;
    if (instance.root != nullptr) {
      instance.root->setOpacity(1.0F);
    }
    if (instance.surface != nullptr) {
      instance.surface->requestLayout();
      instance.surface->requestRedraw();
    }
    return;
  }
  const float from = instance.visibility;
  const float to = 0.0F;
  instance.animations.cancelForOwner(instance.root.get());
  instance.animations.animate(
      from, to, Style::animNormal, Easing::EaseInQuad,
      [inst = &instance](float v) {
        inst->visibility = v;
        if (inst->root != nullptr) {
          inst->root->setOpacity(v);
        }
      },
      [inst = &instance]() {
        if (inst->surface != nullptr) {
          inst->surface->requestRedraw();
        }
      },
      instance.root.get()
  );
  if (instance.surface != nullptr) {
    instance.surface->requestRedraw();
  }
}

void PanelScrim::show(wl_output* output) {
  (void)output; // one mode for the whole desktop: the veil covers every output
  m_visible = true;
  for (auto& instance : m_instances) {
    if (instance == nullptr || instance->surface == nullptr) {
      continue;
    }
    // One screen enters the mode; the veil covers every output so the desktop reads as one state.
    instance->surface->setInputRegion(
        {InputRect{0, 0, static_cast<int>(instance->surface->width()), static_cast<int>(instance->surface->height())}}
    );
    animateInstance(*instance, true);
  }
}

void PanelScrim::hide() {
  m_visible = false;
  for (auto& instance : m_instances) {
    if (instance == nullptr || instance->surface == nullptr) {
      continue;
    }
    instance->surface->setInputRegion({});
    animateInstance(*instance, false);
  }
}

bool PanelScrim::onPointerEvent(const PointerEvent& event) {
  if (!m_visible || event.surface == nullptr) {
    return false;
  }
  const auto it = m_surfaceMap.find(event.surface);
  if (it == m_surfaceMap.end()) {
    return false;
  }
  if (event.type == PointerEvent::Type::Button) {
    if (m_onClose) {
      m_onClose();
    }
    return true;
  }
  return true;
}
