// CPU Monitor Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/cpu_monitor_widget.h"

namespace lshell {

bar_widget_definition(CPUMonitorWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "CPU Monitor",
    category: "System Monitoring",
    description: "Displays CPU usage, temperature, and frequency",
    icon: "utilities-system-monitor",
    default_config: {
        std::string name = "cpu_monitor";
        std::string type = "cpu_monitor";
        bool show_icon = true;
        bool show_overall = true;
        bool show_per_core = false;
        bool show_temperature = true;
        bool show_frequency = false;
    }
)

} // namespace lshell
