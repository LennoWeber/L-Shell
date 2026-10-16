#include "shell/bar/widgets/clock_widget_definition.h"

const noctalia::bar::WidgetDefinition<ClockWidget::Options>& clockWidgetDefinition() {
  using noctalia::bar::field;
  using Options = ClockWidget::Options;

  static const noctalia::bar::WidgetDefinition<Options> definition{
      .type = "clock",
      .fields = {
          field<&Options::format>({
              .key = "format",
          }),
          field<&Options::verticalFormat>({
              .key = "vertical_format",
          }),
          field<&Options::tooltipFormat>({
              .key = "tooltip_format",
          }),
          field<&Options::timezone>({
              .key = "timezone",
          }),
          field<&Options::uppercase>({
              .key = "uppercase",
          }),
          field<&Options::letterSpacing>({
              .key = "letter_spacing",
              .minValue = -0.1,
              .maxValue = 0.5,
              .step = 0.01,
          }),
          field<&Options::lineHeight>({
              .key = "line_height",
              .minValue = 0.0,
              .maxValue = 3.0,
              .step = 0.05,
          }),
      },
  };
  return definition;
}
