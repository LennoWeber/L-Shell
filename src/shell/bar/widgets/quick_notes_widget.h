// L-Shell Quick Notes Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
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

// ============================================================================
// QuickNotesWidget - New feature for productivity
// Allows quick text notes to be saved and displayed
// ============================================================================

class QuickNotesWidget : public ModernDesignedWidget {
public:
    struct Note {
        std::string text;
        std::string timestamp;
        bool starred;
    };

    struct Options {
        bool showIcon = true;
        bool showLatestNote = true;
        bool showCount = true;
        bool showTimestamp = false;
        std::string icon = "📝";
        std::string newNotePlaceholder = "+ New Note";
        float letterSpacing = 0.0f;
        float lineHeight = 1.0f;
        int maxNotes = 10;
        int updateIntervalMs = 5000;
    };

    // Builder pattern for Options
    class OptionsBuilder {
    public:
        Options build() const;
        OptionsBuilder& showIcon(bool show);
        OptionsBuilder& showLatestNote(bool show);
        OptionsBuilder& showCount(bool show);
        OptionsBuilder& showTimestamp(bool show);
        OptionsBuilder& withIcon(std::string icon);
        OptionsBuilder& newNotePlaceholder(std::string placeholder);
        OptionsBuilder& letterSpacing(float spacing);
        OptionsBuilder& lineHeight(float height);
        OptionsBuilder& maxNotes(int count);
        OptionsBuilder& updateInterval(int ms);
    
    private:
        Options options;
    };

    QuickNotesWidget(wl_output* output = nullptr, Options options = {});
    ~QuickNotesWidget() override;

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
    void addNote(const std::string& text);
    void starNote(int index);
    std::string formatLatestNote() const;
    std::string formatTooltip() const;
    std::string getCurrentTimestamp() const;

private:
    Options m_options;
    
    // Notes state
    std::vector<Note> m_notes;
    int m_currentNoteIndex;
    bool m_editing;
    
    // UI elements
    Label* m_icon;
    Label* m_noteLabel;
    Label* m_countLabel;
    Label* m_timestampLabel;
    Node* m_innerRoot;
    
    std::string m_lastText;
    std::string m_lastTooltip;
};
