#include "shell/bar/widgets/spacer_widget.h"

#include "render/scene/node.h"
#include "ui/builders.h"
#include "ui/controls/box.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <cmath>

SpacerWidget::SpacerWidget(bool verticalBar, Options options)
    : m_fixedLength(static_cast<float>(options.length)), m_ruleLength(static_cast<float>(options.ruleLength)),
      m_verticalBar(verticalBar) {}

void SpacerWidget::create() {
  auto spacer = ui::node({});
  spacer->setSize(0.0F, 0.0F);
  spacer->setHitTestVisible(false);
  if (m_ruleLength > 0.0F) {
    spacer->addChild(ui::box({.out = &m_rule, .configure = [](Box& box) { box.setHitTestVisible(false); }}));
  }
  setRoot(std::move(spacer));
}

void SpacerWidget::doLayout(Renderer& /*renderer*/, float /*containerWidth*/, float /*containerHeight*/) {
  if (root() == nullptr) {
    return;
  }
  const float length = m_fixedLength * m_contentScale;
  const float cross = m_rule != nullptr ? std::round(m_ruleLength * m_contentScale) : 0.0F;
  root()->setSize(m_verticalBar ? cross : length, m_verticalBar ? length : cross);
  if (m_rule != nullptr) {
    const float thickness = std::max(1.0F, std::round(Style::borderWidth * m_contentScale));
    const float offset = std::round((length - thickness) * 0.5F);
    m_rule->setFill(widgetForegroundOr(colorSpecFromRole(ColorRole::Outline)));
    m_rule->setPosition(m_verticalBar ? 0.0F : offset, m_verticalBar ? offset : 0.0F);
    m_rule->setSize(m_verticalBar ? cross : thickness, m_verticalBar ? thickness : cross);
  }
}
