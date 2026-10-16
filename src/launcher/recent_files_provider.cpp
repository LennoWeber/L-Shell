#include "launcher/recent_files_provider.h"

#include "config/config_service.h"
#include "core/log.h"
#include "core/process/process.h"
#include "i18n/i18n.h"
#include "util/fuzzy_match.h"
#include "util/string_utils.h"

#include <algorithm>
#include <glib.h>
#include <system_error>

namespace {

  constexpr Logger kLog("launcher");
  constexpr std::size_t kMaxResults = 5;

  [[nodiscard]] std::string_view glyphForMimeType(std::string_view mime) {
    if (mime == "inode/directory") {
      return "folder";
    }
    if (mime.starts_with("image/")) {
      return "photo";
    }
    if (mime.starts_with("video/")) {
      return "video";
    }
    if (mime.starts_with("audio/")) {
      return "music";
    }
    return "file-description";
  }

  // "12 Min.", "1 Std.": how long ago, in the largest whole unit.
  [[nodiscard]] std::string ageLabel(std::chrono::system_clock::time_point used) {
    using namespace std::chrono;
    const auto age = system_clock::now() - used;
    const auto minutes = duration_cast<std::chrono::minutes>(age).count();
    if (minutes < 1) {
      return i18n::tr("launcher.age.now");
    }
    if (minutes < 60) {
      return i18n::tr("launcher.age.minutes", "count", minutes);
    }
    const auto hours = minutes / 60;
    if (hours < 24) {
      return i18n::tr("launcher.age.hours", "count", hours);
    }
    return i18n::tr("launcher.age.days", "count", hours / 24);
  }

} // namespace

RecentFilesProvider::RecentFilesProvider(ConfigService* config)
    : m_config(config), m_listPath(std::filesystem::path(g_get_user_data_dir()) / "recently-used.xbel") {}

std::string RecentFilesProvider::displayName() const { return i18n::tr("launcher.providers.recent.title"); }

bool RecentFilesProvider::defaultIncludeInGlobalSearch() const {
  return m_config != nullptr && m_config->config().shell.launcher.recentFiles;
}

void RecentFilesProvider::reloadIfChanged() const {
  std::error_code ec;
  const auto writeTime = std::filesystem::last_write_time(m_listPath, ec);
  if (ec) {
    m_files.clear();
    m_loadedWriteTime = {};
    return;
  }
  if (writeTime == m_loadedWriteTime) {
    return;
  }
  m_loadedWriteTime = writeTime;
  m_files.clear();

  GBookmarkFile* bookmarks = g_bookmark_file_new();
  GError* error = nullptr;
  if (!g_bookmark_file_load_from_file(bookmarks, m_listPath.c_str(), &error)) {
    kLog.warn("recent files: cannot read {}: {}", m_listPath.string(), error != nullptr ? error->message : "");
    g_clear_error(&error);
    g_bookmark_file_free(bookmarks);
    return;
  }

  gsize count = 0;
  gchar** uris = g_bookmark_file_get_uris(bookmarks, &count);
  for (gsize i = 0; i < count; ++i) {
    gchar* path = g_filename_from_uri(uris[i], nullptr, nullptr);
    if (path == nullptr) {
      continue; // not a local file
    }
    RecentFile file;
    file.path = path;
    g_free(path);
    if (!std::filesystem::exists(file.path, ec)) {
      continue;
    }
    file.name = std::filesystem::path(file.path).filename().string();
    file.nameLower = StringUtils::toLower(file.name);
    if (gchar* mime = g_bookmark_file_get_mime_type(bookmarks, uris[i], nullptr); mime != nullptr) {
      file.mimeType = mime;
      g_free(mime);
    }
    GDateTime* visited = g_bookmark_file_get_visited_date_time(bookmarks, uris[i], nullptr);
    GDateTime* modified = g_bookmark_file_get_modified_date_time(bookmarks, uris[i], nullptr);
    const gint64 seconds = std::max(
        visited != nullptr ? g_date_time_to_unix(visited) : 0, modified != nullptr ? g_date_time_to_unix(modified) : 0
    );
    file.used = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
    m_files.push_back(std::move(file));
  }
  g_strfreev(uris);
  g_bookmark_file_free(bookmarks);

  std::ranges::sort(m_files, std::ranges::greater{}, &RecentFile::used);
}

std::vector<LauncherResult> RecentFilesProvider::query(std::string_view text) const {
  reloadIfChanged();
  const std::string pattern = StringUtils::toLower(StringUtils::trim(text));

  std::vector<LauncherResult> results;
  for (const RecentFile& file : m_files) {
    double score = 0.0;
    if (!pattern.empty()) {
      score = FuzzyMatch::score(pattern, file.nameLower);
      if (!FuzzyMatch::isMatch(score)) {
        continue;
      }
    }
    LauncherResult result;
    result.id = file.path;
    result.title = file.name;
    result.subtitle = std::filesystem::path(file.path).parent_path().string();
    result.glyphName = std::string(glyphForMimeType(file.mimeType));
    result.trailing = ageLabel(file.used);
    result.presentation = "quiet";
    result.score = score;
    results.push_back(std::move(result));
    if (pattern.empty() && results.size() >= kMaxResults) {
      break;
    }
  }
  if (!pattern.empty()) {
    std::ranges::stable_sort(results, std::ranges::greater{}, &LauncherResult::score);
    if (results.size() > kMaxResults) {
      results.resize(kMaxResults);
    }
  }
  return results;
}

bool RecentFilesProvider::activate(const LauncherResult& result) {
  if (!process::commandExists("xdg-open")) {
    kLog.warn("recent files: xdg-open not found");
    return false;
  }
  return process::runAsync(std::vector<std::string>{"xdg-open", result.id});
}
