// Modern Control Center - L-Shell
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/control_center/ModernControlCenter.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/render/core/renderer.h"

namespace lshell::control_center {

// ============================================================================
// ControlCenterTab Implementation
// ============================================================================

ControlCenterTab::ControlCenterTab(const Options& options)
    : m_options(options) {
    setName(options.name);
}

void ControlCenterTab::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    
    float padding = static_cast<float>(theme.panel_padding);
    
    // Layout children
    for (auto* child : getChildren()) {
        child->setSize(width - 2 * padding, height - 2 * padding);
        child->layout(renderer, width - 2 * padding, height - 2 * padding);
        child->root()->setPosition({padding, padding});
    }
}

void ControlCenterTab::doUpdate(Renderer& renderer) {
    // Update all children
    for (auto* child : getChildren()) {
        child->update(renderer);
    }
}

// ============================================================================
// ModernControlCenter Implementation
// ============================================================================

ModernControlCenter::ModernControlCenter(wl_output* output, Options options)
    : ui::Window(output), m_options(std::move(options)) {
    setTitle(m_options.title);
    setSize(m_options.width, m_options.height);
    setResizable(m_options.resizable);
    setDecorations(m_options.decorations);
    
    applyModernTheme();
}

void ModernControlCenter::create() {
    auto& renderer = getRenderer();
    
    // Create main container
    auto* mainContainer = new Node;
    mainContainer->setData(this);
    root()->addChild(mainContainer);
    
    // Create sidebar
    m_sidebar = new ui::Panel;
    m_sidebar->setName("control-center-sidebar");
    m_sidebar->root()->setData(this);
    mainContainer->addChild(m_sidebar->root());
    
    // Create main content area
    m_mainContent = new ui::Panel;
    m_mainContent->setName("control-center-main");
    m_mainContent->root()->setData(this);
    mainContainer->addChild(m_mainContent->root());
    
    // Create sidebar and main content
    createSidebar(renderer);
    createMainContent(renderer);
    
    // Switch to first tab if available
    if (!m_tabs.empty()) {
        switchToTab(m_tabs[0]->getName());
    }
}

void ModernControlCenter::addTab(std::unique_ptr<ControlCenterTab> tab) {
    tab->setParent(this);
    m_tabs.push_back(std::move(tab));
    
    // If this is the first tab, make it active
    if (m_tabs.size() == 1) {
        m_activeTab = m_tabs[0].get();
        m_activeTab->create();
        m_mainContent->addChild(m_activeTab->root());
    }
}

ControlCenterTab* ModernControlCenter::getTab(const std::string& name) {
    for (auto& tab : m_tabs) {
        if (tab->getName() == name) {
            return tab.get();
        }
    }
    return nullptr;
}

void ModernControlCenter::switchToTab(const std::string& name) {
    auto* tab = getTab(name);
    if (!tab || tab == m_activeTab) return;
    
    // Hide current tab
    if (m_activeTab) {
        m_activeTab->root()->setVisible(false);
    }
    
    // Show new tab
    m_activeTab = tab;
    if (!m_activeTab->m_created) {
        m_activeTab->create();
        m_activeTab->m_created = true;
        m_mainContent->addChild(m_activeTab->root());
    }
    m_activeTab->root()->setVisible(true);
    
    // Update sidebar button states
    for (auto* button : m_tabButtons) {
        button->setActive(button->getText() == name);
    }
    
    queueRedraw();
}

void ModernControlCenter::createSidebar(Renderer& renderer) {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    const auto& spacing = getSpacing();
    
    // Configure sidebar
    m_sidebar->setBackgroundColor(colors.background_secondary);
    m_sidebar->setCornerRadius(theme.corner_radius_lg);
    m_sidebar->setBorderWidth(0);
    
    // Create tab buttons
    for (auto& tab : m_tabs) {
        auto* button = new ui::Button;
        button->setText(tab->getTitle());
        button->setIcon(tab->getIcon());
        button->setSize(44, 44);
        button->setCornerRadius(theme.corner_radius_md);
        button->setTooltip(tab->getTitle());
        button->setToggleable(true);
        
        button->onClick = [this, name = tab->getName()]() {
            switchToTab(name);
        };
        
        m_sidebar->addChild(button->root());
        m_tabButtons.push_back(button);
    }
}

void ModernControlCenter::createMainContent(Renderer& renderer) {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    
    // Configure main content area
    m_mainContent->setBackgroundColor(colors.background_primary);
    m_mainContent->setCornerRadius(theme.corner_radius_lg);
    m_mainContent->setBorderWidth(0);
}

void ModernControlCenter::layoutSidebar(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float sidebarWidth = 200.0f;
    float padding = static_cast<float>(spacing.md);
    
    m_sidebar->setSize(sidebarWidth, height);
    m_sidebar->layout(renderer, sidebarWidth, height);
    m_sidebar->root()->setPosition({0, 0});
    
    // Layout tab buttons
    float buttonSize = 44.0f;
    float buttonSpacing = 4.0f;
    float y = padding;
    
    for (auto* button : m_tabButtons) {
        button->setSize(sidebarWidth - 2 * padding, buttonSize);
        button->layout(renderer, sidebarWidth - 2 * padding, buttonSize);
        button->root()->setPosition({padding, y});
        y += buttonSize + buttonSpacing;
    }
}

void ModernControlCenter::layoutMainContent(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    const auto& spacing = getSpacing();
    
    float sidebarWidth = 200.0f;
    float contentWidth = width - sidebarWidth;
    
    m_mainContent->setSize(contentWidth, height);
    m_mainContent->layout(renderer, contentWidth, height);
    m_mainContent->root()->setPosition({sidebarWidth, 0});
    
    // Layout active tab
    if (m_activeTab) {
        m_activeTab->setSize(contentWidth, height);
        m_activeTab->layout(renderer, contentWidth, height);
    }
}

void ModernControlCenter::doLayout(Renderer& renderer, float width, float height) {
    const auto& theme = getDesignTheme();
    
    // Set window size
    setSize(width, height);
    
    // Layout sidebar and main content
    layoutSidebar(renderer, width, height);
    layoutMainContent(renderer, width, height);
}

void ModernControlCenter::doUpdate(Renderer& renderer) {
    // Update all tabs
    for (auto& tab : m_tabs) {
        tab->update(renderer);
    }
}

void ModernControlCenter::onThemeChanged(const lshell::design::Theme& theme) {
    applyModernTheme();
    
    // Propagate theme change to tabs
    for (auto& tab : m_tabs) {
        // Tab will handle its own theme updates
    }
}

void ModernControlCenter::applyModernTheme() {
    const auto& theme = getDesignTheme();
    const auto& colors = getColorPalette();
    
    // Apply theme to window
    setBackgroundColor(colors.background_primary);
    setCornerRadius(theme.corner_radius_lg);
}

const lshell::design::Theme& ModernControlCenter::getDesignTheme() const {
    return lshell::design::Theme::get();
}

const lshell::design::ColorPalette& ModernControlCenter::getColorPalette() const {
    return lshell::design::ColorPalette::get();
}

const lshell::design::Typography& ModernControlCenter::getTypography() const {
    return lshell::design::Typography::get();
}

const lshell::design::Spacing& ModernControlCenter::getSpacing() const {
    return lshell::design::Spacing::get();
}

void ModernControlCenter::toggleVisibility() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

void ModernControlCenter::show() {
    ui::Window::show();
    // Bring to front and focus
    bringToFront();
    focus();
}

void ModernControlCenter::hide() {
    ui::Window::hide();
}

} // namespace lshell::control_center
