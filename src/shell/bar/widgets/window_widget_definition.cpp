// Window Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/window_widget.h"

namespace lshell {

bar_widget_definition(WindowWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Window",
    category: "Window Management",
    description: "Displays window information and controls",
    icon: "window-symbolic",
    default_config: {
        std::string name = "window";
        std::string type = "window";
        bool show_icon = true;
        bool show_title = true;
        bool show_class = false;
        bool show_pid = false;
        int max_title_length = 30;
    }
)

} // namespace lshell
