// L-Shell Modern Widget - Base class for all new widgets
// Uses the new L-Shell Design System
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include "shell/design/theme.h"
#include "shell/design/colors.h"
#include "shell/design/typography.h"
#include "shell/design/spacing.h"
#include "shell/design/animations.h"

#include <string>
#include <functional>
#include <memory>

namespace lshell::new_impl {

// Forward declarations
class ModernBar;
class ModernWidgetContainer;

// ============================================================================
// Widget State
// ============================================================================

enum class WidgetState {
    NORMAL,
    HOVERED,
    ACTIVE,
    DISABLED,
    FOCUSED
};

// ============================================================================
// Widget Alignment
// ============================================================================

enum class WidgetAlign {
    START,
    CENTER,
    END
};

// ============================================================================
// Modern Widget - Base class for all new widgets
// ============================================================================

class ModernWidget {
public:
    using ClickCallback = std::function<void(ModernWidget*)>;
    using HoverCallback = std::function<void(ModernWidget*, bool)>;
    using UpdateCallback = std::function<void(ModernWidget*)>;

    ModernWidget();
    virtual ~ModernWidget();

    // Identification
    virtual std::string get_id() const { return m_id; }
    virtual std::string get_name() const { return m_name; }
    void set_id(const std::string& id) { m_id = id; }
    void set_name(const std::string& name) { m_name = name; }

    // State
    WidgetState get_state() const { return m_state; }
    void set_state(WidgetState state);
    bool is_hovered() const { return m_state == WidgetState::HOVERED; }
    bool is_active() const { return m_state == WidgetState::ACTIVE; }
    bool is_disabled() const { return m_state == WidgetState::DISABLED; }

    // Visibility
    bool is_visible() const { return m_visible; }
    void set_visible(bool visible) { m_visible = visible; }
    void show() { set_visible(true); }
    void hide() { set_visible(false); }

    // Size and position
    virtual float get_width() const = 0;
    virtual float get_height() const = 0;
    virtual void set_size(float width, float height) = 0;
    virtual void set_position(float x, float y) = 0;
    virtual void set_min_size(float min_width, float min_height) = 0;

    // Margins and padding
    void set_margin(float top, float right, float bottom, float left);
    void set_margin(float horizontal, float vertical);
    void set_margin(float all);
    void set_padding(float top, float right, float bottom, float left);
    void set_padding(float horizontal, float vertical);
    void set_padding(float all);

    // Spacing from design system
    void set_margin(lshell::design::Spacing margin);
    void set_padding(lshell::design::Spacing padding);

    // Colors from design system
    void set_background_color(lshell::design::Color color);
    void set_foreground_color(lshell::design::Color color);
    void set_border_color(lshell::design::Color color);
    void set_text_color(lshell::design::Color color);

    // Typography from design system
    void set_font_family(const lshell::design::FontFamily& family);
    void set_font_size(lshell::design::FontSize size);
    void set_font_weight(lshell::design::FontWeight weight);

    // Border
    void set_border_width(float width);
    void set_border_radius(lshell::design::BorderRadius radius);

    // Alignment
    void set_halign(WidgetAlign align);
    void set_valign(WidgetAlign align);

    // Callbacks
    void set_click_callback(ClickCallback callback) { m_click_callback = callback; }
    void set_hover_callback(HoverCallback callback) { m_hover_callback = callback; }
    void set_update_callback(UpdateCallback callback) { m_update_callback = callback; }

    // Event handlers
    virtual void on_click() {
        if (m_click_callback) m_click_callback(this);
    }
    virtual void on_hover(bool entered) {
        set_state(entered ? WidgetState::HOVERED : WidgetState::NORMAL);
        if (m_hover_callback) m_hover_callback(this, entered);
    }
    virtual void on_press() { set_state(WidgetState::ACTIVE); }
    virtual void on_release() { set_state(WidgetState::HOVERED); }
    virtual void on_focus() { set_state(WidgetState::FOCUSED); }
    virtual void on_blur() { set_state(WidgetState::NORMAL); }

    // Theme handling
    virtual void apply_theme(const lshell::design::Theme& theme);
    virtual void on_theme_changed(const lshell::design::Theme& theme);

    // Layout
    virtual void layout(float container_width, float container_height) = 0;
    virtual void update() = 0;
    virtual void render() = 0;

    // Animation
    void set_animation(lshell::design::Animation animation);
    void start_animation();
    void stop_animation();
    bool is_animating() const { return m_animating; }

    // Toolip
    void set_tooltip(const std::string& tooltip);
    std::string get_tooltip() const { return m_tooltip; }

    // Accessibility
    void set_accessible_name(const std::string& name);
    void set_accessible_description(const std::string& description);

protected:
    // Theme access
    const lshell::design::Theme& get_theme() const;

    // State change
    virtual void state_changed(WidgetState old_state, WidgetState new_state);

    // Design system helpers
    lshell::design::Color get_color_for_state(
        lshell::design::Color normal,
        lshell::design::Color hover,
        lshell::design::Color active
    ) const;

    // Members
    std::string m_id;
    std::string m_name;
    WidgetState m_state = WidgetState::NORMAL;
    bool m_visible = true;
    bool m_enabled = true;

    // Callbacks
    ClickCallback m_click_callback;
    HoverCallback m_hover_callback;
    UpdateCallback m_update_callback;

    // Tooltip
    std::string m_tooltip;

    // Animation
    lshell::design::Animation m_animation;
    bool m_animating = false;
    float m_animation_progress = 0.0f;

    // Margins and padding
    float m_margin_top = 0;
    float m_margin_right = 0;
    float m_margin_bottom = 0;
    float m_margin_left = 0;
    float m_padding_top = 0;
    float m_padding_right = 0;
    float m_padding_bottom = 0;
    float m_padding_left = 0;

    // Colors
    lshell::design::Color m_background_color;
    lshell::design::Color m_foreground_color;
    lshell::design::Color m_border_color;
    lshell::design::Color m_text_color;

    // Typography
    lshell::design::FontFamily m_font_family;
    lshell::design::FontSize m_font_size;
    lshell::design::FontWeight m_font_weight;

    // Border
    float m_border_width = 0;
    lshell::design::BorderRadius m_border_radius;

    // Alignment
    WidgetAlign m_halign = WidgetAlign::START;
    WidgetAlign m_valign = WidgetAlign::START;

    // Theme listener
    void register_theme_listener();
    void unregister_theme_listener();
};

// ============================================================================
// Modern Widget Factory
// ============================================================================

class ModernWidgetFactory {
public:
    static std::unique_ptr<ModernWidget> create_clock_widget();
    static std::unique_ptr<ModernWidget> create_battery_widget();
    static std::unique_ptr<ModernWidget> create_network_widget();
    static std::unique_ptr<ModernWidget> create_volume_widget();
    static std::unique_ptr<ModernWidget> create_workspace_widget();
    static std::unique_ptr<ModernWidget> create_window_widget();
    static std::unique_ptr<ModernWidget> create_media_widget();
    static std::unique_ptr<ModernWidget> create_notification_widget();
    static std::unique_ptr<ModernWidget> create_system_monitor_widget();
    static std::unique_ptr<ModernWidget> create_weather_widget();
    static std::unique_ptr<ModernWidget> create_bluetooth_widget();
    static std::unique_ptr<ModernWidget> create_brightness_widget();
    static std::unique_ptr<ModernWidget> create_keyboard_layout_widget();
    static std::unique_ptr<ModernWidget> create_nightlight_widget();
    static std::unique_ptr<ModernWidget> create_screenshot_widget();
    static std::unique_ptr<ModernWidget> create_power_profile_widget();
    static std::unique_ptr<ModernWidget> create_clipboard_widget();
    static std::unique_ptr<ModernWidget> create_privacy_widget();
    static std::unique_ptr<ModernWidget> create_tray_widget();
    static std::unique_ptr<ModernWidget> create_spacer_widget();
    static std::unique_ptr<ModernWidget> create_calendar_widget();
    static std::unique_ptr<ModernWidget> create_date_widget();
    static std::unique_ptr<ModernWidget> create_custom_button_widget();
    static std::unique_ptr<ModernWidget> create_session_widget();
    static std::unique_ptr<ModernWidget> create_settings_widget();
    static std::unique_ptr<ModernWidget> create_control_center_widget();
};

} // namespace lshell::new_impl
