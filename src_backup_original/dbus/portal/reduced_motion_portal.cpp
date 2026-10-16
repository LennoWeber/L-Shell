#include "dbus/portal/reduced_motion_portal.h"

#include "core/deferred_call.h"
#include "core/log.h"
#include "dbus/session_bus.h"

#include <chrono>
#include <string>

namespace {

  constexpr Logger kLog("motion");

  const sdbus::ServiceName kPortalBusName{"org.freedesktop.portal.Desktop"};
  const sdbus::ObjectPath kPortalObjectPath{"/org/freedesktop/portal/desktop"};
  constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";
  constexpr auto kAppearanceNamespace = "org.freedesktop.appearance";
  constexpr auto kReducedMotionKey = "reduced-motion";
  // Never hold up startup on a portal that is slow to activate.
  constexpr auto kReadTimeout = std::chrono::seconds(3);

} // namespace

ReducedMotionPortal::ReducedMotionPortal(SessionBus& bus, ChangeCallback callback) : m_callback(std::move(callback)) {
  m_proxy = sdbus::createProxy(bus.connection(), kPortalBusName, kPortalObjectPath);
  m_proxy->uponSignal("SettingChanged")
      .onInterface(kSettingsInterface)
      .call([this](const std::string& ns, const std::string& key, const sdbus::Variant& value) {
        if (ns != kAppearanceNamespace || key != kReducedMotionKey) {
          return;
        }
        m_receivedChange = true;
        apply(reducedFromValue(value));
      });
  readInitial(false);
}

ReducedMotionPortal::~ReducedMotionPortal() = default;

bool ReducedMotionPortal::reducedFromValue(const sdbus::Variant& value) {
  try {
    if (value.containsValueOfType<std::uint32_t>()) {
      return value.get<std::uint32_t>() == 1U;
    }
    if (value.containsValueOfType<sdbus::Variant>()) {
      return reducedFromValue(value.get<sdbus::Variant>());
    }
  } catch (const sdbus::Error&) {
  }
  return false;
}

void ReducedMotionPortal::readInitial(bool useDeprecatedRead) {
  const std::weak_ptr<void> aliveGuard = m_aliveGuard;
  try {
    m_proxy->callMethodAsync(useDeprecatedRead ? "Read" : "ReadOne")
        .onInterface(kSettingsInterface)
        .withTimeout(kReadTimeout)
        .withArguments(std::string{kAppearanceNamespace}, std::string{kReducedMotionKey})
        .uponReplyInvoke([this, aliveGuard,
                          useDeprecatedRead](std::optional<sdbus::Error> error, sdbus::Variant value) {
          if (aliveGuard.expired()) {
            return;
          }
          if (error.has_value()) {
            // Portals before Settings v2 only have Read; no portal at all means no preference.
            if (!useDeprecatedRead
                && error->getName() == sdbus::Error::Name{"org.freedesktop.DBus.Error.UnknownMethod"}) {
              DeferredCall::callLater([this, aliveGuard]() {
                if (!aliveGuard.expired()) {
                  readInitial(true);
                }
              });
              return;
            }
            kLog.debug("reduced-motion preference unavailable: {}", error->what());
            return;
          }
          // A SettingChanged that raced ahead of this reply is newer.
          if (!m_receivedChange) {
            apply(reducedFromValue(value));
          }
        });
  } catch (const sdbus::Error& e) {
    kLog.debug("reduced-motion preference unavailable: {}", e.what());
  }
}

void ReducedMotionPortal::apply(bool reduced) {
  const std::weak_ptr<void> aliveGuard = m_aliveGuard;
  // Out of the D-Bus dispatch: the callback snaps running animations.
  DeferredCall::callLater([this, aliveGuard, reduced]() {
    if (aliveGuard.expired() || !m_callback) {
      return;
    }
    kLog.info("desktop reduced-motion preference: {}", reduced ? "reduce" : "no preference");
    m_callback(reduced);
  });
}
