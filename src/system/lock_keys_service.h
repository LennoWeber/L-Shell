#pragma once

#include "wayland/wayland_seat.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

class WaylandConnection;

class LockKeysService {
public:
  using LockKeysState = WaylandSeat::LockKeysState;
  using ChangeCallback = std::function<void(const LockKeysState& previous, const LockKeysState& current)>;

  explicit LockKeysService(WaylandConnection& wayland);
  ~LockKeysService();
  LockKeysService(const LockKeysService&) = delete;
  LockKeysService& operator=(const LockKeysService&) = delete;

  [[nodiscard]] LockKeysState state() const noexcept;
  void setChangeCallback(ChangeCallback callback);
  [[nodiscard]] int pollTimeoutMs() const;
  void dispatchPoll();
  void refreshNow();

private:
  [[nodiscard]] LockKeysState readCurrentState();
  [[nodiscard]] std::optional<LockKeysState> readCachedSysfsState();
  void discoverSysfsLeds();
  [[nodiscard]] bool hasCachedSysfsLeds() const noexcept;
  void closeLedFiles();

  // A sysfs LED brightness attribute, kept open: polling it is one pread() instead of an open/read/close.
  struct LedFile {
    int fd = -1;
  };

  WaylandConnection& m_wayland;
  LockKeysState m_state;
  bool m_hasState = false;
  bool m_sysfsDiscovered = false;
  int m_sysfsReadFailures = 0;
  std::chrono::steady_clock::time_point m_nextRefreshAt;
  ChangeCallback m_changeCallback;
  std::vector<LedFile> m_capsLockPaths;
  std::vector<LedFile> m_numLockPaths;
  std::vector<LedFile> m_scrollLockPaths;
};
