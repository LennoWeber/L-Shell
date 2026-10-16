// L-Shell Window Widget - NEW IMPLEMENTATION
// Based on DESIGN-PLAN-2026
// Uses the new design system
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"

#include <string>

// ============================================================================
// WindowWidget - Modern implementation with new design system
// ============================================================================

class WindowWidget : public ModernDesignedWidget {
public:
    struct Options {
        bool show_icons = true;         // Show app icons
        bool show_titles = true;       // Show window titles
        int max_windows = 5;          // Maximum windows to show
    };

    WindowWidget(wl_output* output = nullptr, Options options = {});
    ~WindowWidget() override = default;

    void create() override;

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;

private:
    Options m_options;
    Label* m_label = nullptr;
    std::string m_lastText;
};
