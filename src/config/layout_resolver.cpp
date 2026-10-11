#include "config/layout_resolver.h"

#include <cassert>
#include <stdexcept>

namespace noctalia::config {

  BarConfig synthesizeBar(const ModeBarConfig& mode, std::string_view name, bool smartHide) {
    // Synthesized bars are addressed by name (IPC, peek, settings); a nameless or zero-height bar cannot exist.
    assert(!name.empty() && "a synthesized bar needs a name");
    assert(mode.thickness > 0 && "a mode bar needs a height");
    (void)mode;
    (void)name;
    (void)smartHide;
    throw std::logic_error("synthesizeBar: not implemented");
  }

  ResolvedLayout resolveLayout(const Config& config) {
    (void)config;
    throw std::logic_error("resolveLayout: not implemented");
  }

} // namespace noctalia::config
