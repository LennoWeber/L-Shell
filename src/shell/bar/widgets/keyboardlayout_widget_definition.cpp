// KeyboardLayout Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/keyboardlayout_widget.h"

namespace lshell {

bar_widget_definition(KeyboardLayoutWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Keyboard Layout",
    category: "Input",
    description: "Displays current keyboard layout and allows switching",
    icon: "input-keyboard-symbolic",
    default_config: {
        std::string name = "keyboardlayout";
        std::string type = "keyboardlayout";
        bool show_icon = true;
        bool show_name = true;
        bool cycle_on_click = true;
        std::vector<std::string> layouts = {};
    }
)

} // namespace lshell
