#include "config/layout_resolver.h"

#include <stdexcept>

namespace noctalia::config {

  BarConfig synthesizeBar(const ModeBarConfig& mode, std::string_view name, bool smartHide) {
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
