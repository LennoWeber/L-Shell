#include "launcher/settings_provider.h"

#include "config/config_service.h"
#include "core/deferred_call.h"
#include "i18n/i18n.h"
#include "util/fuzzy_match.h"
#include "util/string_utils.h"

#include <algorithm>
#include <string>
#include <utility>

namespace {

  constexpr std::size_t kMaxResults = 30;
  // Result ids carry the setting path; parts may contain dots (bar or monitor names).
  constexpr char kPathSeparator = '\x1f';

  [[nodiscard]] std::string joinPath(const std::vector<std::string>& path) {
    std::string joined;
    for (const auto& part : path) {
      if (!joined.empty()) {
        joined += kPathSeparator;
      }
      joined += part;
    }
    return joined;
  }

  [[nodiscard]] std::vector<std::string> splitPath(std::string_view joined) {
    std::vector<std::string> path;
    std::size_t start = 0;
    while (start <= joined.size()) {
      const std::size_t end = std::min(joined.find(kPathSeparator, start), joined.size());
      path.emplace_back(joined.substr(start, end - start));
      start = end + 1;
    }
    return path;
  }

} // namespace

SettingsProvider::SettingsProvider(IndexBuilder buildIndex, SettingOpener openSetting, ConfigService* config)
    : m_buildIndex(std::move(buildIndex)), m_openSetting(std::move(openSetting)), m_config(config) {}

std::string SettingsProvider::displayName() const { return i18n::tr("launcher.providers.settings.title"); }

const std::vector<settings::SettingEntry>& SettingsProvider::index() const {
  if (!m_index.has_value()) {
    m_index = m_buildIndex ? m_buildIndex() : std::vector<settings::SettingEntry>{};
  }
  return *m_index;
}

std::vector<LauncherResult> SettingsProvider::query(std::string_view text) const {
  const std::string query = settings::normalizedSettingQuery(StringUtils::trim(text));
  if (query.empty()) {
    return {};
  }

  std::vector<LauncherResult> results;
  for (const auto& entry : index()) {
    if (entry.visibleWhen && m_config != nullptr && !entry.visibleWhen(m_config->config())) {
      continue;
    }
    if (!settings::matchesNormalizedSettingQuery(entry, query)) {
      continue;
    }

    // Every query word already matched; a fuzzy hit on the title ranks closer names first.
    const double titleScore = FuzzyMatch::score(query, StringUtils::toLower(entry.title));

    LauncherResult result;
    result.id = joinPath(entry.path);
    result.title = entry.title;
    result.subtitle = i18n::tr(settings::settingsSectionLabelKey(entry.section));
    if (!entry.group.empty()) {
      result.subtitle += " › " + i18n::tr("settings.navigation.groups." + entry.group);
    }
    result.glyphName = std::string(settings::sectionGlyph(entry.section));
    result.category = i18n::tr(settings::settingsSectionLabelKey(entry.section));
    result.score = FuzzyMatch::isMatch(titleScore) ? titleScore : 0.0;
    results.push_back(std::move(result));
  }

  std::ranges::stable_sort(results, std::ranges::greater{}, &LauncherResult::score);
  if (results.size() > kMaxResults) {
    results.resize(kMaxResults);
  }
  return results;
}

bool SettingsProvider::activate(const LauncherResult& result) {
  if (!m_openSetting || result.id.empty()) {
    return false;
  }
  // Deferred so the launcher closes first; the settings window then takes focus.
  DeferredCall::callLater([open = m_openSetting, path = splitPath(result.id)]() { open(path); });
  return true;
}
