// L-Shell Task List Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/task_list_widget.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/design/typography.h"
#include "shell/design/spacing.h"
#include "shell/design/animations.h"
#include "shell/render/core/renderer.h"
#include "shell/bar/ModernDesignedWidget.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include "render/backend/cairo/cairo_wrapper.h"

// ============================================================================
// TaskListWidget Implementation
// ============================================================================

TaskListWidget::TaskListWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_currentTaskIndex(-1),
      m_icon(nullptr),
      m_progressLabel(nullptr),
      m_countLabel(nullptr),
      m_progressBar(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
    
    // Add some default tasks
    addTask({"Complete L-Shell redesign", "Finish all widgets", false, false, "2026-10-20"});
    addTask({"Write documentation", "Document new features", false, false, "2026-10-18"});
    addTask({"Test all widgets", "Verify functionality", false, true, "2026-10-16"});
}

TaskListWidget::~TaskListWidget() {
    delete m_icon;
    delete m_progressLabel;
    delete m_countLabel;
    delete m_progressBar;
    delete m_innerRoot;
}

void TaskListWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create progress label
    if (m_options.showProgress) {
        m_progressLabel = new Label("");
        m_progressLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_progressLabel->setFontSize(theme.font_size_sm.size);
        m_progressLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_progressLabel->setLetterSpacing(m_options.letterSpacing);
        m_progressLabel->setLineHeight(m_options.lineHeight);
        m_progressLabel->setColor(theme.text_primary);
    }
    
    // Create count label
    if (m_options.showCount) {
        m_countLabel = new Label("");
        m_countLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_countLabel->setFontSize(theme.font_size_tiny.size);
        m_countLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_countLabel->setLetterSpacing(m_options.letterSpacing);
        m_countLabel->setLineHeight(m_options.lineHeight);
        m_countLabel->setColor(theme.text_secondary);
    }
    
    // Update display
    updateDisplay();
    
    // Set tooltip
    setTooltip(m_lastTooltip);
    
    // Create container node
    m_innerRoot = new Node;
    m_innerRoot->setData(this);
    
    // Add children to container
    if (m_icon) m_innerRoot->addChild(m_icon->root());
    if (m_progressLabel) m_innerRoot->addChild(m_progressLabel->root());
    if (m_countLabel) m_innerRoot->addChild(m_countLabel->root());
    if (m_progressBar) m_innerRoot->addChild(m_progressBar->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 1.5f, theme.widget_height);
}

void TaskListWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update icon
    if (m_icon) {
        // Use different icon based on progress
        float progress = getProgress();
        if (progress >= 0.75f) {
            m_icon->setText("🎉");
        } else if (progress >= 0.5f) {
            m_icon->setText("✨");
        } else if (progress >= 0.25f) {
            m_icon->setText("✅");
        } else {
            m_icon->setText(m_options.icon);
        }
        m_icon->setColor(theme.text_primary);
    }
    
    // Update progress label
    if (m_progressLabel) {
        m_progressLabel->setText(formatProgress());
        m_progressLabel->setColor(theme.text_primary);
    }
    
    // Update count label
    if (m_countLabel) {
        std::ostringstream oss;
        if (m_options.showDueToday) {
            oss << formatDueTodayCount();
        } else {
            oss << getCompletedCount() << "/" << getTotalCount();
        }
        m_countLabel->setText(oss.str());
        m_countLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void TaskListWidget::addTask(const Task& task) {
    m_tasks.push_back(task);
    updateDisplay();
}

void TaskListWidget::toggleTask(int index) {
    if (index >= 0 && index < static_cast<int>(m_tasks.size())) {
        m_tasks[index].completed = !m_tasks[index].completed;
        updateDisplay();
    }
}

void TaskListWidget::starTask(int index) {
    if (index >= 0 && index < static_cast<int>(m_tasks.size())) {
        m_tasks[index].starred = !m_tasks[index].starred;
        updateDisplay();
    }
}

int TaskListWidget::getCompletedCount() const {
    int count = 0;
    for (const auto& task : m_tasks) {
        if (task.completed) count++;
    }
    return count;
}

int TaskListWidget::getTotalCount() const {
    return static_cast<int>(m_tasks.size());
}

float TaskListWidget::getProgress() const {
    if (m_tasks.empty()) return 0.0f;
    return static_cast<float>(getCompletedCount()) / m_tasks.size();
}

std::string TaskListWidget::formatProgress() const {
    std::ostringstream oss;
    float progress = getProgress();
    oss << std::fixed << std::setprecision(0) << (progress * 100) << "%";
    return oss.str();
}

std::string TaskListWidget::formatDueTodayCount() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm today_tm = *std::localtime(&now_time);
    
    int dueToday = 0;
    for (const auto& task : m_tasks) {
        // Parse due date (simple YYYY-MM-DD format)
        if (task.dueDate.length() >= 10) {
            std::string dueDate = task.dueDate.substr(0, 10);
            std::tm due_tm = {};
            if (sscanf(dueDate.c_str(), "%d-%d-%d", &due_tm.tm_year, &due_tm.tm_mon, &due_tm.tm_mday) == 3) {
                due_tm.tm_year -= 1900;
                due_tm.tm_mon -= 1;
                std::time_t due_time = std::mktime(&due_tm);
                std::time_t today_time = std::mktime(&today_tm);
                
                if (due_time >= today_time && due_time < today_time + 86400) {
                    dueToday++;
                }
            }
        }
    }
    
    std::ostringstream oss;
    oss << dueToday << " due today";
    return oss.str();
}

std::string TaskListWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "Task List\n";
    oss << "Progress: " << formatProgress() << "\n";
    oss << "Total: " << getTotalCount() << " tasks, " << getCompletedCount() << " completed\n";
    oss << "\n";
    
    // Show all tasks
    for (size_t i = 0; i < m_tasks.size(); ++i) {
        const auto& task = m_tasks[i];
        std::string status = task.completed ? "✓" : "✗";
        if (task.starred) status += "⭐";
        oss << status << " " << task.title;
        if (!task.dueDate.empty()) {
            oss << " (Due: " << task.dueDate << ")";
        }
        oss << "\n";
    }
    
    if (m_tasks.empty()) {
        oss << "No tasks. Click to add one!\n";
    } else {
        oss << "\nClick to manage tasks";
    }
    
    return oss.str();
}

void TaskListWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
    const auto& theme = getDesignTheme();
    
    float paddingX = static_cast<float>(theme.widget_padding_x);
    float paddingY = static_cast<float>(theme.widget_padding_y);
    float spacing = static_cast<float>(theme.widget_spacing);
    float availableWidth = containerWidth - 2 * paddingX;
    
    float x = paddingX;
    
    // Layout icon
    if (m_icon) {
        float iconWidth = theme.font_size_sm.size * 1.5f;
        m_icon->setSize(iconWidth, containerHeight - 2 * paddingY);
        m_icon->layout(renderer, iconWidth, containerHeight - 2 * paddingY);
        m_icon->root()->setPosition({x, paddingY});
        x += iconWidth + spacing;
        availableWidth -= iconWidth + spacing;
    }
    
    // Layout progress
    if (m_progressLabel) {
        float progressWidth = availableWidth * 0.5f;
        m_progressLabel->setSize(progressWidth, containerHeight - 2 * paddingY);
        m_progressLabel->layout(renderer, progressWidth, containerHeight - 2 * paddingY);
        m_progressLabel->root()->setPosition({x, paddingY});
        x += progressWidth + spacing;
        availableWidth -= progressWidth + spacing;
    }
    
    // Layout count
    if (m_countLabel) {
        m_countLabel->setSize(availableWidth, containerHeight - 2 * paddingY);
        m_countLabel->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
        m_countLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void TaskListWidget::doUpdate(Renderer& renderer) {
    // Nothing to update periodically
}

void TaskListWidget::onPrimaryClick() {
    // Add a new task or show the task manager
    addTask({"New Task", "", false, false, ""});
}

void TaskListWidget::onThemeChanged(const lshell::design::Theme& theme) {
    if (m_icon) {
        m_icon->setColor(theme.text_primary);
        m_icon->queueRedraw();
    }
    
    if (m_progressLabel) {
        m_progressLabel->setColor(theme.text_primary);
        m_progressLabel->queueRedraw();
    }
    
    if (m_countLabel) {
        m_countLabel->setColor(theme.text_secondary);
        m_countLabel->queueRedraw();
    }
}

void TaskListWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// TaskListWidget::Options Builder
// ============================================================================

TaskListWidget::Options TaskListWidget::OptionsBuilder::build() const {
    return options;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::showProgress(bool show) {
    options.showProgress = show;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::showCount(bool show) {
    options.showCount = show;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::showDueToday(bool show) {
    options.showDueToday = show;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

TaskListWidget::OptionsBuilder& TaskListWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
