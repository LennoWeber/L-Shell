// Workspace Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/workspace_widget.h"

namespace lshell {

bar_widget_definition(WorkspaceWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Workspace",
    category: "Window Management",
    description: "Displays current workspace and allows switching",
    icon: "workspace-symbolic",
    default_config: {
        std::string name = "workspace";
        std::string type = "workspace";
        bool show_icon = false;
        bool show_number = true;
        bool show_name = false;
        bool show_indicators = true;
        bool cycle_on_scroll = true;
        std::string naming_scheme = "numeric";
    }
)

} // namespace lshell
