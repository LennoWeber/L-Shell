// Quick Notes Widget Definition
// Auto-generated widget definition for registration

#include "shell/bar/widget_definition.h"
#include "shell/bar/widgets/quick_notes_widget.h"

namespace lshell {

bar_widget_definition(QuickNotesWidget,
    domain: "lshell",
    translation_domain: "lshell",
    name: "Quick Notes",
    category: "Productivity",
    description: "Quick text notes with timestamp",
    icon: "x-office-document",
    default_config: {
        std::string name = "quick_notes";
        std::string type = "quick_notes";
        bool show_icon = true;
        bool show_latest_note = true;
        bool show_count = true;
        bool show_timestamp = false;
        int max_notes = 10;
    }
)

} // namespace lshell
