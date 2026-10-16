// Alarm Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/alarm_widget.h"

namespace lshell {

bar_widget_definition(AlarmWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Alarm & Timer",
    category: "Productivity",
    description: "Displays alarms, timers, and stopwatch",
    icon: "media-playlist-concrete",
    default_config: {
        std::string name = "alarm";
        std::string type = "alarm";
        bool show_icon = true;
        bool show_next_alarm = true;
        bool show_timer = true;
        bool show_stopwatch = false;
    }
)

} // namespace lshell
