// RAM Monitor Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/ram_monitor_widget.h"

namespace lshell {

bar_widget_definition(RAMMonitorWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "RAM Monitor",
    category: "System Monitoring",
    description: "Displays RAM and swap usage with memory pressure warnings",
    icon: "utilities-system-monitor",
    default_config: {
        std::string name = "ram_monitor";
        std::string type = "ram_monitor";
        bool show_icon = true;
        bool show_percentage = true;
        bool show_usage = true;
        bool show_swap = true;
        bool show_pressure = false;
    }
)

} // namespace lshell
