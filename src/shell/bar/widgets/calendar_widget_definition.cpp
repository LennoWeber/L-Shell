// Calendar Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/calendar_widget.h"

namespace lshell {

bar_widget_definition(CalendarWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Calendar",
    category: "Productivity",
    description: "Displays current date and upcoming events",
    icon: "x-office-calendar",
    default_config: {
        std::string name = "calendar";
        std::string type = "calendar";
        bool show_icon = true;
        bool show_date = true;
        bool show_day = true;
        bool show_events = true;
        bool show_weekday = false;
        std::string date_format = "%d. %B %Y";
    }
)

} // namespace lshell
