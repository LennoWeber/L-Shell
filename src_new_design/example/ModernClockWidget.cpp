// Modern Clock Widget - Implementation
// Example showing how to use L-Shell Design System

#include "ModernClockWidget.h"
#include "shell/design/theme.h"

#include <ctime>
#include <iomanip>
#include <sstream>

// ============================================================================
// ModernClockWidget Implementation
// ============================================================================

ModernClockWidget::ModernClockWidget(const Options& options)
    : m_options(options) {
    
    // Initialize with current theme
    applyThemeColors();
    
    // Listen for theme changes
    lshell::design::ThemeManager::get_instance().add_theme_change_listener(
        [this](const lshell::design::Theme& theme) {
            onThemeChanged(theme);
        }
    );
    
    // Set initial time
    updateTime();
}

ModernClockWidget::~ModernClockWidget() {
    // In a real implementation, you'd remove the theme listener
    // This requires tracking the listener ID
}

void ModernClockWidget::create() {
    // Initialize the widget
    // In a real implementation, this would create the visual elements
    
    // Apply initial theme
    applyThemeColors();
}

void ModernClockWidget::setUpdateInterval(int seconds) {
    m_updateInterval = seconds;
    // In a real implementation, restart the update timer
}

void ModernClockWidget::setTimeFormat(const std::string& format) {
    m_options.time_format = format;
    updateTime();
}

void ModernClockWidget::setDateFormat(const std::string& format) {
    m_options.date_format = format;
    if (m_options.show_date) {
        updateTime();
    }
}

void ModernClockWidget::setShowDate(bool show) {
    m_options.show_date = show;
    updateTime();
}

void ModernClockWidget::setShowSeconds(bool show) {
    m_options.show_seconds = show;
    updateTime();
    // If showing seconds, update more frequently
    setUpdateInterval(show ? 1 : 60);
}

void ModernClockWidget::set24HourFormat(bool use_24_hour) {
    m_options.use_24_hour = use_24_hour;
    updateTime();
}

const lshell::design::Theme& ModernClockWidget::getTheme() const {
    return lshell::design::get_theme();
}

void ModernClockWidget::applyThemeColors() {
    const auto& theme = getTheme();
    
    // Apply colors from design system
    m_textColor = theme.text_primary;
    m_secondaryTextColor = theme.text_secondary;
    
    // Apply typography from design system
    m_fontFamily = theme.font_family_primary;
    m_timeFontSize = theme.font_size_lg;  // 16px
    m_dateFontSize = theme.font_size_tiny; // 10px
    
    // Apply spacing from design system
    m_horizontalPadding = theme.widget_padding_x;  // 8px
    m_verticalPadding = theme.widget_padding_y;    // 4px
    m_spacing = theme.widget_text_spacing;         // 4px
    
    // In a real implementation, you'd apply these to your visual elements
}

void ModernClockWidget::onThemeChanged(const lshell::design::Theme& theme) {
    applyThemeColors();
    
    // In a real implementation, you'd trigger a redraw
    // queue_draw();
}

void ModernClockWidget::updateTime() {
    // Get current time
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    // Format time string
    std::ostringstream timeStream;
    
    if (m_options.use_24_hour) {
        timeStream << std::put_time(&tm, m_options.time_format.c_str());
    } else {
        // Convert to 12-hour format if needed
        // This is a simplified approach
        std::string format = m_options.time_format;
        // Replace %H with %I for 12-hour
        size_t pos = format.find("%H");
        if (pos != std::string::npos) {
            format.replace(pos, 2, "%I");
        }
        timeStream << std::put_time(&tm, format.c_str());
    }
    
    m_currentTime = timeStream.str();
    
    // Format date string if needed
    if (m_options.show_date) {
        std::ostringstream dateStream;
        dateStream << std::put_time(&tm, m_options.date_format.c_str());
        m_currentDate = dateStream.str();
    } else {
        m_currentDate.clear();
    }
    
    // In a real implementation, you'd update the visual elements
    // and trigger a redraw
}

void ModernClockWidget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {
    const auto& theme = getTheme();
    
    // Calculate widget dimensions based on design system
    float widgetHeight = theme.widget_height;  // 24px
    float minWidth = theme.widget_min_width;    // 24px
    
    // In a real implementation, you'd calculate the actual width needed
    // based on the text size
    
    // Layout the widget with design system spacing
    // In a real implementation, this would position your visual elements
    // using the padding and spacing values from the design system
}

void ModernClockWidget::doUpdate(Renderer& renderer) {
    const auto& theme = getTheme();
    
    // Update time if needed
    updateTime();
    
    // In a real implementation, you'd update the visual elements
    // with the current time and date
    
    // Apply design system colors and typography
    // For example:
    // - Set text color to m_textColor
    // - Set font family to m_fontFamily.primary
    // - Set font size to m_timeFontSize.size
    // - Set padding to m_horizontalPadding and m_verticalPadding
}

bool ModernClockWidget::onPointerEvent(const PointerEvent& event) {
    // Handle pointer events
    // In a real implementation, you might show a tooltip or open a calendar
    
    // Use design system animations for hover effects
    // For example:
    // - Scale up by 5% on hover
    // - Change background color to theme.widget_hover
    
    return false;  // Event not handled
}

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

// Format time with design system considerations
std::string formatTimeWithDesign(const std::tm& tm, const std::string& format, bool use_24_hour) {
    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    return oss.str();
}

// Get appropriate text color based on background
lshell::design::Color getTextColor(const lshell::design::Theme& theme, 
                                   const lshell::design::Color& background) {
    return theme.get_contrast_text(background);
}

} // anonymous namespace

// ============================================================================
// Example: Creating a modern clock widget
// ============================================================================

/*
 * Usage example:
 * 
 * // Create a modern clock widget
 * auto* clock = new ModernClockWidget();
 * 
 * // Customize options
 * ModernClockWidget::Options options;
 * options.time_format = "%H:%M:%S";  // Show seconds
 * options.show_date = true;
 * options.date_format = "%a, %b %d";
 * 
 * auto* clockWithOptions = new ModernClockWidget(options);
 * 
 * // Add to bar
 * bar->addWidget(clock);
 * 
 * // The widget will automatically:
 * // - Use theme colors
 * // - Update every minute (or every second if showing seconds)
 * // - Respond to theme changes
 * // - Use design system spacing and typography
 */
