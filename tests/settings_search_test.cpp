#include "config/config_types.h"
#include "i18n/i18n_service.h"
#include "launcher/settings_provider.h"
#include "shell/settings/settings_registry.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <string_view>

namespace {

  bool expect(bool condition, std::string_view message) {
    if (!condition) {
      std::println(stderr, "settings_search_test: {}", message);
    }
    return condition;
  }

} // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::println(stderr, "settings_search_test: expected the assets directory as argument");
    return 1;
  }
  setenv("NOCTALIA_ASSETS_DIR", argv[1], 1);
  unsetenv("LC_ALL");
  unsetenv("LC_MESSAGES");
  setenv("LANG", "en_US.UTF-8", 1);
  i18n::Service::instance().init();

  Config config;
  config.bars.push_back(BarConfig{});
  const auto registry = settings::buildSettingsRegistry(config, nullptr, nullptr, settings::RegistryEnvironment{});

  bool ok = true;
  const auto barPosition =
      std::ranges::find(registry, std::vector<std::string>{"bar", "default", "position"}, &settings::SettingEntry::path);
  ok = expect(barPosition != registry.end(), "registry has the default bar's position setting") && ok;
  if (barPosition != registry.end()) {
    ok = expect(settings::matchesSettingQuery(*barPosition, "bar position"), "matches words that are not adjacent")
        && ok;
    ok = expect(settings::matchesSettingQuery(*barPosition, "Position BAR"), "matches in any order and case") && ok;
    ok = expect(!settings::matchesSettingQuery(*barPosition, "bar zzzz"), "requires every word") && ok;
  }

  SettingsProvider provider([&registry]() { return registry; }, [](std::vector<std::string> /*path*/) {}, nullptr);
  const auto results = provider.query("bar position");
  ok = expect(!results.empty(), "provider finds settings for a multi-word query") && ok;
  const bool hasBarPosition = std::ranges::any_of(results, [](const LauncherResult& result) {
    return result.id == std::string("bar\x1f" "default\x1f" "position");
  });
  ok = expect(hasBarPosition, "results include the bar position setting") && ok;
  ok = expect(provider.query("   ").empty(), "blank query lists nothing") && ok;
  ok = expect(provider.query("zzzz-no-such-setting").empty(), "unknown words find nothing") && ok;

  return ok ? 0 : 1;
}
