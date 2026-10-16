#!/usr/bin/env python3
"""
Widget Generator for L-Shell New Design System
Generates widget implementations based on templates
"""

import os
import sys
from datetime import datetime

# Widget templates
WIDGET_HEADER_TEMPLATE = """// L-Shell {widget_name} Widget - NEW IMPLEMENTATION
// Based on DESIGN-PLAN-2026
// Uses the new design system
// Generated: {date}
// Author: L-Shell Design Team

#pragma once

#include "shell/bar/ModernDesignedWidget.h"
#include "shell/design/theme.h"

#include <string>

// ============================================================================
// {widget_name}Widget - Modern implementation with new design system
// ============================================================================

class {widget_name}Widget : public ModernDesignedWidget {{
public:
    struct Options {{
{options_content}    }};

    {widget_name}Widget(wl_output* output = nullptr, Options options = {{}});
    ~{widget_name}Widget() override = default;

    void create() override;

protected:
    void doLayout(Renderer& renderer, float containerWidth, float containerHeight) override;
    void doUpdate(Renderer& renderer) override;
    void onThemeChanged(const lshell::design::Theme& theme) override;

private:
{private_members}}};
"""

WIDGET_SOURCE_TEMPLATE = """// L-Shell {widget_name} Widget - NEW IMPLEMENTATION
// Based on DESIGN-PLAN-2026
// Uses the new design system
// Generated: {date}
// Author: L-Shell Design Team

#include "shell/bar/widgets/{widget_name_lower}_widget.h"
#include "shell/render/core/renderer.h"
#include "shell/design/theme.h"
#include "shell/design/colors.h"

// ============================================================================
// {widget_name}Widget Implementation
// ============================================================================

{widget_name}Widget::{widget_name}Widget(wl_output* output, Options options)
    : ModernDesignedWidget(output), m_options(std::move(options)) {{
    // Apply NEW design system
    applyDesignTheme();
}}

void {widget_name}Widget::create() {{
    const auto& theme = getDesignTheme();
    
    // Create label
    m_label = new Label(getDisplayText());
    m_label->setFontFamily(getPrimaryFontFamily().primary);
    m_label->setFontSize(theme.font_size_sm.size);
    m_label->setFontWeight(static_cast<::FontWeight>(theme.font_size_sm.weight));
    m_label->setColor(theme.text_primary);
    
    // Set tooltip
    setTooltip("{widget_name} widget");
    
    // Create container node
    m_innerRoot = new Node;
    m_innerRoot->setData(this);
    m_innerRoot->addChild(m_label->root());
    
    // Set min size from design system
    setMinSize(theme.widget_min_width, theme.widget_height);
}}

void {widget_name}Widget::doLayout(Renderer& renderer, float containerWidth, float containerHeight) {{
    const auto& theme = getDesignTheme();
    
    float paddingX = static_cast<float>(theme.widget_padding_x);
    float paddingY = static_cast<float>(theme.widget_padding_y);
    float availableWidth = containerWidth - 2 * paddingX;
    
    m_label->setSize(availableWidth, containerHeight - 2 * paddingY);
    m_label->layout(renderer, availableWidth, containerHeight - 2 * paddingY);
    m_label->root()->setPosition({{paddingX, paddingY}});
    
    m_innerRoot->setSize(containerWidth, containerHeight);
}}

void {widget_name}Widget::doUpdate(Renderer& renderer) {{
    const auto& theme = getDesignTheme();
    
    // Update label text
    std::string newText = getDisplayText();
    if (newText != m_lastText) {{
        m_lastText = newText;
        m_label->setText(newText);
    }}
    
    // Update colors from theme
    m_label->setColor(theme.text_primary);
}}

void {widget_name}Widget::onThemeChanged(const lshell::design::Theme& theme) {{
    // Update label colors
    if (m_label) {{
        m_label->setColor(theme.text_primary);
        m_label->queueRedraw();
    }}
}}

std::string {widget_name}Widget::getDisplayText() {{
    // Default implementation - override in derived classes
    return "{widget_name}";
}}

// ============================================================================
// Helper functions
// ============================================================================
"""

# Widget configurations
WIDGETS = {
    "Clock": {
        "description": "Displays current time",
        "options": "        std::string format = \"%H:%M\";  // Time format\n        std::string timezone;           // Timezone\n        bool show_seconds = false;     // Show seconds\n        bool show_date = false;        // Show date below time\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatTimeText();"
    },
    "Battery": {
        "description": "Displays battery status",
        "options": "        bool show_percentage = true;   // Show percentage\n        bool show_icon = true;         // Show battery icon\n        bool show_time = true;         // Show remaining time\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatBatteryText();"
    },
    "Network": {
        "description": "Displays network status",
        "options": "        bool show_ssid = true;         // Show network name\n        bool show_signal = true;       // Show signal strength\n        bool show_type = true;         // Show WiFi/Ethernet\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatNetworkText();"
    },
    "Volume": {
        "description": "Displays and controls volume",
        "options": "        bool show_percentage = true;   // Show percentage\n        bool show_icon = true;         // Show volume icon\n        bool show_muted = true;         // Show mute indicator\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatVolumeText();"
    },
    "Workspace": {
        "description": "Displays workspace information",
        "options": "        bool show_numbers = true;      // Show workspace numbers\n        bool show_names = false;       // Show workspace names\n        bool show_indicator = true;    // Show current workspace indicator\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatWorkspaceText();"
    },
    "Window": {
        "description": "Displays active windows",
        "options": "        bool show_icons = true;         // Show app icons\n        bool show_titles = true;       // Show window titles\n        int max_windows = 5;          // Maximum windows to show\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatWindowText();"
    },
    "SystemMonitor": {
        "description": "Displays system resource usage",
        "options": "        bool show_cpu = true;           // Show CPU usage\n        bool show_ram = true;           // Show RAM usage\n        bool show_gpu = true;           // Show GPU usage\n        std::string display_mode = \"compact\"; // compact, detailed\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatSystemMonitorText();"
    },
    "Weather": {
        "description": "Displays weather information",
        "options": "        bool show_icon = true;         // Show weather icon\n        bool show_temp = true;         // Show temperature\n        bool show_condition = true;    // Show weather condition\n        std::string location = \"auto\";      // Location\n        std::string units = \"celsius\";   // Temperature units\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatWeatherText();"
    },
    "Notification": {
        "description": "Displays notification count",
        "options": "        bool show_icon = true;         // Show notification icon\n        bool show_count = true;        // Show unread count\n        int max_count = 99;            // Maximum count to show\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n    int m_notificationCount = 0;\n",
        "display_text": "return formatNotificationText();"
    },
    "Media": {
        "description": "Displays and controls media player",
        "options": "        bool show_track = true;       // Show current track\n        bool show_artist = true;      // Show artist\n        bool show_controls = false;    // Show play/pause controls\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatMediaText();"
    },
    "Bluetooth": {
        "description": "Displays bluetooth status",
        "options": "        bool show_icon = true;         // Show bluetooth icon\n        bool show_status = true;       // Show on/off status\n        bool show_devices = true;      // Show connected devices\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatBluetoothText();"
    },
    "Brightness": {
        "description": "Displays and controls screen brightness",
        "options": "        bool show_percentage = true;   // Show percentage\n        bool show_icon = true;         // Show brightness icon\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatBrightnessText();"
    },
    "KeyboardLayout": {
        "description": "Displays current keyboard layout",
        "options": "        bool show_icon = true;         // Show keyboard icon\n        bool show_name = true;         // Show layout name\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatKeyboardLayoutText();"
    },
    "Nightlight": {
        "description": "Displays and controls nightlight",
        "options": "        bool show_icon = true;         // Show nightlight icon\n        bool show_status = true;       // Show on/off status\n        bool show_temperature = true;   // Show color temperature\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatNightlightText();"
    },
    "Screenshot": {
        "description": "Quick access to screenshot functions",
        "options": "        std::string action = \"region\"; // region, fullscreen, window\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return \"📷\";"
    },
    "PowerProfile": {
        "description": "Displays and changes power profile",
        "options": "        bool show_icon = true;         // Show power icon\n        bool show_name = true;         // Show profile name\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatPowerProfileText();"
    },
    "Clipboard": {
        "description": "Displays clipboard history",
        "options": "        int max_items = 10;            // Maximum items to show\n        bool show_preview = true;      // Show content preview\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n    int m_itemCount = 0;\n",
        "display_text": "return formatClipboardText();"
    },
    "Privacy": {
        "description": "Displays privacy status (mic/camera)",
        "options": "        bool show_mic = true;          // Show microphone status\n        bool show_camera = true;        // Show camera status\n",
        "private": "    Options m_options;\n    Label* m_label = nullptr;\n    std::string m_lastText;\n",
        "display_text": "return formatPrivacyText();"
    },
    "Tray": {
        "description": "Displays system tray icons",
        "options": "        int icon_size = 16;            // Tray icon size\n        bool show_labels = false;      // Show app labels\n",
        "private": "    Options m_options;\n    // Tray icons would be managed separately\n    std::string m_lastText;\n",
        "display_text": "return \"\";"
    },
    "Spacer": {
        "description": "Flexible spacer widget",
        "options": "        int size = 8;                 // Spacer size in pixels\n        bool expand = false;           // Expand to fill space\n",
        "private": "    Options m_options;\n",
        "display_text": "return \"\";"
    },
}


def generate_widget_files(widget_name, widget_config, output_dir):
    """Generate header and source files for a widget"""
    
    # Create widget name variations
    widget_name_lower = widget_name.lower()
    date = datetime.now().strftime("%Y-%m-%d")
    
    # Generate header
    options_content = widget_config.get("options", "")
    private_members = widget_config.get("private", "")
    
    header_content = WIDGET_HEADER_TEMPLATE.format(
        widget_name=widget_name,
        widget_name_lower=widget_name_lower,
        date=date,
        options_content=options_content,
        private_members=private_members
    )
    
    # Generate source
    display_text = widget_config.get("display_text", "return \"{widget_name}\";")
    
    source_content = WIDGET_SOURCE_TEMPLATE.format(
        widget_name=widget_name,
        widget_name_lower=widget_name_lower,
        date=date,
        display_text=display_text
    )
    
    # Write files
    header_path = os.path.join(output_dir, f"{widget_name_lower}_widget.h")
    source_path = os.path.join(output_dir, f"{widget_name_lower}_widget.cpp")
    
    with open(header_path, 'w') as f:
        f.write(header_content)
    
    with open(source_path, 'w') as f:
        f.write(source_content)
    
    print(f"Generated: {widget_name} widget ({header_path}, {source_path})")


def main():
    output_dir = "src/shell/bar/widgets"
    
    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate all widgets
    for widget_name, config in WIDGETS.items():
        generate_widget_files(widget_name, config, output_dir)
    
    print(f"\nGenerated {len(WIDGETS)} widgets in {output_dir}")


if __name__ == "__main__":
    main()
