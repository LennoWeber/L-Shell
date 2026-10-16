// Modern Control Center - L-Shell
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#pragma once

#include "../design/theme.h"
#include "../design/colors.h"
#include "../design/typography.h"
#include "../design/spacing.h"
#include "../design/animations.h"

#include "../../ui/controls/button.h"
#include "../panel/panel.h"

#include <vector>
#include <string>
#include <functional>
#include <memory>

// ============================================================================
// Forward Declarations
// ============================================================================

namespace lshell::control_center {

class ControlCenterTab;

// ============================================================================
// ControlCenterTab - Base class for all control center tabs
// ============================================================================

class ControlCenterTab : public shell::panel::Panel {
public:
    struct Options {
        std::string name;
        std::string icon;
        std::string title;
        bool scrollable = true;
    };

    ControlCenterTab(const Options& options);
    virtual ~ControlCenterTab() = default;

    virtual void create() = 0;
    virtual void doLayout(Renderer& renderer, float width, float height) override;
    virtual void doUpdate(Renderer& renderer) override;

    const std::string& getName() const { return m_options.name; }
    const std::string& getIcon() const { return m_options.icon; }
    const std::string& getTitle() const { return m_options.title; }

protected:
    Options m_options;
    bool m_created = false;
};

// ============================================================================
// ModernControlCenter - Main control center class
// ============================================================================

class ModernControlCenter : public shell::panel::Panel {
public:
    struct Options {
        std::string title = "Control Center";
        float width = 800.0f;
        float height = 600.0f;
        bool resizable = true;
        bool decorations = true;
        bool transparent = false;
        std::string corner = ""; // Which corner to anchor to
    };

    ModernControlCenter(wl_output* output = nullptr, Options options = {});
    ~ModernControlCenter() override = default;

    void create() override;
    void toggleVisibility();
    void show();
    void hide();

    // Add tabs to the control center
    void addTab(std::unique_ptr<ControlCenterTab> tab);
    ControlCenterTab* getTab(const std::string& name);
    void switchToTab(const std::string& name);

protected:
    void doLayout(Renderer& renderer, float width, float height) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;

private:
    Options m_options;
    std::vector<std::unique_ptr<ControlCenterTab>> m_tabs;
    ControlCenterTab* m_activeTab = nullptr;
    
    // UI Elements
    ui::Panel* m_sidebar = nullptr;
    ui::Panel* m_mainContent = nullptr;
    std::vector<ui::Button*> m_tabButtons;
    
    // Design system
    const lshell::design::Theme& getDesignTheme() const;
    const lshell::design::ColorPalette& getColorPalette() const;
    const lshell::design::Typography& getTypography() const;
    const lshell::design::Spacing& getSpacing() const;
    
    void createSidebar(Renderer& renderer);
    void createMainContent(Renderer& renderer);
    void layoutSidebar(Renderer& renderer, float width, float height);
    void layoutMainContent(Renderer& renderer, float width, float height);
    
    void applyModernTheme();
};

// ============================================================================
// Helper functions and utilities
// ============================================================================

} // namespace lshell::control_center
