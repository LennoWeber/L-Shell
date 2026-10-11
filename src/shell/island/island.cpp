#include "shell/island/island.h"

#include "compositors/compositor_platform.h"
#include "config/config_service.h"
#include "core/deferred_call.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "dbus/mpris/mpris_service.h"
#include "dbus/upower/upower_service.h"
#include "i18n/i18n.h"
#include "notification/notification.h"
#include "notification/notification_manager.h"
#include "render/render_context.h"
#include "render/scene/input_area.h"
#include "render/scene/node.h"
#include "shell/bar/widget_action_dispatcher.h"
#include "shell/osd/osd_overlay.h"
#include "shell/panel/panel_manager.h"
#include "shell/workspace_occupancy.h"
#include "time/time_format.h"
#include "ui/builders.h"
#include "ui/controls/box.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"
#include "ui/controls/progress_bar.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "wayland/surface.h"
#include "wayland/wayland_connection.h"
#include "wayland/wayland_seat.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <format>
#include <linux/input-event-codes.h>

namespace {

  constexpr Logger kLog("island");

  using shell::island::Activity;
  using shell::island::ActivityKind;
  using shell::island::Presentation;

  // Room around the capsule for its drop shadow (logical px, unscaled).
  constexpr float kBleed = 20.0F;
  // Extra width the compact state gets for its ambient indicator.
  constexpr float kCompactExtra = 44.0F;
  constexpr float kExpandMs = 380.0F;
  constexpr float kCollapseMs = 300.0F;
  constexpr float kFadeMs = 220.0F;
  // Hidden islands slide this far up while fading.
  constexpr float kHideLift = 10.0F;
  constexpr float kLowBattery = 20.0F;
  constexpr float kCriticalBattery = 10.0F;
  // The menu-bar clock: weekday and date, then the time in the shell's own format (12 or 24 hours).
  constexpr const char* kClockDateFormat = "%a %-d %b";

  [[nodiscard]] float lerp(float a, float b, float t) noexcept { return a + ((b - a) * t); }

  [[nodiscard]] ActivityKind activityKindForOsd(OsdKind kind) noexcept {
    switch (kind) {
    case OsdKind::Volume:
    case OsdKind::Microphone:
      return ActivityKind::Volume;
    case OsdKind::Brightness:
    case OsdKind::KeyboardBacklight:
      return ActivityKind::Brightness;
    case OsdKind::Media:
      return ActivityKind::Media;
    default:
      return ActivityKind::System;
    }
  }

  [[nodiscard]] std::string osdTitle(OsdKind kind) {
    switch (kind) {
    case OsdKind::Volume:
      return i18n::tr("island.volume");
    case OsdKind::Microphone:
      return i18n::tr("island.microphone");
    case OsdKind::Brightness:
      return i18n::tr("island.brightness");
    case OsdKind::KeyboardBacklight:
      return i18n::tr("island.keyboard-backlight");
    default:
      return {};
    }
  }

  [[nodiscard]] std::string percentText(double percent) {
    return std::format("{}%", static_cast<int>(std::lround(std::clamp(percent, 0.0, 100.0))));
  }

  [[nodiscard]] std::string timeText(const ConfigService* config) {
    return formatLocalTime(config != nullptr ? config->config().shell.timeFormat.c_str() : "%H:%M");
  }

  [[nodiscard]] std::string clockText(const ConfigService* config) {
    return formatLocalTime(kClockDateFormat) + "  " + timeText(config);
  }

} // namespace

Island::Island() = default;

Island::~Island() {
  if (m_notifications != nullptr && m_notificationToken >= 0) {
    m_notifications->removeEventCallback(m_notificationToken);
  }
  destroyInstances();
}

void Island::initialize(const IslandServices& services) {
  assert(services.renderContext != nullptr && "the island draws itself");
  m_platform = &services.platform;
  m_config = &services.config;
  m_renderContext = services.renderContext;
  m_ipc = services.ipc;
  m_panels = services.panels;
  m_mpris = services.mpris;
  m_notifications = services.notifications;
  m_upower = services.upower;
  if (m_notifications != nullptr) {
    m_notificationToken =
        m_notifications->addEventCallback([this](const Notification& notification, NotificationEvent event) {
          onNotificationEvent(notification, event);
        });
  }
  if (m_upower != nullptr) {
    const auto& state = m_upower->state();
    if (state.isPresent) {
      m_lastCharging = batteryStatePlugged(state.state).value_or(!state.onBattery);
      m_lastBatteryPercent = static_cast<float>(state.percentage);
    }
  }
  // Surfaces are created by onOutputChange(), in the shell's canonical stacking order.
  (void)applyConfig();
  onMediaChanged();
}

void Island::reload() {
  if (m_config == nullptr) {
    return;
  }
  if (applyConfig()) {
    destroyInstances();
    syncInstances();
  }
}

bool Island::applyConfig() {
  const auto& layout = m_config->resolvedLayout();
  const bool changed = layout.island != m_islandConfig || layout.islandVisibility != m_visibility;
  m_islandConfig = layout.island;
  m_visibility = layout.islandVisibility;
  if (m_islandConfig.has_value()) {
    m_activities.configure(*m_islandConfig);
    auto parsed = noctalia::bar::parseWidgetAction(m_islandConfig->clickAction);
    if (parsed.has_value()) {
      m_clickAction = *parsed;
    } else {
      kLog.warn("layout island click_action \"{}\": {}", m_islandConfig->clickAction, parsed.error());
      m_clickAction = {};
    }
  }
  return changed;
}

void Island::setSuppressed(bool suppressed) {
  if (suppressed == m_suppressed) {
    return;
  }
  m_suppressed = suppressed;
  destroyInstances();
  syncInstances();
}

void Island::onOutputChange() {
  if (m_config == nullptr) {
    return;
  }
  destroyInstances();
  syncInstances();
}

void Island::onWorkspaceChanged() {
  // Window-title changes arrive here too, in bursts; one visibility pass per burst is enough.
  if (m_visibilityPassQueued) {
    return;
  }
  m_visibilityPassQueued = true;
  DeferredCall::callLater([this]() {
    m_visibilityPassQueued = false;
    for (auto& instance : m_instances) {
      animateVisibility(*instance);
    }
  });
}

void Island::onSecondTick() {
  // The island shows minutes: repaint when the minute turns, not every second.
  std::string clock = clockText(m_config);
  if (clock == m_clockText) {
    return;
  }
  m_clockText = std::move(clock);
  for (auto& instance : m_instances) {
    if (instance->surface != nullptr && instance->visibility > 0.0F) {
      instance->surface->requestUpdate();
    }
  }
}

bool Island::isActive() const noexcept { return m_islandConfig.has_value() && !m_instances.empty(); }

bool Island::shownOn(const Instance& instance) const {
  if (m_visibility == noctalia::config::IslandVisibility::Always || m_platform == nullptr) {
    return true;
  }
  return !shell::showsHome(*m_platform, instance.output);
}

void Island::syncInstances() {
  if (m_suppressed) {
    return;
  }
  if (m_platform == nullptr || !m_islandConfig.has_value()) {
    return;
  }
  for (const auto& out : m_platform->outputs()) {
    if (!out.done || out.connectorName.empty() || out.output == nullptr) {
      continue;
    }
    createInstance(out);
  }
}

void Island::createInstance(const WaylandOutput& output) {
  const IslandConfig& config = *m_islandConfig;
  const float scale = std::max(0.1F, m_config->config().accessibility.uiScale);
  const auto width =
      static_cast<std::uint32_t>(std::ceil((static_cast<float>(config.expandedWidth) + (2.0F * kBleed)) * scale));
  const auto height = static_cast<std::uint32_t>(
      std::ceil((static_cast<float>(config.marginTop + config.expandedHeight) + kBleed) * scale)
  );
  const std::int32_t reserved = config.reserveSpace
      ? static_cast<std::int32_t>(std::ceil(static_cast<float>(config.marginTop + config.height) * scale))
      : 0;

  auto instance = std::make_unique<Instance>();
  instance->outputName = output.name;
  instance->output = output.output;
  instance->uiScale = scale;
  instance->visibility = 0.0F;

  LayerSurfaceConfig surfaceConfig{
      .nameSpace = "noctalia-island",
      .layer = LayerShellLayer::Top,
      .anchor = LayerShellAnchor::Top,
      .width = width,
      .height = height,
      // Without its own zone the island ignores the others, so a bar's zone cannot push it down.
      .exclusiveZone = reserved > 0 ? reserved : -1,
      .defaultWidth = width,
      .defaultHeight = height,
      .prewarmBlur = true,
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
  instance->surface->setFrameTickCallback([this, instancePtr](float /*deltaMs*/) {
    if (instancePtr->animations.hasActive()) {
      layoutCapsule(*instancePtr);
    }
  });
  instance->surface->setAnimationManager(&instance->animations);
  if (!instance->surface->initialize(output.output)) {
    kLog.warn("failed to create the island surface on {}", output.connectorName);
    return;
  }
  instance->surface->setInputRegion({});
  m_instances.push_back(std::move(instance));
  animateVisibility(*instancePtr);
}

void Island::destroyInstances() {
  for (auto& instance : m_instances) {
    instance->animations.cancelAll();
  }
  m_instances.clear();
  // A capsule destroyed under the pointer never sends its leave: let the tracker's hold go with it.
  m_activities.setHovered(false);
  scheduleActivityTimer();
}

void Island::prepareFrame(Instance& instance, bool needsUpdate, bool needsLayout) {
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
  UiPhaseScope layoutPhase(UiPhase::Layout);
  if (rebuild) {
    buildScene(instance, width, height);
  }
  if (rebuild || needsUpdate || needsLayout) {
    updateContent(instance);
    layoutCapsule(instance);
  }
  if (rebuild) {
    // Building the scene cancelled any fade that started before the first configure; start it again.
    instance.shownTarget.reset();
    animateVisibility(instance);
  }
}

void Island::buildScene(Instance& instance, std::uint32_t width, std::uint32_t height) {
  instance.animations.cancelAll();
  instance.expansionAnim = 0;
  instance.visibilityAnim = 0;
  const float s = instance.uiScale;
  const bool black = m_islandConfig->material == IslandMaterial::Black;
  const ColorSpec text = black ? fixedColorSpec(rgba(1.0F, 1.0F, 1.0F, 1.0F)) : colorSpecFromRole(ColorRole::OnSurface);
  const ColorSpec secondary =
      black ? fixedColorSpec(rgba(1.0F, 1.0F, 1.0F, 0.6F)) : colorSpecFromRole(ColorRole::OnSurfaceVariant);

  instance.sceneRoot = ui::node({.width = static_cast<float>(width), .height = static_cast<float>(height)});
  instance.surface->setSceneRoot(instance.sceneRoot.get());

  auto capsule = ui::node({.clipChildren = false});
  instance.capsule = instance.sceneRoot->addChild(std::move(capsule));

  shell::glass::GlassStyle style;
  style.material = black ? shell::glass::Material::Black : shell::glass::Material::Glass;
  instance.glass = shell::glass::buildGlass(*instance.capsule, style);

  instance.clock = static_cast<Label*>(instance.capsule->addChild(
      ui::label({
          .text = clockText(m_config),
          .fontSize = Style::fontSizeBody * s,
          .fontWeight = FontWeight::Medium,
          .color = text,
          .maxLines = 1,
      })
  ));

  instance.ambientIndicator = instance.capsule->addChild(
      ui::glyph({
          .glyph = std::string("music"),
          .glyphSize = 14.0F * s,
          .color = colorSpecFromRole(ColorRole::Primary),
      })
  );

  auto expanded = ui::node({});
  instance.expandedContent = instance.capsule->addChild(std::move(expanded));
  instance.activityGlyph = static_cast<Glyph*>(instance.expandedContent->addChild(
      ui::glyph({
          .glyph = std::string("clock"),
          .glyphSize = 26.0F * s,
          .color = text,
      })
  ));
  instance.activityTitle = static_cast<Label*>(instance.expandedContent->addChild(
      ui::label({
          .fontSize = Style::fontSizeBody * s,
          .fontWeight = FontWeight::SemiBold,
          .color = text,
          .maxLines = 1,
      })
  ));
  instance.activitySubtitle = static_cast<Label*>(instance.expandedContent->addChild(
      ui::label({
          .fontSize = Style::fontSizeCaption * s,
          .color = secondary,
          .maxLines = 1,
      })
  ));
  instance.activityLevel = static_cast<ProgressBar*>(instance.expandedContent->addChild(
      ui::progressBar({
          .fill = colorSpecFromRole(ColorRole::Primary),
          .track =
              black ? fixedColorSpec(rgba(1.0F, 1.0F, 1.0F, 0.18F)) : colorSpecFromRole(ColorRole::OnSurface, 0.14F),
          .radius = 3.0F * s,
      })
  ));

  auto input = ui::inputArea({.out = &instance.inputArea});
  Instance* instancePtr = &instance;
  instance.inputArea->setOnEnter([this, instancePtr](const InputArea::PointerData&) {
    instancePtr->pointerInside = true;
    m_activities.setHovered(true);
    refreshAll();
  });
  instance.inputArea->setOnLeave([this, instancePtr]() {
    instancePtr->pointerInside = false;
    const bool anyHovered = std::ranges::any_of(m_instances, [](const auto& other) { return other->pointerInside; });
    m_activities.setHovered(anyHovered);
    scheduleActivityTimer();
    refreshAll();
  });
  instance.inputArea->setOnClick([this, instancePtr](const InputArea::PointerData& data) {
    if (data.button == BTN_LEFT) {
      activateClick(*instancePtr);
    }
  });
  instance.capsule->addChild(std::move(input));
  instance.inputDispatcher.setSceneRoot(instance.sceneRoot.get());

  const Presentation presentation = m_activities.presentation();
  instance.expansion = presentation == Presentation::Expanded ? 1.0F : 0.0F;
}

float Island::capsuleWidth(const Instance& instance) const noexcept {
  const IslandConfig& config = *m_islandConfig;
  const float s = instance.uiScale;
  float collapsed = static_cast<float>(config.collapsedWidth);
  if (m_activities.state().ambient.has_value()) {
    collapsed += kCompactExtra;
  }
  return lerp(collapsed, static_cast<float>(config.expandedWidth), instance.expansion) * s;
}

float Island::capsuleHeight(const Instance& instance) const noexcept {
  const IslandConfig& config = *m_islandConfig;
  return lerp(static_cast<float>(config.height), static_cast<float>(config.expandedHeight), instance.expansion)
      * instance.uiScale;
}

void Island::updateContent(Instance& instance) {
  if (instance.clock == nullptr || !m_islandConfig.has_value()) {
    return;
  }
  instance.clock->setText(clockText(m_config));

  const auto& state = m_activities.state();
  const Activity* shown = state.transient.has_value() ? &*state.transient
      : state.ambient.has_value()                     ? &*state.ambient
                                                      : nullptr;
  if (shown != nullptr) {
    instance.activityGlyph->setGlyph(shown->glyph.empty() ? std::string_view("info") : std::string_view(shown->glyph));
    instance.activityTitle->setText(shown->title);
    instance.activitySubtitle->setText(shown->subtitle);
    const bool hasLevel = shown->level.has_value();
    instance.activityLevel->setVisible(hasLevel);
    if (hasLevel) {
      instance.activityLevel->setProgress(std::clamp(*shown->level, 0.0F, 1.0F));
      // Muted or off: the level stays neutral; the accent marks only live values.
      instance.activityLevel->setFill(
          shown->inactive ? colorSpecFromRole(ColorRole::OnSurfaceVariant) : colorSpecFromRole(ColorRole::Primary)
      );
    }
    return;
  }
  // Hovered with nothing going on: the date, the time and the battery, like a glance at the lock screen.
  instance.activityGlyph->setGlyph("calendar");
  instance.activityTitle->setText(formatLocalTime("%A, %-d %B"));
  std::string subtitle = timeText(m_config);
  if (m_upower != nullptr && m_upower->state().isPresent) {
    subtitle += "  ·  " + percentText(m_upower->state().percentage);
  }
  instance.activitySubtitle->setText(subtitle);
  instance.activityLevel->setVisible(false);
}

void Island::layoutCapsule(Instance& instance) {
  if (instance.sceneRoot == nullptr || instance.capsule == nullptr || !m_islandConfig.has_value()) {
    return;
  }
  Renderer& renderer = instance.surface->renderTarget().renderer();
  const IslandConfig& config = *m_islandConfig;
  const float s = instance.uiScale;
  const float rootW = instance.sceneRoot->width();
  const float w = capsuleWidth(instance);
  const float h = capsuleHeight(instance);
  const float x = std::round((rootW - w) * 0.5F);
  const float y = std::round((static_cast<float>(config.marginTop) - (kHideLift * (1.0F - instance.visibility))) * s);

  instance.capsule->setPosition(x, y);
  instance.capsule->setSize(w, h);
  instance.capsule->setOpacity(instance.visibility);

  // Pill when collapsed; a rounder-than-panel rectangle when expanded, like the Dynamic Island.
  const float radius = std::min(h * 0.5F, lerp(h * 0.5F, 26.0F * s, instance.expansion));
  shell::glass::GlassStyle style;
  style.material =
      config.material == IslandMaterial::Black ? shell::glass::Material::Black : shell::glass::Material::Glass;
  style.radius = Radii(radius);
  shell::glass::applyGlass(instance.glass, style, isResolvedLightTheme(), s);
  shell::glass::layoutGlass(instance.glass, 0.0F, 0.0F, w, h, s);

  const float e = instance.expansion;
  const float collapsedOpacity = std::clamp(1.0F - (e * 2.0F), 0.0F, 1.0F);
  const float expandedOpacity = std::clamp((e * 2.0F) - 1.0F, 0.0F, 1.0F);

  instance.clock->measure(renderer);
  const float collapsedH = static_cast<float>(config.height) * s;
  instance.clock->setOpacity(collapsedOpacity);
  instance.clock->setPosition(
      std::round((w - instance.clock->width()) * 0.5F), std::round((collapsedH - instance.clock->height()) * 0.5F)
  );

  const bool ambient = m_activities.state().ambient.has_value();
  instance.ambientIndicator->setVisible(ambient);
  if (ambient) {
    // Glyphs have no size until measured; centering against an unmeasured one puts it at the top.
    static_cast<Glyph*>(instance.ambientIndicator)->measure(renderer);
    instance.ambientIndicator->setOpacity(collapsedOpacity);
    instance.ambientIndicator->setPosition(
        std::round(14.0F * s), std::round((collapsedH - instance.ambientIndicator->height()) * 0.5F)
    );
  }

  // Expanded content: glyph | title over subtitle (over the level bar).
  const float pad = 18.0F * s;
  instance.expandedContent->setOpacity(expandedOpacity);
  instance.expandedContent->setVisible(expandedOpacity > 0.0F);
  instance.expandedContent->setPosition(0.0F, 0.0F);
  instance.expandedContent->setSize(w, h);
  if (expandedOpacity > 0.0F) {
    const float glyphSize = 26.0F * s;
    const float textX = pad + glyphSize + (14.0F * s);
    const float textW = std::max(0.0F, w - textX - pad);
    instance.activityGlyph->measure(renderer);
    instance.activityGlyph->setPosition(pad, std::round((h - instance.activityGlyph->height()) * 0.5F));
    instance.activityTitle->setMaxWidth(textW);
    instance.activitySubtitle->setMaxWidth(textW);
    instance.activityTitle->measure(renderer);
    instance.activitySubtitle->measure(renderer);
    const bool level = instance.activityLevel->visible();
    const float levelH = 6.0F * s;
    const float gap = 4.0F * s;
    const float block = instance.activityTitle->height()
        + gap
        + instance.activitySubtitle->height()
        + (level ? gap + gap + levelH : 0.0F);
    float ty = std::round((h - block) * 0.5F);
    instance.activityTitle->setPosition(textX, ty);
    ty += instance.activityTitle->height() + gap;
    instance.activitySubtitle->setPosition(textX, ty);
    if (level) {
      ty += instance.activitySubtitle->height() + gap + gap;
      instance.activityLevel->setPosition(textX, ty);
      instance.activityLevel->setSize(textW, levelH);
    }
  }

  if (instance.inputArea != nullptr) {
    instance.inputArea->setPosition(0.0F, 0.0F);
    instance.inputArea->setSize(w, h);
  }
  syncInputRegion(instance);
  syncBlurRegion(instance);
}

void Island::syncInputRegion(Instance& instance) {
  if (instance.surface == nullptr || instance.capsule == nullptr) {
    return;
  }
  if (instance.visibility <= 0.01F || !shownOn(instance)) {
    instance.surface->setInputRegion({});
    return;
  }
  // While hovered the region covers the full expanded rectangle, so the growing capsule never slips out from
  // under the pointer mid-animation.
  const IslandConfig& config = *m_islandConfig;
  const float s = instance.uiScale;
  const bool hoverExpands = instance.pointerInside && config.expandOnHover;
  const float w = hoverExpands ? static_cast<float>(config.expandedWidth) * s : instance.capsule->width();
  const float h = hoverExpands ? static_cast<float>(config.expandedHeight) * s : instance.capsule->height();
  const float x = (instance.sceneRoot->width() - w) * 0.5F;
  instance.surface->setInputRegion({InputRect{
      static_cast<int>(std::floor(x)), static_cast<int>(std::floor(instance.capsule->y())),
      static_cast<int>(std::ceil(w)), static_cast<int>(std::ceil(h))
  }});
}

void Island::syncBlurRegion(Instance& instance) {
  if (instance.surface == nullptr || instance.capsule == nullptr) {
    return;
  }
  if (m_islandConfig->material == IslandMaterial::Black || instance.visibility <= 0.01F) {
    instance.surface->clearBlurRegion();
    return;
  }
  const int x = static_cast<int>(std::floor(instance.capsule->x()));
  const int y = static_cast<int>(std::floor(instance.capsule->y()));
  const int w = std::max(1, static_cast<int>(std::ceil(instance.capsule->width())));
  const int h = std::max(1, static_cast<int>(std::ceil(instance.capsule->height())));
  const float radius = std::min(
      static_cast<float>(h) * 0.5F, lerp(static_cast<float>(h) * 0.5F, 26.0F * instance.uiScale, instance.expansion)
  );
  instance.surface->setBlurRegion(Surface::tessellateRoundedRect(x, y, w, h, radius));
}

void Island::animatePresentation(Instance& instance) {
  const Presentation presentation = m_activities.presentation();
  const float target = presentation == Presentation::Expanded ? 1.0F : 0.0F;
  if (instance.expansionAnim != 0) {
    instance.animations.cancel(instance.expansionAnim);
    instance.expansionAnim = 0;
  }
  if (instance.surface == nullptr) {
    return;
  }
  instance.surface->requestUpdate();
  if (std::abs(instance.expansion - target) < 1.0e-3F) {
    instance.surface->requestLayout();
    return;
  }
  Instance* instancePtr = &instance;
  const bool expanding = target > instance.expansion;
  // Expanding overshoots a touch, like the island's spring; collapsing settles without a bounce.
  instance.expansionAnim = instance.animations.animate(
      instance.expansion, target, expanding ? kExpandMs : kCollapseMs,
      expanding ? Easing::EaseOutBack : Easing::EaseOutCubic,
      [instancePtr](float value) { instancePtr->expansion = std::max(value, 0.0F); },
      [this, instancePtr]() {
        instancePtr->expansionAnim = 0;
        layoutCapsule(*instancePtr);
      },
      instance.capsule
  );
  instance.surface->requestRedraw();
}

void Island::animateVisibility(Instance& instance) {
  if (instance.surface == nullptr) {
    return;
  }
  const bool shown = shownOn(instance);
  if (instance.shownTarget == shown) {
    return; // already there, or on the way
  }
  instance.shownTarget = shown;
  const float target = shown ? 1.0F : 0.0F;
  if (instance.visibilityAnim != 0) {
    instance.animations.cancel(instance.visibilityAnim);
    instance.visibilityAnim = 0;
  }
  if (std::abs(instance.visibility - target) < 1.0e-3F) {
    instance.surface->requestLayout();
    return;
  }
  Instance* instancePtr = &instance;
  instance.visibilityAnim = instance.animations.animate(
      instance.visibility, target, kFadeMs, Easing::EaseOutCubic,
      [instancePtr](float value) { instancePtr->visibility = value; },
      [this, instancePtr]() {
        instancePtr->visibilityAnim = 0;
        layoutCapsule(*instancePtr);
      },
      instance.capsule
  );
  instance.surface->requestLayout();
  instance.surface->requestRedraw();
}

void Island::refreshAll() {
  for (auto& instance : m_instances) {
    animatePresentation(*instance);
  }
}

bool Island::post(Activity activity) {
  // Nothing to show (a nameless notification, an OSD event without text or level): leave the island alone.
  if (!m_islandConfig.has_value() || (activity.title.empty() && !activity.level.has_value())) {
    return false;
  }
  if (!m_activities.post(std::move(activity))) {
    return false;
  }
  scheduleActivityTimer();
  refreshAll();
  return true;
}

void Island::scheduleActivityTimer() {
  const auto& state = m_activities.state();
  if (!state.transient.has_value()) {
    m_activityTimer.stop();
    return;
  }
  const auto delay = std::chrono::milliseconds(static_cast<std::int64_t>(std::ceil(state.transientRemainingMs)));
  m_activityTimer.start(delay, [this]() { onActivityTimer(); });
}

void Island::onActivityTimer() {
  const float remaining = m_activities.state().transientRemainingMs;
  if (m_activities.advance(remaining)) {
    refreshAll();
    return;
  }
  // Hovered: the tracker holds the activity; the leave handler re-arms the timer.
}

bool Island::takeOsdContent(const OsdContent& content) {
  if (!m_islandConfig.has_value() || m_instances.empty()) {
    return false;
  }
  // Only take over where the island is actually on screen; elsewhere the regular OSD still has to show.
  const bool visibleSomewhere =
      std::ranges::any_of(m_instances, [this](const auto& instance) { return shownOn(*instance); });
  if (!visibleSomewhere) {
    return false;
  }
  Activity activity{
      .kind = activityKindForOsd(content.kind),
      .glyph = content.icon,
      .title = osdTitle(content.kind),
      .subtitle = content.value,
      .inactive = content.inactive,
      .overLimit = content.overLimit,
  };
  if (activity.title.empty()) {
    activity.title = content.value;
    activity.subtitle.clear();
  }
  if (content.showProgress) {
    activity.level = std::max(content.progress, 0.0F);
  }
  // The OSD stays away only when the island really shows the change; refused (kind off, a more important activity
  // showing), the regular OSD has to.
  return post(std::move(activity)) && m_islandConfig->suppressOsd;
}

void Island::onPowerChanged(const UPowerChange& /*change*/) {
  if (m_upower == nullptr) {
    return;
  }
  const auto& state = m_upower->state();
  if (!state.isPresent) {
    return;
  }
  const bool charging = batteryStatePlugged(state.state).value_or(!state.onBattery);
  const auto percent = static_cast<float>(state.percentage);
  const bool chargerChanged = m_lastCharging.has_value() && *m_lastCharging != charging;
  const bool crossedLow = !charging
      && m_lastBatteryPercent.has_value()
      && ((*m_lastBatteryPercent > kLowBattery && percent <= kLowBattery)
          || (*m_lastBatteryPercent > kCriticalBattery && percent <= kCriticalBattery));
  m_lastCharging = charging;
  m_lastBatteryPercent = percent;
  if (!chargerChanged && !crossedLow) {
    return;
  }
  Activity activity{
      .kind = ActivityKind::Battery,
      .glyph = batteryGlyphName(state.percentage, state.state),
      .title = crossedLow ? i18n::tr("island.low-battery")
          : charging      ? i18n::tr("island.charging")
                          : i18n::tr("island.on-battery"),
      .subtitle = percentText(state.percentage),
      .level = percent / 100.0F,
      .inactive = false,
  };
  post(std::move(activity));
}

void Island::onMediaChanged() {
  if (m_mpris == nullptr) {
    return;
  }
  const auto player = m_mpris->activePlayer();
  const bool playing = player.has_value() && player->playbackStatus == "Playing";
  if (!playing) {
    m_activities.setAmbient(std::nullopt);
    refreshAll();
    return;
  }
  std::string artist;
  for (const auto& name : player->artists) {
    if (!artist.empty()) {
      artist += ", ";
    }
    artist += name;
  }
  Activity media{
      .kind = ActivityKind::Media,
      .glyph = "music",
      .title = player->title.empty() ? player->identity : player->title,
      .subtitle = artist,
  };
  const auto& previous = m_activities.state().ambient;
  const bool trackChanged = !previous.has_value() || previous->title != media.title;
  m_activities.setAmbient(media);
  if (trackChanged) {
    post(std::move(media));
  } else {
    refreshAll();
  }
}

void Island::onNotificationEvent(const Notification& notification, NotificationEvent event) {
  if (event != NotificationEvent::Added) {
    return;
  }
  post(
      Activity{
          .kind = ActivityKind::Notification,
          .glyph = "bell",
          .title = notification.appName.empty() ? notification.summary : notification.appName,
          .subtitle = notification.appName.empty() ? notification.body : notification.summary,
      }
  );
}

void Island::activateClick(Instance& instance) {
  const auto& action = m_clickAction;
  if (action.kind == noctalia::bar::WidgetAction::Kind::Ipc && noctalia::bar::isAnchoredPanelVerb(action.verb)) {
    if (m_panels == nullptr) {
      return;
    }
    const auto args = noctalia::bar::parsePanelVerbArgs(action.args);
    if (args.panelId.empty()) {
      return;
    }
    const std::string panelId(args.panelId);
    const PanelOpenRequest request{
        .output = instance.output, .context = args.panelContext, .screenPosition = "top_center"
    };
    if (action.verb == "panel-toggle") {
      m_panels->togglePanel(panelId, request);
    } else {
      m_panels->openPanel(panelId, request);
    }
    return;
  }
  noctalia::bar::WidgetActionDispatcher dispatcher;
  dispatcher.setIpcService(m_ipc);
  (void)dispatcher.run(action, IpcInvocationContext{.output = instance.output});
}

bool Island::onPointerEvent(const PointerEvent& event) {
  Instance* instance = instanceForSurface(event.surface);
  if (instance == nullptr) {
    return false;
  }
  const auto sx = static_cast<float>(event.sx);
  const auto sy = static_cast<float>(event.sy);
  switch (event.type) {
  case PointerEvent::Type::Enter:
    instance->inputDispatcher.pointerEnter(sx, sy, event.serial);
    return true;
  case PointerEvent::Type::Leave:
    instance->inputDispatcher.pointerLeave();
    return true;
  case PointerEvent::Type::Motion:
    instance->inputDispatcher.pointerMotion(sx, sy, event.serial);
    return true;
  case PointerEvent::Type::Button:
    instance->inputDispatcher.pointerButton(sx, sy, event.button, event.pressed, event.serial, event.time, event.touch);
    return true;
  case PointerEvent::Type::Axis:
    return true;
  }
  return false;
}

void Island::requestLayout() {
  for (auto& instance : m_instances) {
    if (instance->surface != nullptr) {
      instance->surface->requestLayout();
    }
  }
}

void Island::requestRedraw() {
  for (auto& instance : m_instances) {
    if (instance->surface != nullptr) {
      instance->surface->requestLayout();
      instance->surface->requestRedraw();
    }
  }
}

std::vector<wl_surface*> Island::surfaces() const {
  std::vector<wl_surface*> out;
  for (const auto& instance : m_instances) {
    if (instance->surface != nullptr && instance->surface->wlSurface() != nullptr) {
      out.push_back(instance->surface->wlSurface());
    }
  }
  return out;
}

std::vector<InputRect> Island::surfaceRectsForOutput(wl_output* output) const {
  std::vector<InputRect> out;
  if (m_platform == nullptr) {
    return out;
  }
  const auto& outputs = m_platform->outputs();
  for (const auto& instance : m_instances) {
    if (instance->output != output || instance->capsule == nullptr || instance->visibility <= 0.01F) {
      continue;
    }
    const auto it = std::ranges::find(outputs, instance->outputName, &WaylandOutput::name);
    if (it == outputs.end() || instance->sceneRoot == nullptr) {
      continue;
    }
    const int outputW = it->effectiveLogicalWidth();
    const int surfaceX = (outputW - static_cast<int>(instance->sceneRoot->width())) / 2;
    out.push_back(
        InputRect{
            surfaceX + static_cast<int>(instance->capsule->x()), static_cast<int>(instance->capsule->y()),
            static_cast<int>(std::ceil(instance->capsule->width())),
            static_cast<int>(std::ceil(instance->capsule->height()))
        }
    );
  }
  return out;
}

Island::Instance* Island::instanceForSurface(wl_surface* surface) const noexcept {
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
