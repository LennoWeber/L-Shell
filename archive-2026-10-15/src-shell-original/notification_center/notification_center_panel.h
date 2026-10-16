#pragma once

#include "shell/panel/panel.h"
#include "system/icon_resolver.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Box;
class Button;
class CompositorPlatform;
class ConfigService;
class Flex;
class Glyph;
class Image;
class InputArea;
class Label;
class NotificationManager;
class Renderer;
class WeatherService;
struct Notification;

struct NotificationCenterServices {
  ConfigService* config = nullptr;
  NotificationManager* notifications = nullptr;
  WeatherService* weather = nullptr;
  CompositorPlatform* platform = nullptr;
};

// macOS Notification Center: the latest notifications as cards over two small widgets, the month and the weather.
// The menu bar clock opens it (`panel-toggle notification-center`); the widgets lead on to Control Center's
// calendar and weather, "N more" to the full notification history.
class NotificationCenterPanel : public Panel {
public:
  static constexpr std::string_view kPanelId = "notification-center";

  explicit NotificationCenterPanel(NotificationCenterServices services);
  ~NotificationCenterPanel() override;

  void create() override;
  void onOpen(std::string_view context) override;
  void onClose() override;

  [[nodiscard]] float preferredWidth() const override;
  [[nodiscard]] float preferredHeight() const override;
  [[nodiscard]] PanelPlacement panelPlacement() const noexcept override { return PanelPlacement::Floating; }

private:
  struct Card {
    std::uint32_t id = 0;
    std::string iconPath;
    bool iconBound = false;
    Box* slot = nullptr; // a soft square behind the fallback glyph, clear behind a real app icon
    Image* icon = nullptr;
    Glyph* fallback = nullptr;
    Button* close = nullptr;
    bool areaHovered = false;
    bool closeHovered = false;
  };

  void doLayout(Renderer& renderer, float width, float height) override;
  void doUpdate(Renderer& renderer) override;

  // Card count shown for the current history (newest first, at most kMaxCards), and how many more there are.
  [[nodiscard]] std::size_t shownCount() const;
  [[nodiscard]] std::size_t hiddenCount() const;
  void rebuildNotifications();
  // `active`: still on screen as a live notification; `pending`: expired with actions, still answerable.
  void buildCard(const Notification& notification, bool active, bool pending);
  void rebuildCalendar();
  void rebuildWeather();
  void activate(std::uint32_t id, bool active, bool pending);
  void dismiss(std::uint32_t id, bool active);
  void clearAll();
  // The × shows while the pointer is over the card or the × itself (the dispatcher hovers only the deepest area).
  void setCardHover(std::size_t index, std::optional<bool> area, std::optional<bool> close);
  // Rebuilds the cards and resizes the panel on the next loop iteration (history changed while open).
  void queueRefresh();
  [[nodiscard]] std::string resolveIconPath(const Notification& notification);

  NotificationCenterServices m_services;
  IconResolver m_iconResolver;
  Flex* m_rootLayout = nullptr;
  Button* m_clearButton = nullptr;
  Flex* m_list = nullptr;
  Label* m_more = nullptr;
  InputArea* m_moreArea = nullptr;
  Flex* m_calendar = nullptr;
  InputArea* m_weatherArea = nullptr;
  Box* m_weatherSky = nullptr;
  Flex* m_weatherContent = nullptr;
  std::vector<Card> m_cards;
  std::uint64_t m_shownSerial = 0;
  int m_eventToken = -1;
  bool m_refreshQueued = false;
};
