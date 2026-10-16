#pragma once

#include <functional>
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

class SessionBus;

// Follows the desktop's Reduce Motion preference: xdg-desktop-portal Settings,
// org.freedesktop.appearance "reduced-motion" (0 = no preference, 1 = reduce). Missing portal
// or key means no preference.
class ReducedMotionPortal {
public:
  using ChangeCallback = std::function<void(bool reduced)>;

  ReducedMotionPortal(SessionBus& bus, ChangeCallback callback);
  ~ReducedMotionPortal();

  ReducedMotionPortal(const ReducedMotionPortal&) = delete;
  ReducedMotionPortal& operator=(const ReducedMotionPortal&) = delete;

  // Decodes the setting's value (a uint32, possibly wrapped in one more variant by portals that
  // only implement the deprecated Read). Returns false for anything else.
  [[nodiscard]] static bool reducedFromValue(const sdbus::Variant& value);

private:
  void readInitial(bool useDeprecatedRead);
  void apply(bool reduced);

  std::unique_ptr<sdbus::IProxy> m_proxy;
  ChangeCallback m_callback;
  std::shared_ptr<void> m_aliveGuard = std::make_shared<int>(0);
  bool m_receivedChange = false;
};
