// L-Shell Modern Widget - Implementation
// Uses the new L-Shell Design System
// Generated: 2026-10-15
// Author: L-Shell Design Team

#include "ModernWidget.h"
#include "shell/design/theme.h"

namespace lshell::new_impl {

// ============================================================================
// ModernWidget Implementation
// ============================================================================

ModernWidget::ModernWidget() {
    // Generate unique ID
    static int widget_counter = 0;
    m_id = "widget_" + std::to_string(widget_counter++);
    
    // Register for theme changes
    register_theme_listener();
    
    // Apply initial theme
    apply_theme(lshell::design::get_theme());
}

ModernWidget::~ModernWidget() {
    unregister_theme_listener();
}

void ModernWidget::set_state(WidgetState state) {
    if (m_state == state) return;
    
    WidgetState old_state = m_state;
    m_state = state;
    state_changed(old_state, state);
}

void ModernWidget::set_margin(float top, float right, float bottom, float left) {
    m_margin_top = top;
    m_margin_right = right;
    m_margin_bottom = bottom;
    m_margin_left = left;
}

void ModernWidget::set_margin(float horizontal, float vertical) {
    set_margin(vertical, horizontal, vertical, horizontal);
}

void ModernWidget::set_margin(float all) {
    set_margin(all, all, all, all);
}

void ModernWidget::set_padding(float top, float right, float bottom, float left) {
    m_padding_top = top;
    m_padding_right = right;
    m_padding_bottom = bottom;
    m_padding_left = left;
}

void ModernWidget::set_padding(float horizontal, float vertical) {
    set_padding(vertical, horizontal, vertical, horizontal);
}

void ModernWidget::set_padding(float all) {
    set_padding(all, all, all, all);
}

void ModernWidget::set_margin(lshell::design::Spacing margin) {
    set_margin(static_cast<float>(margin));
}

void ModernWidget::set_padding(lshell::design::Spacing padding) {
    set_padding(static_cast<float>(padding));
}

void ModernWidget::set_background_color(lshell::design::Color color) {
    m_background_color = color;
}

void ModernWidget::set_foreground_color(lshell::design::Color color) {
    m_foreground_color = color;
}

void ModernWidget::set_border_color(lshell::design::Color color) {
    m_border_color = color;
}

void ModernWidget::set_text_color(lshell::design::Color color) {
    m_text_color = color;
}

void ModernWidget::set_font_family(const lshell::design::FontFamily& family) {
    m_font_family = family;
}

void ModernWidget::set_font_size(lshell::design::FontSize size) {
    m_font_size = size;
}

void ModernWidget::set_font_weight(lshell::design::FontWeight weight) {
    m_font_weight = weight;
}

void ModernWidget::set_border_width(float width) {
    m_border_width = width;
}

void ModernWidget::set_border_radius(lshell::design::BorderRadius radius) {
    m_border_radius = radius;
}

void ModernWidget::set_halign(WidgetAlign align) {
    m_halign = align;
}

void ModernWidget::set_valign(WidgetAlign align) {
    m_valign = align;
}

void ModernWidget::set_animation(lshell::design::Animation animation) {
    m_animation = animation;
}

void ModernWidget::start_animation() {
    m_animating = true;
    m_animation_progress = 0.0f;
}

void ModernWidget::stop_animation() {
    m_animating = false;
}

void ModernWidget::set_tooltip(const std::string& tooltip) {
    m_tooltip = tooltip;
}

void ModernWidget::set_accessible_name(const std::string& name) {
    m_name = name;
}

void ModernWidget::set_accessible_description(const std::string& description) {
    // In a real implementation, this would be used for accessibility
}

const lshell::design::Theme& ModernWidget::get_theme() const {
    return lshell::design::get_theme();
}

void ModernWidget::state_changed(WidgetState old_state, WidgetState new_state) {
    // Base implementation does nothing
    // Derived classes can override this to handle state changes
}

lshell::design::Color ModernWidget::get_color_for_state(
    lshell::design::Color normal,
    lshell::design::Color hover,
    lshell::design::Color active
) const {
    switch (m_state) {
        case WidgetState::ACTIVE:
            return active;
        case WidgetState::HOVERED:
            return hover;
        case WidgetState::DISABLED:
            return normal.with_alpha(static_cast<uint8_t>(normal.a * 0.5f));
        default:
            return normal;
    }
}

void ModernWidget::apply_theme(const lshell::design::Theme& theme) {
    // Apply default colors from theme
    m_background_color = theme.widget_background;
    m_foreground_color = theme.text_primary;
    m_border_color = theme.widget_border;
    m_text_color = theme.text_primary;

    // Apply default typography
    m_font_family = theme.font_family_primary;
    m_font_size = theme.font_size_sm;
    m_font_weight = lshell::design::FontWeight::REGULAR;

    // Apply default spacing
    set_margin(theme.widget_padding_x, theme.widget_padding_y);
    set_padding(0);

    // Apply default border
    m_border_width = 0;
    m_border_radius = theme.radius_widget;
}

void ModernWidget::on_theme_changed(const lshell::design::Theme& theme) {
    apply_theme(theme);
    // In a real implementation, you would trigger a redraw
    // queue_draw();
}

void ModernWidget::register_theme_listener() {
    lshell::design::ThemeManager::get_instance().add_theme_change_listener(
        [this](const lshell::design::Theme& theme) {
            on_theme_changed(theme);
        }
    );
}

void ModernWidget::unregister_theme_listener() {
    // In a real implementation, you would need to track the listener ID
    // to remove it properly
}

// ============================================================================
// Modern Widget Factory Implementation
// ============================================================================

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_clock_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Clock Widget");
    widget->set_id("widget_clock");
    widget->set_tooltip("Current time");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    // Customize for clock
    widget->set_font_size(theme.font_size_sm);
    widget->set_font_weight(lshell::design::FontWeight::MEDIUM);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_battery_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Battery Widget");
    widget->set_id("widget_battery");
    widget->set_tooltip("Battery status");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_network_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Network Widget");
    widget->set_id("widget_network");
    widget->set_tooltip("Network status");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_volume_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Volume Widget");
    widget->set_id("widget_volume");
    widget->set_tooltip("Volume control");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_workspace_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Workspace Widget");
    widget->set_id("widget_workspace");
    widget->set_tooltip("Workspace switcher");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_window_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Window Widget");
    widget->set_id("widget_window");
    widget->set_tooltip("Active windows");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_media_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Media Widget");
    widget->set_id("widget_media");
    widget->set_tooltip("Media player");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_notification_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Notification Widget");
    widget->set_id("widget_notification");
    widget->set_tooltip("Notifications");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_system_monitor_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("System Monitor Widget");
    widget->set_id("widget_sysmon");
    widget->set_tooltip("System monitor");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_weather_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Weather Widget");
    widget->set_id("widget_weather");
    widget->set_tooltip("Weather information");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_bluetooth_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Bluetooth Widget");
    widget->set_id("widget_bluetooth");
    widget->set_tooltip("Bluetooth status");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_brightness_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Brightness Widget");
    widget->set_id("widget_brightness");
    widget->set_tooltip("Screen brightness");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_keyboard_layout_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Keyboard Layout Widget");
    widget->set_id("widget_keyboard");
    widget->set_tooltip("Keyboard layout");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_nightlight_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Nightlight Widget");
    widget->set_id("widget_nightlight");
    widget->set_tooltip("Nightlight toggle");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_screenshot_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Screenshot Widget");
    widget->set_id("widget_screenshot");
    widget->set_tooltip("Take screenshot");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_power_profile_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Power Profile Widget");
    widget->set_id("widget_power_profile");
    widget->set_tooltip("Power profile");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_clipboard_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Clipboard Widget");
    widget->set_id("widget_clipboard");
    widget->set_tooltip("Clipboard history");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_privacy_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Privacy Widget");
    widget->set_id("widget_privacy");
    widget->set_tooltip("Privacy settings");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_tray_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Tray Widget");
    widget->set_id("widget_tray");
    widget->set_tooltip("System tray");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_spacer_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Spacer Widget");
    widget->set_id("widget_spacer");
    widget->set_tooltip("");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    // Spacer is invisible and just takes up space
    widget->set_background_color(lshell::design::Color(0, 0, 0, 0));
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_calendar_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Calendar Widget");
    widget->set_id("widget_calendar");
    widget->set_tooltip("Calendar");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_date_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Date Widget");
    widget->set_id("widget_date");
    widget->set_tooltip("Current date");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_custom_button_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Custom Button Widget");
    widget->set_id("widget_custom_button");
    widget->set_tooltip("Custom button");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_session_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Session Widget");
    widget->set_id("widget_session");
    widget->set_tooltip("Session actions");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_settings_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Settings Widget");
    widget->set_id("widget_settings");
    widget->set_tooltip("Settings");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

std::unique_ptr<ModernWidget> ModernWidgetFactory::create_control_center_widget() {
    auto widget = std::make_unique<ModernWidget>();
    widget->set_name("Control Center Widget");
    widget->set_id("widget_control_center");
    widget->set_tooltip("Open control center");
    
    const auto& theme = lshell::design::get_theme();
    widget->apply_theme(theme);
    
    return widget;
}

} // namespace lshell::new_impl
