#include "system/lock_keys_service.h"

#include "wayland/wayland_connection.h"

#include <chrono>
#include <fcntl.h>
#include <optional>
#include <string>
#include <unistd.h>
#include <utility>

namespace {

  namespace fs = std::filesystem;

  constexpr auto kRefreshInterval = std::chrono::milliseconds(200);
  // Without keyboard LEDs the state comes from the Wayland keyboard, which only changes while the shell has
  // keyboard focus: no need to look five times a second.
  constexpr auto kSeatOnlyRefreshInterval = std::chrono::seconds(1);
  constexpr int kSysfsFailureRescanThreshold = 3;

  // sysfs attributes report the current value on every read from offset 0.
  bool readBrightness(int fd, bool& on) {
    char buffer[16];
    const ssize_t count = ::pread(fd, buffer, sizeof(buffer) - 1, 0);
    if (count <= 0) {
      return false;
    }
    buffer[count] = '\0';
    on = buffer[0] != '0';
    return true;
  }

} // namespace

LockKeysService::LockKeysService(WaylandConnection& wayland) : m_wayland(wayland) {}

LockKeysService::~LockKeysService() { closeLedFiles(); }

void LockKeysService::closeLedFiles() {
  for (auto* group : {&m_capsLockPaths, &m_numLockPaths, &m_scrollLockPaths}) {
    for (const LedFile& led : *group) {
      if (led.fd >= 0) {
        ::close(led.fd);
      }
    }
    group->clear();
  }
}

LockKeysService::LockKeysState LockKeysService::state() const noexcept { return m_state; }

void LockKeysService::setChangeCallback(ChangeCallback callback) { m_changeCallback = std::move(callback); }

int LockKeysService::pollTimeoutMs() const {
  if (m_nextRefreshAt == std::chrono::steady_clock::time_point{}) {
    return 0;
  }

  const auto now = std::chrono::steady_clock::now();
  if (m_nextRefreshAt <= now) {
    return 0;
  }

  return static_cast<int>(std::chrono::ceil<std::chrono::milliseconds>(m_nextRefreshAt - now).count());
}

void LockKeysService::dispatchPoll() {
  if (m_nextRefreshAt != std::chrono::steady_clock::time_point{}
      && std::chrono::steady_clock::now() < m_nextRefreshAt) {
    return;
  }

  refreshNow();
}

void LockKeysService::refreshNow() {
  const LockKeysState previous = m_state;
  const bool hadState = m_hasState;

  m_state = readCurrentState();
  m_hasState = true;
  m_nextRefreshAt =
      std::chrono::steady_clock::now() + (hasCachedSysfsLeds() ? kRefreshInterval : kSeatOnlyRefreshInterval);

  if (hadState && previous != m_state && m_changeCallback) {
    m_changeCallback(previous, m_state);
  }
}

LockKeysService::LockKeysState LockKeysService::readCurrentState() {
  if (!m_sysfsDiscovered) {
    discoverSysfsLeds();
  }

  if (auto sysfsState = readCachedSysfsState(); sysfsState.has_value()) {
    return *sysfsState;
  }

  return m_wayland.keyboardLockKeysState();
}

std::optional<LockKeysService::LockKeysState> LockKeysService::readCachedSysfsState() {
  if (!hasCachedSysfsLeds()) {
    return std::nullopt;
  }

  LockKeysState state;
  bool readAny = false;
  bool hadFailure = false;

  auto readGroup = [&](const std::vector<LedFile>& leds, bool LockKeysState::* field) {
    for (const LedFile& led : leds) {
      bool on = false;
      if (!readBrightness(led.fd, on)) {
        hadFailure = true;
        continue;
      }
      readAny = true;
      state.*field = state.*field || on;
    }
  };

  readGroup(m_capsLockPaths, &LockKeysState::capsLock);
  readGroup(m_numLockPaths, &LockKeysState::numLock);
  readGroup(m_scrollLockPaths, &LockKeysState::scrollLock);

  if (!readAny) {
    ++m_sysfsReadFailures;
    if (m_sysfsReadFailures >= kSysfsFailureRescanThreshold) {
      discoverSysfsLeds();
    }
    return std::nullopt;
  }

  if (hadFailure) {
    ++m_sysfsReadFailures;
    if (m_sysfsReadFailures >= kSysfsFailureRescanThreshold) {
      discoverSysfsLeds();
    }
  } else {
    m_sysfsReadFailures = 0;
  }

  return state;
}

void LockKeysService::discoverSysfsLeds() {
  closeLedFiles();
  m_sysfsReadFailures = 0;
  m_sysfsDiscovered = true;

  const fs::path ledsDir{"/sys/class/leds"};
  std::error_code ec;
  if (!fs::is_directory(ledsDir, ec)) {
    return;
  }

  for (fs::directory_iterator it(ledsDir, ec), end; it != end && !ec; it.increment(ec)) {
    const std::string name = it->path().filename().string();
    const std::size_t sep = name.find("::");
    if (sep == std::string::npos) {
      continue;
    }

    const std::string kind = name.substr(sep + 2);
    std::vector<LedFile>* target = nullptr;
    if (kind == "capslock") {
      target = &m_capsLockPaths;
    } else if (kind == "numlock") {
      target = &m_numLockPaths;
    } else if (kind == "scrolllock") {
      target = &m_scrollLockPaths;
    }

    if (target == nullptr) {
      continue;
    }

    const fs::path brightnessPath = it->path() / "brightness";
    const int fd = ::open(brightnessPath.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
      continue;
    }
    bool on = false;
    if (!readBrightness(fd, on)) {
      ::close(fd);
      continue;
    }
    target->push_back(LedFile{.fd = fd});
  }
}

bool LockKeysService::hasCachedSysfsLeds() const noexcept {
  return !m_capsLockPaths.empty() || !m_numLockPaths.empty() || !m_scrollLockPaths.empty();
}
