// L-Shell Workspace Widget - NEW IMPLEMENTATION
// Based on DESIGN-PLAN-2026
// Uses the new design system
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"

#include <string>

// ============================================================================
// WorkspaceWidget - Modern implementation with new design system
// ============================================================================

class WorkspaceWidget : public ModernDesignedWidget {
public:
    struct Options {
        bool show_numbers = true;      // Show workspace numbers
        bool show_names = false;       // Show workspace names
        bool show_indicator = true;    // Show current workspace indicator
    };

    WorkspaceWidget(wl_output* output = nullptr, Options options = {});
    ~WorkspaceWidget() override = default;

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
