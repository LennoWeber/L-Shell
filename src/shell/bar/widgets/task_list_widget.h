// L-Shell Task List Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

#include <string>
#include <vector>
#include <memory>

class Label;
class Node;
class ProgressBar;

// ============================================================================
// TaskListWidget - New feature for productivity
// Displays and manages a to-do list with progress tracking
// ============================================================================

class TaskListWidget : public ModernDesignedWidget {
public:
    struct Task {
        std::string title;
        std::string description;
        bool completed;
        bool starred;
        std::string dueDate;
    };

    struct Options {
        bool showIcon = true;
        bool showProgress = true;
        bool showCount = true;
        bool showDueToday = false;
        std::string icon = "✅";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int updateIntervalMs = 300000; // 5 minutes
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showProgress(bool show);
        OptionsBuilder& showCount(bool show);
        OptionsBuilder& showDueToday(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    TaskListWidget(wl_output* output = nullptr, Options options = {});
    ~TaskListWidget() override;

    void create() override;
    void applyCommonOptions(
        const CommonWidgetOptions& options, FontWeight barFontWeight, 
        const std::string& barFontFamily, std::string_view logContext) override;

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;
    
    // Widget actions
    void onPrimaryClick() override;

private:
    void updateDisplay();
    void addTask(const Task& task);
    void toggleTask(int index);
    void starTask(int index);
    int getCompletedCount() const;
    int getTotalCount() const;
    float getProgress() const;
    std::string formatProgress() const;
    std::string formatTooltip() const;
    std::string formatDueTodayCount() const;

private:
    Options m_options;
    
    // Tasks state
    std::vector<Task> m_tasks;
    int m_currentTaskIndex;
    
    // UI elements
    Label* m_icon;
    Label* m_progressLabel;
    Label* m_countLabel;
    ProgressBar* m_progressBar;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
