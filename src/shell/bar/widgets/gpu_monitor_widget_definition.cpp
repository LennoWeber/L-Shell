// GPU Monitor Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/gpu_monitor_widget.h"

namespace lshell {

bar_widget_definition(GPUMonitorWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "GPU Monitor",
    category: "System Monitoring",
    description: "Displays GPU usage, VRAM, temperature, and fan speed",
    icon: "video-x-generic",
    default_config: {
        std::string name = "gpu_monitor";
        std::string type = "gpu_monitor";
        bool show_icon = true;
        bool show_percentage = true;
        bool show_vram = true;
        bool show_temperature = true;
        bool show_fan_speed = false;
        bool compact_mode = true;
    }
)

} // namespace lshell
