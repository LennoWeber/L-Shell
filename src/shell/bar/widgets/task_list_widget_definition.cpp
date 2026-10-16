// Task List Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/task_list_widget.h"

namespace lshell {

bar_widget_definition(TaskListWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Task List",
    category: "Productivity",
    description: "To-do list with progress tracking",
    icon: "x-office-spreadsheet",
    default_config: {
        std::string name = "task_list";
        std::string type = "task_list";
        bool show_icon = true;
        bool show_progress = true;
        bool show_count = true;
        bool show_due_today = false;
    }
)

} // namespace lshell
