// Disk Monitor Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/disk_monitor_widget.h"

namespace lshell {

bar_widget_definition(DiskMonitorWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Disk Monitor",
    category: "System Monitoring",
    description: "Displays disk usage, read/write speeds, and temperature",
    icon: "drive-harddisk",
    default_config: {
        std::string name = "disk_monitor";
        std::string type = "disk_monitor";
        bool show_icon = true;
        bool show_usage = true;
        bool show_speed = true;
        bool show_temperature = false;
        bool show_multiple_disks = false;
    }
)

} // namespace lshell
