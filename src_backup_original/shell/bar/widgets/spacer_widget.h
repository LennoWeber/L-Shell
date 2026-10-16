#pragma once

#include "shell/bar/widget.h"

class Box;

class SpacerWidget : public Widget {
public:
  struct Options {
    int length = 20;
    // Draws a 1px rule across the bar in the middle of the gap, this long (0 = an empty gap). Colored by `color`.
    int ruleLength = 0;
  };

  SpacerWidget(bool verticalBar, Options options);

  void create() override;

  bool noGapAroundMe() const noexcept override { return true; }
  bool isBarClickThrough() const noexcept override { return true; }

private:
  void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
  float m_fixedLength;
  float m_ruleLength = 0.0F;
  Box* m_rule = nullptr;
  bool m_verticalBar = false;
};
