#include "shell/expo/expo_layout.h"
#include "tests/test_check.h"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace {

  [[nodiscard]] bool overlaps(const ExpoRect& a, const ExpoRect& b) {
    return a.x < b.right() - 0.5F && b.x < a.right() - 0.5F && a.y < b.bottom() - 0.5F && b.y < a.bottom() - 0.5F;
  }

  [[nodiscard]] bool inside(const ExpoRect& rect, float width, float height) {
    return rect.x >= -0.5F && rect.y >= -0.5F && rect.right() <= width + 0.5F && rect.bottom() <= height + 0.5F;
  }

  void checkLayout(const ExpoLayoutInput& input) {
    const ExpoLayout layout = computeExpoLayout(input);
    TEST_CHECK(layout.sections.size() == input.sections.size());
    std::vector<ExpoRect> all;
    for (std::size_t i = 0; i < input.sections.size(); ++i) {
      const ExpoSectionLayout& section = layout.sections[i];
      TEST_CHECK(section.cards.size() == input.sections[i].cardCount);
      for (const ExpoRect& card : section.cards) {
        // Cards keep their monitor's shape and, caption included, stay on screen.
        TEST_CHECK(std::abs((card.width / card.height) - input.sections[i].aspect) < 0.05F);
        ExpoRect withCaption = card;
        withCaption.height += layout.captionGap + layout.captionHeight;
        TEST_CHECK(inside(withCaption, input.screenWidth, input.screenHeight));
        for (const ExpoRect& other : all) {
          TEST_CHECK(!overlaps(withCaption, other));
        }
        all.push_back(withCaption);
      }
      if (input.showHeaders && !section.cards.empty()) {
        TEST_CHECK(section.header.width > 0.0F);
        TEST_CHECK(section.header.bottom() <= section.cards.front().y);
      }
    }
  }

} // namespace

int main() {
  // One monitor, a handful of desktops: one row, cards as large as the screen allows.
  for (std::size_t count = 1; count <= 9; ++count) {
    checkLayout(
        ExpoLayoutInput{
            .screenWidth = 1440.0F,
            .screenHeight = 900.0F,
            .scale = 1.0F,
            .showHeaders = false,
            .sections = {ExpoSectionInput{.cardCount = count, .aspect = 1.6F}},
        }
    );
  }

  // A lone desktop does not fill the screen.
  {
    const ExpoLayout layout = computeExpoLayout(
        ExpoLayoutInput{
            .screenWidth = 1920.0F,
            .screenHeight = 1080.0F,
            .sections = {ExpoSectionInput{.cardCount = 1, .aspect = 16.0F / 9.0F}},
        }
    );
    TEST_CHECK(layout.sections.front().cards.front().width <= 1920.0F * 0.5F + 0.5F);
    TEST_CHECK(layout.sections.front().cards.front().width >= 1920.0F * 0.4F);
  }

  // Two desktops sit side by side rather than stacked.
  {
    const ExpoLayout layout = computeExpoLayout(
        ExpoLayoutInput{
            .screenWidth = 1920.0F,
            .screenHeight = 1080.0F,
            .sections = {ExpoSectionInput{.cardCount = 2, .aspect = 16.0F / 9.0F}},
        }
    );
    const auto& cards = layout.sections.front().cards;
    TEST_CHECK(std::abs(cards[0].y - cards[1].y) < 0.5F);
  }

  // Two monitors with different shapes and a portrait one, with section headers.
  checkLayout(
      ExpoLayoutInput{
          .screenWidth = 2560.0F,
          .screenHeight = 1440.0F,
          .scale = 1.25F,
          .showHeaders = true,
          .sections = {
              ExpoSectionInput{.cardCount = 4, .aspect = 16.0F / 9.0F},
              ExpoSectionInput{.cardCount = 3, .aspect = 9.0F / 16.0F}
          },
      }
  );
  checkLayout(
      ExpoLayoutInput{
          .screenWidth = 1280.0F,
          .screenHeight = 800.0F,
          .scale = 1.0F,
          .showHeaders = true,
          .sections = {
              ExpoSectionInput{.cardCount = 10, .aspect = 1.6F}, ExpoSectionInput{.cardCount = 0, .aspect = 1.6F},
              ExpoSectionInput{.cardCount = 7, .aspect = 1.6F}
          },
      }
  );

  // Nothing to show.
  TEST_CHECK(computeExpoLayout(ExpoLayoutInput{.screenWidth = 800.0F, .screenHeight = 600.0F}).sections.empty());

  // Real geometry maps proportionally; a border poking slightly past the monitor is clipped to the card.
  {
    const std::vector<ExpoWindowGeometry> windows{
        {.x = 0.0F, .y = 0.0F, .width = 960.0F, .height = 1080.0F},
        {.x = 960.0F, .y = 540.0F, .width = 960.0F, .height = 540.0F},
        {.x = 1800.0F, .y = -10.0F, .width = 140.0F, .height = 300.0F},
    };
    const auto rects = placeExpoWindows(480.0F, 270.0F, 1920.0F, 1080.0F, windows, 1.0F);
    TEST_CHECK(rects.size() == 3);
    TEST_CHECK(std::abs(rects[0].width - 240.0F) < 0.5F && std::abs(rects[0].height - 270.0F) < 0.5F);
    TEST_CHECK(std::abs(rects[1].x - 240.0F) < 0.5F && std::abs(rects[1].y - 135.0F) < 0.5F);
    for (const ExpoRect& rect : rects) {
      TEST_CHECK(inside(rect, 480.0F, 270.0F));
      TEST_CHECK(rect.width >= 14.0F && rect.height >= 14.0F);
    }
  }

  // Scrolling layout: columns past the monitor stay whole and side by side instead of being clamped to slivers.
  {
    const std::vector<ExpoWindowGeometry> windows{
        {.x = 10.0F, .y = 10.0F, .width = 1900.0F, .height = 1060.0F},
        {.x = 1930.0F, .y = 10.0F, .width = 1900.0F, .height = 1060.0F},
        {.x = -1910.0F, .y = 10.0F, .width = 1900.0F, .height = 1060.0F},
    };
    const auto rects = placeExpoWindows(480.0F, 270.0F, 1920.0F, 1080.0F, windows, 1.0F);
    for (std::size_t i = 0; i < rects.size(); ++i) {
      TEST_CHECK(inside(rects[i], 480.0F, 270.0F));
      TEST_CHECK(std::abs(rects[i].width - rects[0].width) < 0.5F);
      TEST_CHECK(std::abs((rects[i].width / rects[i].height) - (1900.0F / 1060.0F)) < 0.05F);
      for (std::size_t j = i + 1; j < rects.size(); ++j) {
        TEST_CHECK(!overlaps(rects[i], rects[j]));
      }
    }
    TEST_CHECK(rects[2].x < rects[0].x && rects[0].x < rects[1].x);
  }

  // Without geometry the windows form a grid that does not overlap.
  for (std::size_t count = 1; count <= 12; ++count) {
    const std::vector<ExpoWindowGeometry> windows(count);
    const auto rects = placeExpoWindows(400.0F, 250.0F, 1600.0F, 1000.0F, windows, 1.0F);
    TEST_CHECK(rects.size() == count);
    for (std::size_t i = 0; i < count; ++i) {
      TEST_CHECK(inside(rects[i], 400.0F, 250.0F));
      for (std::size_t j = i + 1; j < count; ++j) {
        TEST_CHECK(!overlaps(rects[i], rects[j]));
      }
    }
  }

  // Spatial navigation follows rows and columns.
  {
    const std::vector<ExpoRect> grid{
        {.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 60.0F},     // 0 top left
        {.x = 140.0F, .y = 0.0F, .width = 100.0F, .height = 60.0F},   // 1 top right
        {.x = 0.0F, .y = 100.0F, .width = 100.0F, .height = 60.0F},   // 2 bottom left
        {.x = 140.0F, .y = 100.0F, .width = 100.0F, .height = 60.0F}, // 3 bottom right
        {.x = 300.0F, .y = 210.0F, .width = 40.0F, .height = 40.0F},  // 4 far away, low
    };
    TEST_CHECK(expoSpatialNeighbor(grid, 0, ExpoDirection::Right) == 1U);
    TEST_CHECK(expoSpatialNeighbor(grid, 0, ExpoDirection::Down) == 2U);
    TEST_CHECK(expoSpatialNeighbor(grid, 3, ExpoDirection::Left) == 2U);
    TEST_CHECK(expoSpatialNeighbor(grid, 3, ExpoDirection::Up) == 1U);
    TEST_CHECK(!expoSpatialNeighbor(grid, 0, ExpoDirection::Left).has_value());
    TEST_CHECK(!expoSpatialNeighbor(grid, 0, ExpoDirection::Up).has_value());
    TEST_CHECK(expoSpatialNeighbor(grid, 3, ExpoDirection::Right) == 4U);
    TEST_CHECK(!expoSpatialNeighbor(grid, 9, ExpoDirection::Right).has_value());
  }

  return EXIT_SUCCESS;
}
