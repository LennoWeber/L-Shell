#pragma once

#include "launcher/launcher_provider.h"
#include "shell/settings/settings_registry.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

class ConfigService;

// Finds shell settings by name ("bar position", "dock") and opens the settings
// window on the chosen one. Searches the same registry the settings window shows,
// so labels are translated and gating (compositor, hardware) matches the window.
class SettingsProvider : public LauncherProvider {
public:
  using IndexBuilder = std::function<std::vector<settings::SettingEntry>()>;
  using SettingOpener = std::function<void(std::vector<std::string> path)>;

  SettingsProvider(IndexBuilder buildIndex, SettingOpener openSetting, ConfigService* config);

  [[nodiscard]] std::string_view defaultPrefix() const override { return "set"; }
  [[nodiscard]] std::string_view id() const override { return "Settings"; }
  [[nodiscard]] std::string displayName() const override;
  [[nodiscard]] std::string_view defaultGlyphName() const override { return "settings"; }
  [[nodiscard]] bool trackUsage() const override { return true; }
  [[nodiscard]] bool defaultIncludeInGlobalSearch() const override { return true; }

  // The index is built on first use per launcher session and dropped on close,
  // so it follows config and language changes.
  void reset() override { m_index.reset(); }

  [[nodiscard]] std::vector<LauncherResult> query(std::string_view text) const override;
  bool activate(const LauncherResult& result) override;

private:
  [[nodiscard]] const std::vector<settings::SettingEntry>& index() const;

  IndexBuilder m_buildIndex;
  SettingOpener m_openSetting;
  ConfigService* m_config = nullptr;
  mutable std::optional<std::vector<settings::SettingEntry>> m_index;
};
