#include "shell/status_overlay/status_overlay_panel.h"

#include <stdexcept>

StatusOverlayPanel::StatusOverlayPanel(StatusOverlayServices services) : m_services(services) {}

void StatusOverlayPanel::create() { throw std::logic_error("StatusOverlayPanel::create: not implemented"); }

void StatusOverlayPanel::onOpen(std::string_view context) {
  (void)context;
  throw std::logic_error("StatusOverlayPanel::onOpen: not implemented");
}

void StatusOverlayPanel::onClose() { throw std::logic_error("StatusOverlayPanel::onClose: not implemented"); }

bool StatusOverlayPanel::handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) {
  (void)sym;
  (void)modifiers;
  (void)pressed;
  (void)preedit;
  throw std::logic_error("StatusOverlayPanel::handleGlobalKey: not implemented");
}

float StatusOverlayPanel::preferredWidth() const {
  throw std::logic_error("StatusOverlayPanel::preferredWidth: not implemented");
}

float StatusOverlayPanel::preferredHeight() const {
  throw std::logic_error("StatusOverlayPanel::preferredHeight: not implemented");
}

void StatusOverlayPanel::doLayout(Renderer& renderer, float width, float height) {
  (void)renderer;
  (void)width;
  (void)height;
  throw std::logic_error("StatusOverlayPanel::doLayout: not implemented");
}

void StatusOverlayPanel::doUpdate(Renderer& renderer) {
  (void)renderer;
  throw std::logic_error("StatusOverlayPanel::doUpdate: not implemented");
}

void StatusOverlayPanel::onPanelCardOpacityChanged(float opacity) {
  (void)opacity;
  throw std::logic_error("StatusOverlayPanel::onPanelCardOpacityChanged: not implemented");
}

void StatusOverlayPanel::buildTiles(float scale) {
  (void)scale;
  throw std::logic_error("StatusOverlayPanel::buildTiles: not implemented");
}

void StatusOverlayPanel::buildTile(Tile& tile, float scale) {
  (void)tile;
  (void)scale;
  throw std::logic_error("StatusOverlayPanel::buildTile: not implemented");
}

void StatusOverlayPanel::refreshTile(Tile& tile) {
  (void)tile;
  throw std::logic_error("StatusOverlayPanel::refreshTile: not implemented");
}

void StatusOverlayPanel::refreshClock() { throw std::logic_error("StatusOverlayPanel::refreshClock: not implemented"); }
