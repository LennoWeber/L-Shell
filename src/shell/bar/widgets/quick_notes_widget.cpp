// L-Shell Quick Notes Widget - NEW FEATURE from DESIGN-PLAN-ERWEITERT-2026
// Based on DESIGN-PLAN-ERWEITERT-2026
// Author: L-Shell Design Team

#include "shell/bar/widgets/quick_notes_widget.h"
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
// QuickNotesWidget Implementation
// ============================================================================

QuickNotesWidget::QuickNotesWidget(wl_output* output, Options options)
    : ModernDesignedWidget(output),
      m_options(std::move(options)),
      m_currentNoteIndex(-1),
      m_editing(false),
      m_icon(nullptr),
      m_noteLabel(nullptr),
      m_countLabel(nullptr),
      m_timestampLabel(nullptr),
      m_innerRoot(nullptr),
      m_lastText(""),
      m_lastTooltip("") {
    
    applyDesignTheme();
    
    // Add a default note
    addNote(m_options.newNotePlaceholder);
}

QuickNotesWidget::~QuickNotesWidget() {
    delete m_icon;
    delete m_noteLabel;
    delete m_countLabel;
    delete m_timestampLabel;
    delete m_innerRoot;
}

void QuickNotesWidget::create() {
    const auto& theme = getDesignTheme();
    
    // Create icon label
    if (m_options.showIcon) {
        m_icon = new Label(m_options.icon);
        m_icon->setFontFamily(getPrimaryFontFamily().icon);
        m_icon->setFontSize(theme.font_size_sm.size);
        m_icon->setColor(theme.text_primary);
    }
    
    // Create note label
    if (m_options.showLatestNote) {
        m_noteLabel = new Label("");
        m_noteLabel->setFontFamily(getPrimaryFontFamily().primary);
        m_noteLabel->setFontSize(theme.font_size_sm.size);
        m_noteLabel->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
        m_noteLabel->setLetterSpacing(m_options.letterSpacing);
        m_noteLabel->setLineHeight(m_options.lineHeight);
        m_noteLabel->setColor(theme.text_primary);
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
    
    // Create timestamp label
    if (m_options.showTimestamp) {
        m_timestampLabel = new Label("");
        m_timestampLabel->setFontFamily(getPrimaryFontFamily().secondary);
        m_timestampLabel->setFontSize(theme.font_size_tiny.size);
        m_timestampLabel->setFontWeight(static_cast<::FontWeight>(FontWeight::REGULAR));
        m_timestampLabel->setLetterSpacing(m_options.letterSpacing);
        m_timestampLabel->setLineHeight(m_options.lineHeight);
        m_timestampLabel->setColor(theme.text_secondary);
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
    if (m_noteLabel) m_innerRoot->addChild(m_noteLabel->root());
    if (m_countLabel) m_innerRoot->addChild(m_countLabel->root());
    if (m_timestampLabel) m_innerRoot->addChild(m_timestampLabel->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width * 2, theme.widget_height);
}

void QuickNotesWidget::updateDisplay() {
    const auto& theme = getDesignTheme();
    
    // Update icon
    if (m_icon) {
        m_icon->setColor(theme.text_primary);
    }
    
    // Update note label
    if (m_noteLabel) {
        m_noteLabel->setText(formatLatestNote());
        m_noteLabel->setColor(theme.text_primary);
    }
    
    // Update count label
    if (m_countLabel) {
        std::ostringstream oss;
        oss << m_notes.size() << " notes";
        m_countLabel->setText(oss.str());
        m_countLabel->setColor(theme.text_secondary);
    }
    
    // Update timestamp label
    if (m_timestampLabel && !m_notes.empty()) {
        m_timestampLabel->setText(m_notes.front().timestamp);
        m_timestampLabel->setColor(theme.text_secondary);
    }
    
    // Update tooltip
    m_lastTooltip = formatTooltip();
    setTooltip(m_lastTooltip);
}

void QuickNotesWidget::addNote(const std::string& text) {
    if (text.empty()) return;
    
    // Remove oldest note if we exceed max
    if (m_notes.size() >= static_cast<size_t>(m_options.maxNotes)) {
        m_notes.pop_back();
    }
    
    // Add new note at the front
    Note note;
    note.text = text;
    note.timestamp = getCurrentTimestamp();
    note.starred = false;
    m_notes.insert(m_notes.begin(), note);
    
    updateDisplay();
}

void QuickNotesWidget::starNote(int index) {
    if (index >= 0 && index < static_cast<int>(m_notes.size())) {
        m_notes[index].starred = !m_notes[index].starred;
        updateDisplay();
    }
}

std::string QuickNotesWidget::formatLatestNote() const {
    if (m_notes.empty()) {
        return m_options.newNotePlaceholder;
    }
    
    // Truncate long notes
    std::string text = m_notes.front().text;
    if (text.length() > 20) {
        text = text.substr(0, 18) + "..";
    }
    
    return text;
}

std::string QuickNotesWidget::formatTooltip() const {
    std::ostringstream oss;
    oss << "Quick Notes\n";
    oss << "Total: " << m_notes.size() << " notes\n";
    oss << "\n";
    
    // Show last 5 notes
    int count = 0;
    for (const auto& note : m_notes) {
        if (count >= 5) break;
        oss << "• " << note.text;
        if (note.starred) oss << " ⭐";
        oss << "\n";
        oss << "  " << note.timestamp << "\n";
        count++;
    }
    
    if (m_notes.empty()) {
        oss << "No notes yet. Click to add one!\n";
    } else {
        oss << "\nClick to add/edit notes";
    }
    
    return oss.str();
}

std::string QuickNotesWidget::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm = *std::localtime(&now_time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void QuickNotesWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
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
    
    // Layout note
    if (m_noteLabel) {
        float noteWidth = availableWidth * 0.6f;
        m_noteLabel->setSize(noteWidth, containerHeight - 2 * paddingY);
        m_noteLabel->layout(renderer, noteWidth, containerHeight - 2 * paddingY);
        m_noteLabel->root()->setPosition({x, paddingY});
        x += noteWidth + spacing;
        availableWidth -= noteWidth + spacing;
    }
    
    // Layout count
    if (m_countLabel) {
        float countWidth = availableWidth * 0.4f;
        m_countLabel->setSize(countWidth, containerHeight - 2 * paddingY);
        m_countLabel->layout(renderer, countWidth, containerHeight - 2 * paddingY);
        m_countLabel->root()->setPosition({x, paddingY});
    }
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}

void QuickNotesWidget::doUpdate(Renderer& renderer) {
    // Nothing to update periodically
}

void QuickNotesWidget::onPrimaryClick() {
    // Add a new note or show the notes editor
    addNote("New note at " + getCurrentTimestamp());
}

void QuickNotesWidget::onThemeChanged(const lshell::design::Theme& theme) {
    if (m_icon) {
        m_icon->setColor(theme.text_primary);
        m_icon->queueRedraw();
    }
    
    if (m_noteLabel) {
        m_noteLabel->setColor(theme.text_primary);
        m_noteLabel->queueRedraw();
    }
    
    if (m_countLabel) {
        m_countLabel->setColor(theme.text_secondary);
        m_countLabel->queueRedraw();
    }
    
    if (m_timestampLabel) {
        m_timestampLabel->setColor(theme.text_secondary);
        m_timestampLabel->queueRedraw();
    }
}

void QuickNotesWidget::applyCommonOptions(
    const CommonWidgetOptions& options, FontWeight barFontWeight, 
    const std::string& barFontFamily, std::string_view logContext) {
    
    ModernDesignedWidget::applyCommonOptions(options, barFontWeight, barFontFamily, logContext);
    applyDesignTheme();
}

// ============================================================================
// QuickNotesWidget::Options Builder
// ============================================================================

QuickNotesWidget::Options QuickNotesWidget::OptionsBuilder::build() const {
    return options;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::showIcon(bool show) {
    options.showIcon = show;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::showLatestNote(bool show) {
    options.showLatestNote = show;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::showCount(bool show) {
    options.showCount = show;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::showTimestamp(bool show) {
    options.showTimestamp = show;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::withIcon(std::string icon) {
    options.icon = std::move(icon);
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::newNotePlaceholder(std::string placeholder) {
    options.newNotePlaceholder = std::move(placeholder);
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::letterSpacing(float spacing) {
    options.letterSpacing = spacing;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::lineHeight(float height) {
    options.lineHeight = height;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::maxNotes(int count) {
    options.maxNotes = count;
    return *this;
}

QuickNotesWidget::OptionsBuilder& QuickNotesWidget::OptionsBuilder::updateInterval(int ms) {
    options.updateIntervalMs = ms;
    return *this;
}
