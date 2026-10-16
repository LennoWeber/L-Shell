#pragma once

#include "launcher/launcher_provider.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

class ConfigService;

// Recently used files from the desktop's shared list ($XDG_DATA_HOME/recently-used.xbel, kept by GTK and Qt apps).
// Shown under "Recent" in the launcher sheet; with shell.launcher.recent_files they also join the global search.
class RecentFilesProvider : public LauncherProvider {
public:
  explicit RecentFilesProvider(ConfigService* config);

  [[nodiscard]] std::string_view defaultPrefix() const override { return "recent"; }
  [[nodiscard]] std::string_view id() const override { return "Recent"; }
  [[nodiscard]] std::string displayName() const override;
  [[nodiscard]] std::string_view defaultGlyphName() const override { return "history"; }
  [[nodiscard]] bool defaultIncludeInGlobalSearch() const override;
  [[nodiscard]] bool listsWithoutQuery() const override { return defaultIncludeInGlobalSearch(); }

  [[nodiscard]] std::vector<LauncherResult> query(std::string_view text) const override;

  bool activate(const LauncherResult& result) override;

private:
  struct RecentFile {
    std::string path;
    std::string name;
    std::string nameLower;
    std::string mimeType;
    std::chrono::system_clock::time_point used;
  };

  void reloadIfChanged() const;

  ConfigService* m_config = nullptr;
  std::filesystem::path m_listPath;
  mutable std::filesystem::file_time_type m_loadedWriteTime{};
  mutable std::vector<RecentFile> m_files;
};
