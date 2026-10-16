#include "ui/app_glyph.h"

#include "system/desktop_entry.h"
#include "util/string_utils.h"

#include <array>
#include <glib.h>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

  constexpr std::string_view kGenericAppGlyph = "app-window";

  // Most specific first: an IDE is also "Development", a music player also "AudioVideo".
  constexpr auto kCategoryGlyphs = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"TerminalEmulator", "terminal"},
      {"WebBrowser", "world"},
      {"FileManager", "folder"},
      {"Email", "mail"},
      {"InstantMessaging", "message"},
      {"Chat", "message"},
      {"IRCClient", "message"},
      {"Calendar", "calendar"},
      {"IDE", "code"},
      {"TextEditor", "code"},
      {"Development", "code"},
      {"Calculator", "calculator"},
      {"Photography", "photo"},
      {"Viewer", "photo"},
      {"RasterGraphics", "brush"},
      {"VectorGraphics", "brush"},
      {"Graphics", "photo"},
      {"Music", "music"},
      {"Audio", "music"},
      {"Video", "video"},
      {"TV", "device-tv"},
      {"AudioVideo", "player-play"},
      {"Game", "device-gamepad-2"},
      {"WordProcessor", "file-text"},
      {"Spreadsheet", "file-text"},
      {"Presentation", "presentation"},
      {"Office", "file-text"},
      {"Education", "school"},
      {"Science", "flask"},
      {"Settings", "settings"},
      {"Monitor", "activity"},
      {"System", "settings"},
      {"Security", "key"},
      {"Network", "world"},
      {"Archiving", "box"},
      {"Utility", "tool"},
  });

  [[nodiscard]] bool hasCategory(std::string_view categories, std::string_view category) {
    std::size_t start = 0;
    while (start < categories.size()) {
      const std::size_t end = categories.find(';', start);
      const std::string_view token =
          categories.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
      if (token == category) {
        return true;
      }
      if (end == std::string_view::npos) {
        break;
      }
      start = end + 1;
    }
    return false;
  }

  // Categories of the system-wide entry with this id, which a user-level entry of the same id shadows.
  [[nodiscard]] const std::string& shadowedCategories(const std::string& id) {
    static std::unordered_map<std::string, std::string> cache;
    if (const auto it = cache.find(id); it != cache.end()) {
      return it->second;
    }
    std::string categories;
    for (const gchar* const* dir = g_get_system_data_dirs(); *dir != nullptr && categories.empty(); ++dir) {
      const std::string path = std::string(*dir) + "/applications/" + id + ".desktop";
      GKeyFile* file = g_key_file_new();
      if (g_key_file_load_from_file(file, path.c_str(), G_KEY_FILE_NONE, nullptr)) {
        if (gchar* value = g_key_file_get_string(file, "Desktop Entry", "Categories", nullptr); value != nullptr) {
          categories = value;
          g_free(value);
        }
      }
      g_key_file_free(file);
    }
    return cache.emplace(id, std::move(categories)).first->second;
  }

  // Last resort for entries without any categories: well-known names in the id, icon or command.
  [[nodiscard]] std::string_view guessedGlyph(const DesktopEntry& entry) {
    static constexpr auto kHints = std::to_array<std::pair<std::string_view, std::string_view>>({
        {"term", "terminal"},
        {"ghostty", "terminal"},
        {"kitty", "terminal"},
        {"alacritty", "terminal"},
        {"foot", "terminal"},
        {"konsole", "terminal"},
        {"browser", "world"},
        {"firefox", "world"},
        {"chrom", "world"},
        {"brave", "world"},
        {"zen", "world"},
        {"helium", "world"},
        {"mail", "mail"},
        {"thunderbird", "mail"},
        {"spotify", "music"},
        {"music", "music"},
        {"code", "code"},
        {"zed", "code"},
        {"cursor", "code"},
        {"idea", "code"},
        {"vim", "code"},
        {"dolphin", "folder"},
        {"nautilus", "folder"},
        {"files", "folder"},
        {"element", "message"},
        {"riot", "message"},
        {"signal", "message"},
        {"discord", "message"},
        {"telegram", "message"},
        {"slack", "message"},
        {"steam", "device-gamepad-2"},
        {"heroic", "device-gamepad-2"},
        {"gimp", "brush"},
        {"kdenlive", "video"},
        {"office", "file-text"},
        {"anki", "school"},
        {"zotero", "school"},
    });
    const std::string haystack = StringUtils::toLower(entry.id + " " + entry.icon + " " + entry.exec);
    for (const auto& [hint, glyph] : kHints) {
      if (haystack.find(hint) != std::string::npos) {
        return glyph;
      }
    }
    return kGenericAppGlyph;
  }

} // namespace

std::string_view appGlyph(std::string_view categories) {
  for (const auto& [category, glyph] : kCategoryGlyphs) {
    if (hasCategory(categories, category)) {
      return glyph;
    }
  }
  return kGenericAppGlyph;
}

std::string_view appGlyph(const DesktopEntry& entry) {
  if (!entry.categories.empty()) {
    return appGlyph(entry.categories);
  }
  // A user-level copy without categories (some tools write bare ones) hides the installed entry's: read those.
  if (const std::string& shadowed = shadowedCategories(entry.id); !shadowed.empty()) {
    return appGlyph(shadowed);
  }
  return guessedGlyph(entry);
}

std::string_view appGlyphForDesktopId(std::string_view desktopEntryId) {
  if (desktopEntryId.empty()) {
    return kGenericAppGlyph;
  }
  for (const DesktopEntry& entry : desktopEntries()) {
    if (entry.id == desktopEntryId) {
      return appGlyph(entry);
    }
  }
  return kGenericAppGlyph;
}
