// PowerProfile Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/powerprofile_widget.h"

namespace lshell {

bar_widget_definition(PowerProfileWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Power Profile",
    category: "System",
    description: "Displays and controls power profile settings",
    icon: "power-profile-symbolic",
    default_config: {
        std::string name = "powerprofile";
        std::string type = "powerprofile";
        bool show_icon = true;
        bool show_mode = true;
        bool cycle_on_click = true;
        std::string default_profile = "balanced";
    }
)

} // namespace lshell
