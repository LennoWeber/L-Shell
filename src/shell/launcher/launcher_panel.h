#pragma once

#include "launcher/launcher_provider.h"
#include "launcher/usage_tracker.h"
#include "shell/panel/panel.h"
#include "system/icon_resolver.h"
#include "ui/signal.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class ContextMenuPopup;
class Flex;
class Input;
class Label;
class LauncherResultAdapter;
class LauncherAppGridAdapter;
class Renderer;
class Segmented;
class ScrollView;
class Separator;
class VirtualGridView;
class ConfigService;
class AsyncTextureCache;
class SessionActionRunner;
struct SessionPanelActionConfig;

class LauncherPanel : public Panel {
public:
  // `sessionActions` runs the sheet's session tiles; without it the sheet has none.
  LauncherPanel(ConfigService* config, AsyncTextureCache* asyncTextures, SessionActionRunner* sessionActions = nullptr);
  ~LauncherPanel() override;

  void addProvider(std::unique_ptr<LauncherProvider> provider);
  // Drop every dynamically-registered (plugin-backed) provider, so the enabled
  // plugin set can be re-applied without disturbing the built-in providers.
  void clearDynamicProviders();
  // Drop providers whose stable id starts with `prefix` (e.g. config-driven "dmenu.").
  void clearProvidersWithIdPrefix(std::string_view prefix);
  // Restrict the next open to a single provider (stdin/dmenu session). When set,
  // onInputChanged queries only that provider and skips prefix routing/overview.
  // Cleared on close.
  void setScopedProvider(std::string_view providerId, std::string_view placeholder = {});

  void create() override;
  void onOpen(std::string_view context) override;
  void onClose() override;
  void onIconThemeChanged() override;

  void clearUsage();
  void syncUsageTrackingState();

  // Invoked after a terminal close when the activation copied text and the provider
  // supports auto-paste. The host schedules virtual-keyboard paste (clipboard path).
  void setCopiedActivationCallback(std::function<void()> callback) { m_onCopiedActivation = std::move(callback); }

  [[nodiscard]] float preferredWidth() const override;
  // Only the search row until there is something to show; the command bar then fits its rows.
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] std::optional<float> floatingCornerRadius() const override;
  [[nodiscard]] std::optional<float> contentPadding() const override;
  [[nodiscard]] LayerShellKeyboard keyboardMode() const override { return LayerShellKeyboard::Exclusive; }
  [[nodiscard]] InputArea* initialFocusArea() const override;
  [[nodiscard]] bool handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) override;
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override;

private:
  enum ActiveCategoryType { All, RecentlyUsed, Category };

  struct CategoryFilterSlot {
    ActiveCategoryType type;
    std::size_t categoryIndex = 0;
  };

  void onPanelCardOpacityChanged(float opacity) override;
  void doLayout(Renderer& renderer, float width, float height) override;
  void onInputChanged(const std::string& text);
  void setQuery(std::string query);
  // Re-gather the current query, preserving the selected result by identity.
  void reapplyCurrentQuery();
  // A plugin provider delivered fresh async results — re-gather if the panel is open.
  void onProviderResultsChanged();
  void refreshResults();
  void activateAt(std::size_t index);
  void activateSelected();
  bool handleKeyEvent(std::uint32_t sym, std::uint32_t modifiers);
  void applyEmptyState();
  void bindDetailResult();
  [[nodiscard]] bool shouldUseDetailPresentation() const;
  [[nodiscard]] bool startsWithLauncherPrefix(std::string_view text) const;
  void applyProviderConfig(LauncherProvider& provider) const;
  void finishActivation(LauncherProvider& provider, const std::string& resultId, bool copied);
  [[nodiscard]] std::vector<LauncherResult> providerOverviewResults(std::string_view text) const;
  [[nodiscard]] bool openAppActionsMenu(std::size_t index, float anchorX, float anchorY);
  void rebuildCategoryFilter(const std::vector<LauncherCategory>& categories);
  void setCategoryFilterVisible(bool visible);
  void setActiveCategorySlot(std::size_t slotIndex);
  void applyActiveCategory();
  // Spotlight-style grouping: "Top Hit" first, then one header per provider.
  [[nodiscard]] bool wantsSectionHeaders() const;
  void insertSectionHeaders();
  [[nodiscard]] std::string providerDisplayName(std::string_view providerId) const;
  [[nodiscard]] bool shouldCollapse() const;
  void syncCollapsedState();
  // shell.launcher.style = "command"; fixed from create() until close so the tree and geometry agree.
  [[nodiscard]] bool commandStyle() const;
  // shell.launcher.style = "sheet", fixed the same way.
  [[nodiscard]] bool sheetStyle() const;
  // The sheet's session tiles at its foot: lock, sleep and shut down, as far as they are configured.
  [[nodiscard]] std::unique_ptr<Flex> buildSessionFoot(float scale);
  void runSessionAction(const SessionPanelActionConfig& action);
  // A sheet section's title: "Apps" for applications, the provider's name otherwise.
  [[nodiscard]] std::string sectionTitle(std::string_view providerId) const;
  [[nodiscard]] std::string defaultPlaceholder() const;
  [[nodiscard]] std::string resultTag(std::string_view providerId) const;
  [[nodiscard]] float commandBodyHeight() const;
  // Footer count and key hints, the "/" hint, and the panel height as the rows come and go.
  void syncCommandChrome();
  void queuePreferredSizeRelayout();
  void syncLauncherListStyle();
  void syncLauncherViewLayout(Renderer* renderer = nullptr);
  [[nodiscard]] bool shouldUseAppGrid() const;
  void refreshLauncherAppIconColorization();
  void updateLauncherGridMetrics(Renderer& renderer);
  void updatePinnedApplicationState();
  void applyPinnedApplicationOrder();
  void reorderPinnedApplication(std::string_view sourcePath, std::string_view targetPath);
  [[nodiscard]] bool shouldTrackUsage() const;

  std::vector<std::unique_ptr<LauncherProvider>> m_providers;
  std::vector<LauncherResult> m_results;
  std::vector<LauncherResult> m_allResults;
  UsageTracker m_usageTracker;
  IconResolver m_iconResolver;

  Flex* m_container = nullptr;
  Input* m_input = nullptr;
  Separator* m_searchDivider = nullptr;
  Flex* m_commandHint = nullptr;
  Separator* m_footerDivider = nullptr;
  Flex* m_footer = nullptr;
  Label* m_footerCount = nullptr;
  Flex* m_footerNavigate = nullptr;
  Flex* m_footerOpen = nullptr;
  Flex* m_footerActions = nullptr;
  Segmented* m_categoryFilter = nullptr;
  Flex* m_body = nullptr;
  VirtualGridView* m_grid = nullptr;
  ScrollView* m_detailScroll = nullptr;
  Label* m_detailSubtitle = nullptr;
  Label* m_detailBody = nullptr;
  Label* m_emptyLabel = nullptr;
  bool m_anyProviderLoading = false;
  std::unique_ptr<LauncherResultAdapter> m_listAdapter;
  std::unique_ptr<LauncherAppGridAdapter> m_gridAdapter;

  std::string m_query;
  std::string m_scopedProviderId;
  std::string m_scopedPlaceholder;
  ActiveCategoryType m_activeCategoryType = All;
  std::string m_activeCategory;
  std::vector<LauncherCategory> m_currentCategories;
  std::vector<CategoryFilterSlot> m_categoryFilterSlots;
  bool m_hasRecentlyUsed = false;
  std::size_t m_selectedIndex = 0;
  bool m_categoryFilterVisible = true;
  bool m_collapsed = true;
  bool m_commandStyle = false;
  bool m_sheetStyle = false;
  float m_commandHeight = 0.0F;
  bool m_launcherShowIcons = true;
  bool m_launcherShowAppOriginIndicator = true;
  bool m_launcherCompact = false;
  bool m_launcherAppGrid = false;
  bool m_usingAppGrid = false;
  float m_launcherRowHeight = 0.0F;
  std::uint64_t m_desktopEntriesVersion = 0;
  ConfigService* m_config = nullptr;
  AsyncTextureCache* m_asyncTextures = nullptr;
  SessionActionRunner* m_sessionActions = nullptr;
  std::unique_ptr<ContextMenuPopup> m_actionsMenu;
  Signal<>::ScopedConnection m_appIconColorizeConn;
  std::function<void()> m_onCopiedActivation;
};
