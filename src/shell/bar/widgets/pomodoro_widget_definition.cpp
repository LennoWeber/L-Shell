// Pomodoro Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/pomodoro_widget.h"

namespace lshell {

bar_widget_definition(PomodoroWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Pomodoro Timer",
    category: "Productivity",
    description: "Pomodoro technique timer with work and break sessions",
    icon: "utilities-timer",
    default_config: {
        std::string name = "pomodoro";
        std::string type = "pomodoro";
        bool show_icon = true;
        bool show_timer = true;
        bool show_sessions = true;
        bool show_progress = false;
        int work_duration = 25;
        int short_break_duration = 5;
        int long_break_duration = 15;
        int sessions_before_long_break = 4;
    }
)

} // namespace lshell
