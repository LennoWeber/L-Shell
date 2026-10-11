#include "shell/notification_center/notification_center_panel.h"

#include "compositors/compositor_platform.h"
#include "core/deferred_call.h"
#include "core/log.h"
#include "i18n/i18n.h"
#include "net/uri.h"
#include "notification/notification.h"
#include "notification/notification_display_name.h"
#include "notification/notification_manager.h"
#include "render/core/render_styles.h"
#include "render/scene/input_area.h"
#include "shell/panel/panel_manager.h"
#include "system/weather_service.h"
#include "time/time_format.h"
#include "ui/builders.h"
#include "ui/controls/box.h"
#include "ui/controls/button.h"
#include "ui/controls/flex.h"
#include "ui/controls/glyph.h"
#include "ui/controls/image.h"
#include "ui/controls/label.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <unistd.h>
#include <utility>

namespace {

  constexpr Logger kLog("notification-center");

  // Logical px, measured against the macOS Tahoe Notification Center.
  constexpr float kContentWidth = 344.0F;
  constexpr float kHeaderHeight = 28.0F;
  constexpr float kSectionGap = 12.0F;
  constexpr float kCardHeight = 72.0F;
  constexpr float kCardGap = 8.0F;
  constexpr float kCardRadius = 18.0F;
  constexpr float kCardPaddingH = 12.0F;
  constexpr float kIconSize = 36.0F;
  constexpr float kIconRadius = 9.0F;
  constexpr float kCloseSize = 20.0F;
  constexpr float kEmptyHeight = 64.0F;
  constexpr float kMoreHeight = 22.0F;
  constexpr float kWidgetGap = 12.0F;
  constexpr float kWidgetPadding = 12.0F;
  constexpr float kCalendarRowHeight = 17.0F;
  constexpr float kWeatherTempSize = 38.0F;
  constexpr std::size_t kMaxCards = 4;

  [[nodiscard]] float widgetSize() { return (kContentWidth - kWidgetGap) * 0.5F; }

  [[nodiscard]] ColorSpec white(float alpha) { return fixedColorSpec(rgba(1.0F, 1.0F, 1.0F, alpha)); }

  // "now", "5m", "2h", then the date: the macOS notification time stamps.
  [[nodiscard]] std::string relativeTime(const Notification& notification) {
    if (!notification.receivedWallClock.has_value()) {
      return {};
    }
    using namespace std::chrono;
    const auto age = duration_cast<minutes>(system_clock::now() - *notification.receivedWallClock);
    if (age.count() < 1) {
      return i18n::tr("notification-center.now");
    }
    if (age.count() < 60) {
      return std::format("{}m", age.count());
    }
    if (age.count() < 24 * 60) {
      return std::format("{}h", age.count() / 60);
    }
    const auto seconds = duration_cast<std::chrono::seconds>(notification.receivedWallClock->time_since_epoch());
    return formatLocalUnixTime(seconds.count(), "%-d %b");
  }

  // The first character of a (possibly multi-byte) UTF-8 string: weekday initials for the month widget.
  [[nodiscard]] std::string firstCharacter(const std::string& text) {
    if (text.empty()) {
      return text;
    }
    const auto lead = static_cast<unsigned char>(text.front());
    std::size_t length = 1;
    if ((lead & 0xE0U) == 0xC0U) {
      length = 2;
    } else if ((lead & 0xF0U) == 0xE0U) {
      length = 3;
    } else if ((lead & 0xF8U) == 0xF0U) {
      length = 4;
    }
    return text.substr(0, std::min(length, text.size()));
  }

  [[nodiscard]] std::string upperAscii(std::string text) {
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
  }

  [[nodiscard]] std::tm localNow() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return local;
  }

  [[nodiscard]] int daysInMonth(int year, int month0) {
    const std::chrono::year_month_day_last last{
        std::chrono::year{year}, std::chrono::month_day_last{std::chrono::month{static_cast<unsigned>(month0 + 1)}}
    };
    return static_cast<int>(static_cast<unsigned>(last.day()));
  }

  // A clear sky by day, a deep blue one at night: the macOS weather widget's backdrop.
  [[nodiscard]] RoundedRectStyle skyStyle(bool day, float radius) {
    const Color top = day ? rgba(0.24F, 0.51F, 0.86F, 1.0F) : rgba(0.09F, 0.13F, 0.27F, 1.0F);
    const Color bottom = day ? rgba(0.47F, 0.71F, 0.93F, 1.0F) : rgba(0.21F, 0.28F, 0.45F, 1.0F);
    return RoundedRectStyle{
        .fill = top,
        .border = rgba(0.0F, 0.0F, 0.0F, 0.0F),
        .fillMode = FillMode::LinearGradient,
        .gradientDirection = GradientDirection::Vertical,
        .gradientStops =
            {GradientStop{0.0F, top}, GradientStop{1.0F, bottom}, GradientStop{1.0F, bottom},
             GradientStop{1.0F, bottom}},
        .radius = Radii{radius},
        .softness = 1.0F,
    };
  }

  void openControlCenter(std::string_view tab) {
    // Deferred: opening another panel replaces this one, which must not happen inside its own click handler.
    DeferredCall::callLater([tab]() {
      PanelManager::instance().openPanel("control-center", PanelOpenRequest{.context = tab});
    });
  }

} // namespace

NotificationCenterPanel::NotificationCenterPanel(NotificationCenterServices services) : m_services(services) {}

NotificationCenterPanel::~NotificationCenterPanel() {
  if (m_services.notifications != nullptr && m_eventToken >= 0) {
    m_services.notifications->removeEventCallback(m_eventToken);
  }
}

std::size_t NotificationCenterPanel::shownCount() const {
  if (m_services.notifications == nullptr) {
    return 0;
  }
  return std::min(m_services.notifications->history().size(), kMaxCards);
}

std::size_t NotificationCenterPanel::hiddenCount() const {
  if (m_services.notifications == nullptr) {
    return 0;
  }
  return m_services.notifications->history().size() - shownCount();
}

float NotificationCenterPanel::preferredWidth() const { return scaled(kContentWidth + (Style::panelPadding * 2.0F)); }

float NotificationCenterPanel::preferredHeight() const {
  const std::size_t cards = shownCount();
  float list = kEmptyHeight;
  if (cards > 0) {
    list = (kCardHeight * static_cast<float>(cards)) + (kCardGap * static_cast<float>(cards - 1));
    if (hiddenCount() > 0) {
      list += kCardGap + kMoreHeight;
    }
  }
  const float content = kHeaderHeight + kSectionGap + list + kSectionGap + widgetSize();
  return std::ceil(scaled(content + (Style::panelPadding * 2.0F)));
}

void NotificationCenterPanel::create() {
  const float scale = contentScale();

  auto header = ui::row(
      {.align = FlexAlign::Center, .gap = Style::spaceSm * scale, .height = kHeaderHeight * scale},
      ui::label({
          .text = i18n::tr("notification-center.title"),
          .fontSize = Style::fontSizeTitle * scale,
          .fontWeight = FontWeight::SemiBold,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .flexGrow = 1.0F,
      }),
      ui::button({
          .out = &m_clearButton,
          .text = i18n::tr("notification-center.clear"),
          .fontSize = Style::fontSizeCaption * scale,
          .variant = ButtonVariant::Ghost,
          .paddingV = Style::spaceXs * scale,
          .paddingH = Style::spaceSm * scale,
          .radius = Style::scaledRadiusLg(scale),
          .onClick = [this]() { clearAll(); },
      })
  );

  auto list = ui::column({.out = &m_list, .align = FlexAlign::Stretch, .gap = kCardGap * scale});

  const float widget = widgetSize() * scale;
  auto calendarArea = ui::inputArea({
      .width = widget,
      .height = widget,
      .onClick = [](const InputArea::PointerData& /*data*/) { openControlCenter("calendar"); },
  });
  auto calendar = ui::column({
      .out = &m_calendar,
      .align = FlexAlign::Stretch,
      .gap = 0.0F,
      .padding = kWidgetPadding * scale,
      .width = widget,
      .height = widget,
  });
  calendarArea->addChild(std::move(calendar));

  auto weatherArea = ui::inputArea({
      .out = &m_weatherArea,
      .width = widget,
      .height = widget,
      .onClick = [](const InputArea::PointerData& /*data*/) { openControlCenter("weather"); },
  });
  weatherArea->addChild(ui::box({.out = &m_weatherSky, .width = widget, .height = widget}));
  weatherArea->addChild(
      ui::column({
          .out = &m_weatherContent,
          .align = FlexAlign::Start,
          .gap = 0.0F,
          .padding = kWidgetPadding * scale,
          .width = widget,
          .height = widget,
      })
  );

  auto widgets =
      ui::row({.align = FlexAlign::Start, .gap = kWidgetGap * scale}, std::move(calendarArea), std::move(weatherArea));

  auto root = ui::column(
      {.out = &m_rootLayout, .align = FlexAlign::Stretch, .gap = kSectionGap * scale}, std::move(header),
      std::move(list), std::move(widgets)
  );
  setRoot(std::move(root));
  if (m_animations != nullptr) {
    this->root()->setAnimationManager(m_animations);
  }
}

void NotificationCenterPanel::onOpen(std::string_view /*context*/) {
  if (m_services.notifications != nullptr) {
    m_services.notifications->markNotificationHistorySeen();
    if (m_eventToken < 0) {
      m_eventToken = m_services.notifications->addEventCallback(
          [this](const Notification& /*notification*/, NotificationEvent /*event*/) { queueRefresh(); }
      );
    }
  }
  rebuildNotifications();
  rebuildCalendar();
  rebuildWeather();
}

void NotificationCenterPanel::onClose() {
  if (m_services.notifications != nullptr && m_eventToken >= 0) {
    m_services.notifications->removeEventCallback(m_eventToken);
    m_eventToken = -1;
  }
  m_rootLayout = nullptr;
  m_clearButton = nullptr;
  m_list = nullptr;
  m_more = nullptr;
  m_moreArea = nullptr;
  m_calendar = nullptr;
  m_weatherArea = nullptr;
  m_weatherSky = nullptr;
  m_weatherContent = nullptr;
  m_cards.clear();
  m_shownSerial = 0;
  clearReleasedRoot();
}

void NotificationCenterPanel::queueRefresh() {
  // Deferred and coalesced: history changes arrive inside a card's own click handler (dismiss, activate), which
  // must not destroy the card it runs in.
  if (m_refreshQueued) {
    return;
  }
  m_refreshQueued = true;
  DeferredCall::callLater([this]() {
    m_refreshQueued = false;
    if (m_list == nullptr || !PanelManager::instance().isOpenPanel(kPanelId)) {
      return;
    }
    rebuildNotifications();
    // The height follows the card count.
    PanelManager::instance().relayoutActivePanelPreferredSize();
    PanelManager::instance().refresh();
  });
}

std::string NotificationCenterPanel::resolveIconPath(const Notification& notification) {
  const int target = static_cast<int>(std::round(kIconSize * contentScale() * 2.0F));
  const auto resolveName = [this, target](const std::string& value) -> std::string {
    if (value.empty() || uri::isRemoteUrl(value)) {
      return {};
    }
    const std::string local = uri::normalizeFileUrl(value);
    if (!local.empty() && local.front() == '/') {
      return access(local.c_str(), R_OK) == 0 ? local : std::string();
    }
    return m_iconResolver.resolve(local, target);
  };
  if (notification.icon.has_value()) {
    if (std::string path = resolveName(*notification.icon); !path.empty()) {
      return path;
    }
  }
  // Most apps send no icon of their own: their desktop entry or app name names the app icon.
  if (notification.desktopEntry.has_value()) {
    if (std::string path = resolveName(*notification.desktopEntry); !path.empty()) {
      return path;
    }
  }
  std::string appName = notification.appName;
  std::ranges::transform(appName, appName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return resolveName(appName);
}

void NotificationCenterPanel::rebuildNotifications() {
  if (m_list == nullptr || m_services.notifications == nullptr) {
    return;
  }
  const std::uint64_t serial = m_services.notifications->changeSerial();
  if (serial == m_shownSerial && !m_cards.empty()) {
    return;
  }
  m_shownSerial = serial;
  const float scale = contentScale();

  while (!m_list->children().empty()) {
    (void)m_list->removeChild(m_list->children().back().get());
  }
  m_cards.clear();
  m_more = nullptr;
  m_moreArea = nullptr;

  const auto& history = m_services.notifications->history();
  if (m_clearButton != nullptr) {
    m_clearButton->setVisible(!history.empty());
  }
  if (history.empty()) {
    m_list->addChild(
        ui::column(
            {.align = FlexAlign::Center, .justify = FlexJustify::Center, .height = kEmptyHeight * scale},
            ui::label({
                .text = i18n::tr("notification-center.empty"),
                .fontSize = Style::fontSizeBody * scale,
                .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
            })
        )
    );
    return;
  }

  std::size_t shown = 0;
  for (const auto& entry : std::views::reverse(history)) {
    if (shown == kMaxCards) {
      break;
    }
    buildCard(entry.notification, entry.active && m_services.notifications->hasPendingDBusClose(entry.notification.id));
    ++shown;
  }
  if (const std::size_t hidden = history.size() - shown; hidden > 0) {
    auto more = ui::inputArea({
        .out = &m_moreArea,
        .height = kMoreHeight * scale,
        .onClick = [](const InputArea::PointerData& /*data*/) { openControlCenter("notifications"); },
    });
    more->addChild(
        ui::label({
            .out = &m_more,
            .text = i18n::tr("notification-center.more", "count", std::to_string(hidden)),
            .fontSize = Style::fontSizeCaption * scale,
            .fontWeight = FontWeight::Medium,
            .color = colorSpecFromRole(ColorRole::Primary),
        })
    );
    m_list->addChild(std::move(more));
  }
}

void NotificationCenterPanel::buildCard(const Notification& notification, bool active) {
  const float scale = contentScale();
  const float width = kContentWidth * scale;
  const float height = kCardHeight * scale;
  const float icon = kIconSize * scale;
  const float close = kCloseSize * scale;
  const float gap = Style::spaceSm * scale;
  const float textWidth = width - (2.0F * kCardPaddingH * scale) - icon - close - (2.0F * gap);
  const std::uint32_t id = notification.id;
  const std::size_t index = m_cards.size();

  Card& card = m_cards.emplace_back();
  card.id = id;
  card.active = active;
  card.iconPath = resolveIconPath(notification);

  auto area = ui::inputArea({
      .width = width,
      .height = height,
      .onEnter =
          [this, index](const InputArea::PointerData& /*data*/) {
            if (index < m_cards.size() && m_cards[index].close != nullptr) {
              m_cards[index].close->setVisible(true);
              PanelManager::instance().refresh();
            }
          },
      .onLeave =
          [this, index]() {
            if (index < m_cards.size() && m_cards[index].close != nullptr) {
              m_cards[index].close->setVisible(false);
              PanelManager::instance().refresh();
            }
          },
      .onClick = [this, id, active](const InputArea::PointerData& /*data*/) { activate(id, active); },
  });

  auto iconSlot = ui::box({
      .out = &card.slot,
      .fill = colorSpecFromRole(ColorRole::OnSurface, 0.08F),
      .radius = kIconRadius * scale,
      .width = icon,
      .height = icon,
  });
  iconSlot->addChild(
      ui::image({
          .out = &card.icon,
          .fit = ImageFit::Contain,
          .radius = kIconRadius * scale,
          .width = icon,
          .height = icon,
          .visible = false,
      })
  );
  const float fallbackSize = icon * 0.55F;
  iconSlot->addChild(
      ui::glyph({
          .out = &card.fallback,
          .glyph = "bell",
          .glyphSize = fallbackSize,
          .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
      })
  );
  card.fallback->setPosition(std::round((icon - fallbackSize) * 0.5F), std::round((icon - fallbackSize) * 0.5F));

  auto text = ui::column(
      {.align = FlexAlign::Stretch, .gap = 1.0F * scale, .flexGrow = 1.0F},
      ui::row(
          {.align = FlexAlign::Center, .gap = gap},
          ui::label({
              .text = notificationDisplayAppName(notification),
              .fontSize = Style::fontSizeCaption * scale,
              .fontWeight = FontWeight::Medium,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .maxWidth = textWidth * 0.7F,
              .maxLines = 1,
              .ellipsize = TextEllipsize::End,
              .flexGrow = 1.0F,
          }),
          ui::label({
              .text = relativeTime(notification),
              .fontSize = Style::fontSizeCaption * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .maxLines = 1,
          })
      ),
      ui::label({
          .text = notification.summary,
          .fontSize = Style::fontSizeBody * scale,
          .fontWeight = FontWeight::SemiBold,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxWidth = textWidth,
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
      }),
      ui::label({
          .text = notification.body,
          .fontSize = Style::fontSizeCaption * scale,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxWidth = textWidth,
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
          .visible = !notification.body.empty(),
      })
  );

  auto dismissButton = ui::button({
      .out = &card.close,
      .glyph = "close",
      .glyphSize = Style::fontSizeCaption * scale,
      .variant = ButtonVariant::Ghost,
      .padding = 0.0F,
      .radius = close * 0.5F,
      .width = close,
      .height = close,
      .visible = false,
      .onClick = [this, id, active]() { dismiss(id, active); },
  });

  auto row = ui::row(
      {
          .align = FlexAlign::Center,
          .gap = gap,
          .paddingH = kCardPaddingH * scale,
          .width = width,
          .height = height,
      },
      std::move(iconSlot), std::move(text), std::move(dismissButton)
  );
  row->setCardStyle(scale, panelCardOpacity());
  row->setRadius(kCardRadius * scale);
  area->addChild(std::move(row));
  m_list->addChild(std::move(area));
}

void NotificationCenterPanel::rebuildCalendar() {
  if (m_calendar == nullptr) {
    return;
  }
  const float scale = contentScale();
  while (!m_calendar->children().empty()) {
    (void)m_calendar->removeChild(m_calendar->children().back().get());
  }
  // Styled on open, when the panel's card opacity (glass or solid) is known.
  m_calendar->setCardStyle(scale, panelCardOpacity());
  m_calendar->setRadius(kCardRadius * scale);

  const std::tm now = localNow();
  const int year = now.tm_year + 1900;
  const int month = now.tm_mon;
  const int today = now.tm_mday;
  const int firstWeekday = ((now.tm_wday - ((today - 1) % 7)) + 7) % 7; // 0 = Sunday
  const int weekStart = std::clamp(localeFirstDayOfWeek(), 0, 6);
  const int leading = (firstWeekday - weekStart + 7) % 7;
  const int days = daysInMonth(year, month);

  const float cell = (widgetSize() - (2.0F * kWidgetPadding)) / 7.0F * scale;
  const float rowHeight = kCalendarRowHeight * scale;
  const float dayFont = Style::fontSizeMini * scale;

  m_calendar->addChild(
      ui::label({
          .text = upperAscii(formatLocalTime("%B")),
          .fontSize = Style::fontSizeCaption * scale,
          .fontWeight = FontWeight::Bold,
          .color = colorSpecFromRole(ColorRole::Primary),
          .maxLines = 1,
      })
  );
  m_calendar->addChild(ui::row({.height = 4.0F * scale}));

  // Weekday initials, starting on the locale's first day; 4 Jan 1970 was a Sunday.
  auto weekdays = ui::row({.align = FlexAlign::Center, .gap = 0.0F, .height = rowHeight});
  for (int i = 0; i < 7; ++i) {
    const std::int64_t sunday = 3 * 86400;
    const std::int64_t stamp = sunday + (static_cast<std::int64_t>((weekStart + i) % 7) * 86400) + 43200;
    weekdays->addChild(
        ui::row(
            {.align = FlexAlign::Center, .justify = FlexJustify::Center, .width = cell, .height = rowHeight},
            ui::label({
                .text = firstCharacter(formatLocalUnixTime(stamp, "%a")),
                .fontSize = dayFont,
                .fontWeight = FontWeight::SemiBold,
                .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
            })
        )
    );
  }
  m_calendar->addChild(std::move(weekdays));

  int day = 1 - leading;
  for (int week = 0; week < 6 && day <= days; ++week) {
    auto row = ui::row({.align = FlexAlign::Center, .gap = 0.0F, .height = rowHeight});
    for (int i = 0; i < 7; ++i, ++day) {
      const bool inMonth = day >= 1 && day <= days;
      const bool isToday = day == today;
      auto slot = ui::row({
          .align = FlexAlign::Center,
          .justify = FlexJustify::Center,
          .width = cell,
          .height = rowHeight,
      });
      if (inMonth) {
        auto number = ui::row(
            {
                .align = FlexAlign::Center,
                .justify = FlexJustify::Center,
                .fill = isToday ? colorSpecFromRole(ColorRole::Primary) : clearColorSpec(),
                .radius = rowHeight * 0.5F,
                .width = rowHeight,
                .height = rowHeight,
            },
            ui::label({
                .text = std::to_string(day),
                .fontSize = dayFont,
                .fontWeight = isToday ? FontWeight::Bold : FontWeight::Medium,
                .color = colorSpecFromRole(isToday ? ColorRole::OnPrimary : ColorRole::OnSurface),
                .textAlign = TextAlign::Center,
            })
        );
        slot->addChild(std::move(number));
      }
      row->addChild(std::move(slot));
    }
    m_calendar->addChild(std::move(row));
  }
}

void NotificationCenterPanel::rebuildWeather() {
  if (m_weatherContent == nullptr || m_weatherSky == nullptr) {
    return;
  }
  const float scale = contentScale();
  while (!m_weatherContent->children().empty()) {
    (void)m_weatherContent->removeChild(m_weatherContent->children().back().get());
  }

  const WeatherService* weather = m_services.weather;
  const bool ready = weather != nullptr && weather->enabled() && weather->hasData();
  const bool day = !ready || weather->snapshot().current.isDay;
  m_weatherSky->setStyle(skyStyle(day, kCardRadius * scale));

  std::string location = ready ? weather->snapshot().locationName : std::string();
  if (location.empty()) {
    location = i18n::tr("notification-center.weather");
  }
  std::string temperature = "--";
  std::string glyph = "cloud";
  std::string condition = i18n::tr("notification-center.weather-unavailable");
  std::string highLow;
  if (ready) {
    const auto& snapshot = weather->snapshot();
    const auto degrees = [weather](double celsius) {
      return std::format("{}°", std::lround(weather->displayTemperature(celsius)));
    };
    temperature = degrees(snapshot.current.temperatureC);
    glyph = WeatherService::glyphForCode(snapshot.current.weatherCode, snapshot.current.isDay);
    condition = WeatherService::shortDescriptionForCode(snapshot.current.weatherCode);
    if (!snapshot.forecastDays.empty()) {
      const auto& today = snapshot.forecastDays.front();
      highLow = i18n::tr(
          "notification-center.high-low", "high", degrees(today.temperatureMaxC), "low", degrees(today.temperatureMinC)
      );
    }
  }

  const float innerWidth = (widgetSize() - (2.0F * kWidgetPadding)) * scale;
  m_weatherContent->addChild(
      ui::row(
          {.align = FlexAlign::Center, .gap = Style::spaceXs * scale},
          ui::label({
              .text = location,
              .fontSize = Style::fontSizeCaption * scale,
              .fontWeight = FontWeight::SemiBold,
              .color = white(1.0F),
              .maxWidth = innerWidth - (Style::fontSizeCaption * scale) - (Style::spaceXs * scale),
              .maxLines = 1,
              .ellipsize = TextEllipsize::End,
          }),
          ui::glyph({
              .glyph = "navigation-filled",
              .glyphSize = Style::fontSizeMini * scale,
              .color = white(1.0F),
              .visible = ready,
          })
      )
  );
  m_weatherContent->addChild(
      ui::label({
          .text = temperature,
          .fontSize = kWeatherTempSize * scale,
          .fontWeight = FontWeight::Light,
          .color = white(1.0F),
          .maxLines = 1,
      })
  );
  m_weatherContent->addChild(ui::spacer());
  m_weatherContent->addChild(ui::glyph({.glyph = glyph, .glyphSize = 16.0F * scale, .color = white(1.0F)}));
  m_weatherContent->addChild(
      ui::label({
          .text = condition,
          .fontSize = Style::fontSizeCaption * scale,
          .fontWeight = FontWeight::SemiBold,
          .color = white(1.0F),
          .maxWidth = innerWidth,
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
      })
  );
  m_weatherContent->addChild(
      ui::label({
          .text = highLow,
          .fontSize = Style::fontSizeCaption * scale,
          .color = white(0.85F),
          .maxLines = 1,
          .visible = !highLow.empty(),
      })
  );
}

void NotificationCenterPanel::doLayout(Renderer& renderer, float width, float height) {
  if (m_rootLayout == nullptr) {
    return;
  }
  // App icons load here, where a renderer is at hand; each card binds its icon once.
  for (Card& card : m_cards) {
    if (card.iconBound || card.icon == nullptr || card.fallback == nullptr) {
      continue;
    }
    card.iconBound = true;
    const int target = static_cast<int>(std::round(kIconSize * contentScale() * 2.0F));
    const bool loaded = !card.iconPath.empty() && card.icon->setSourceFile(renderer, card.iconPath, target, true);
    card.icon->setVisible(loaded);
    card.fallback->setVisible(!loaded);
    if (loaded && card.slot != nullptr) {
      card.slot->setFill(clearColorSpec());
    }
  }
  m_rootLayout->setSize(width, height);
  m_rootLayout->layout(renderer);
}

void NotificationCenterPanel::doUpdate(Renderer& /*renderer*/) {}

void NotificationCenterPanel::activate(std::uint32_t id, bool active) {
  NotificationManager* notifications = m_services.notifications;
  if (notifications == nullptr) {
    return;
  }
  // Like macOS: a click opens the app (its default action) and the notification is done.
  if (active && notifications->hasPendingDBusClose(id)) {
    const std::string token = m_services.platform != nullptr
        ? m_services.platform->requestActivationToken(m_services.platform->lastPointerSurface())
        : std::string{};
    if (!notifications->invokeAction(id, "default", token, true)) {
      kLog.debug("notification #{} has no default action", id);
      (void)notifications->close(id, CloseReason::Dismissed);
    }
  }
  notifications->removeHistoryEntry(id);
  DeferredCall::callLater([]() { PanelManager::instance().close(); });
}

void NotificationCenterPanel::dismiss(std::uint32_t id, bool active) {
  NotificationManager* notifications = m_services.notifications;
  if (notifications == nullptr) {
    return;
  }
  if (active) {
    (void)notifications->close(id, CloseReason::Dismissed);
  }
  // Removing a closed entry sends no event, so ask for the refresh here too.
  notifications->removeHistoryEntry(id);
  queueRefresh();
}

void NotificationCenterPanel::clearAll() {
  NotificationManager* notifications = m_services.notifications;
  if (notifications == nullptr) {
    return;
  }
  std::vector<std::uint32_t> activeIds;
  activeIds.reserve(notifications->all().size());
  for (const auto& notification : notifications->all()) {
    activeIds.push_back(notification.id);
  }
  for (const std::uint32_t id : activeIds) {
    (void)notifications->close(id, CloseReason::Dismissed);
  }
  notifications->clearHistory();
  queueRefresh();
}
