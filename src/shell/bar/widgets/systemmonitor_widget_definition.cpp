// SystemMonitor Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/systemmonitor_widget.h"

namespace lshell {

bar_widget_definition(SystemMonitorWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "System Monitor",
    category: "System",
    description: "Displays comprehensive system monitoring information",
    icon: "system-monitor-symbolic",
    default_config: {
        std::string name = "systemmonitor";
        std::string type = "systemmonitor";
        bool show_icon = true;
        bool show_cpu = true;
        bool show_ram = true;
        bool show_disk = false;
        bool show_network = false;
        bool show_temperature = false;
        int update_interval_ms = 2000;
    }
)

} // namespace lshell
