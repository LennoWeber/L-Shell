// L-Shell Design System - Animations
// Smooth, professional animations
// Generated: 2026-10-15
// Author: L-Shell Design Team

#pragma once

#include <cstdint>
#include <functional>

namespace lshell::design {

// ============================================================================
// Timing Functions (Easing)
// ============================================================================

enum class EasingFunction {
    LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_IN_OUT_CUBIC,
    EASE_IN_QUART,
    EASE_OUT_QUART,
    EASE_IN_OUT_QUART,
    EASE_IN_QUINT,
    EASE_OUT_QUINT,
    EASE_IN_OUT_QUINT,
    EASE_IN_SINE,
    EASE_OUT_SINE,
    EASE_IN_OUT_SINE,
    EASE_IN_EXPO,
    EASE_OUT_EXPO,
    EASE_IN_OUT_EXPO,
    EASE_IN_CIRC,
    EASE_OUT_CIRC,
    EASE_IN_OUT_CIRC,
    EASE_IN_ELASTIC,
    EASE_OUT_ELASTIC,
    EASE_IN_OUT_ELASTIC,
    EASE_IN_BACK,
    EASE_OUT_BACK,
    EASE_IN_OUT_BACK,
    EASE_OUT_BOUNCE,
    EASE_IN_BOUNCE,
    EASE_IN_OUT_BOUNCE,
    SPRING
};

/**
 * Get easing function value at progress t (0.0 to 1.0)
 */
float easing_function_value(EasingFunction easing, float t);

// ============================================================================
// Duration
// ============================================================================

/**
 * Animation duration in milliseconds
 */
struct Duration {
    uint32_t ms;

    constexpr Duration(uint32_t milliseconds) : ms(milliseconds) {}

    // Implicit conversion to uint32_t
    constexpr operator uint32_t() const { return ms; }

    // Comparison
    constexpr bool operator<(const Duration& other) const { return ms < other.ms; }
    constexpr bool operator>(const Duration& other) const { return ms > other.ms; }
    constexpr bool operator<=(const Duration& other) const { return ms <= other.ms; }
    constexpr bool operator>=(const Duration& other) const { return ms >= other.ms; }
    constexpr bool operator==(const Duration& other) const { return ms == other.ms; }
    constexpr bool operator!=(const Duration& other) const { return ms != other.ms; }

    // CSS string conversion
    std::string to_css() const;
};

// Duration presets (in milliseconds)
static constexpr Duration DURATION_INSTANT = Duration(0);
static constexpr Duration DURATION_FASTEST = Duration(50);
static constexpr Duration DURATION_FASTER = Duration(100);
static constexpr Duration DURATION_FAST = Duration(150);
static constexpr Duration DURATION_NORMAL = Duration(200);
static constexpr Duration DURATION_SLOW = Duration(300);
static constexpr Duration DURATION_SLOWER = Duration(400);
static constexpr Duration DURATION_SLOWEST = Duration(500);
static constexpr Duration DURATION_LONG = Duration(1000);

// ============================================================================
// Animation Properties
// ============================================================================

enum class AnimatableProperty {
    OPACITY,
    SCALE,
    SCALE_X,
    SCALE_Y,
    POSITION_X,
    POSITION_Y,
    SIZE_WIDTH,
    SIZE_HEIGHT,
    ROTATION,
    COLOR,
    BACKGROUND_COLOR,
    BORDER_COLOR,
    BORDER_WIDTH,
    BORDER_RADIUS,
    SHADOW,
    BLUR,
    TRANSFORM,
    CUSTOM
};

// ============================================================================
// Animation Definition
// ============================================================================

/**
 * Single animation keyframe
 */
struct AnimationKeyframe {
    float progress;  // 0.0 to 1.0
    float value;     // Value at this keyframe
    EasingFunction easing;  // Easing to use after this keyframe

    AnimationKeyframe(float progress, float value, EasingFunction easing = EasingFunction::LINEAR)
        : progress(progress), value(value), easing(easing) {}
};

/**
 * Complete animation definition
 */
struct Animation {
    Duration duration;
    EasingFunction easing;
    uint32_t delay;  // Delay in milliseconds
    bool fill_forwards;  // Keep final state after animation ends
    bool fill_backwards; // Apply initial state before animation starts
    uint32_t iterations; // Number of iterations (0 = infinite)
    bool alternate;      // Alternate direction on each iteration

    // Keyframes (optional, if not using simple easing)
    std::vector<AnimationKeyframe> keyframes;

    Animation(
        Duration duration = DURATION_NORMAL,
        EasingFunction easing = EasingFunction::EASE_OUT_QUAD,
        uint32_t delay = 0,
        bool fill_forwards = true,
        bool fill_backwards = false,
        uint32_t iterations = 1,
        bool alternate = false
    ) : duration(duration), easing(easing), delay(delay),
        fill_forwards(fill_forwards), fill_backwards(fill_backwards),
        iterations(iterations), alternate(alternate) {}

    // Simple animation with keyframes
    static Animation with_keyframes(
        const std::vector<AnimationKeyframe>& keyframes,
        Duration duration = DURATION_NORMAL,
        uint32_t delay = 0
    ) {
        Animation anim(duration, EasingFunction::LINEAR, delay);
        anim.keyframes = keyframes;
        return anim;
    }

    // CSS string conversion
    std::string to_css() const;
};

// ============================================================================
// Predefined Animations
// ============================================================================

// -- Fade Animations --
static constexpr Animation ANIM_FADE_IN = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_FADE_IN_SLOW = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_FADE_OUT = {
    DURATION_FAST,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_FADE_OUT_SLOW = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_FADE_TOGGLE = {
    DURATION_FAST,
    EasingFunction::EASE_IN_OUT_QUAD,
    0,
    true,
    false,
    0, // Infinite
    true // Alternate
};

// -- Scale Animations --
static constexpr Animation ANIM_SCALE_IN = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_BACK,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SCALE_OUT = {
    DURATION_FAST,
    EasingFunction::EASE_IN_BACK,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SCALE_PULSE = {
    DURATION_SLOW,
    EasingFunction::EASE_IN_OUT_SINE,
    0,
    true,
    false,
    0, // Infinite
    true // Alternate
};

// -- Slide Animations --
static constexpr Animation ANIM_SLIDE_IN_FROM_RIGHT = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_IN_FROM_LEFT = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_IN_FROM_TOP = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_IN_FROM_BOTTOM = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_OUT_TO_RIGHT = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_OUT_TO_LEFT = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false,
    1,
    false
};

// -- Combined Animations --
static constexpr Animation ANIM_SLIDE_FADE_IN_FROM_RIGHT = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

static constexpr Animation ANIM_SLIDE_FADE_OUT_TO_RIGHT = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false,
    1,
    false
};

// -- Button Hover Animation --
static constexpr Animation ANIM_BUTTON_HOVER = {
    DURATION_FASTEST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false,
    1,
    false
};

// -- Spring Animation --
static constexpr Animation ANIM_SPRING = {
    DURATION_SLOW,
    EasingFunction::SPRING,
    0,
    true,
    false,
    1,
    false
};

// -- Bounce Animation --
static constexpr Animation ANIM_BOUNCE = {
    DURATION_SLOW,
    EasingFunction::EASE_OUT_BOUNCE,
    0,
    true,
    false,
    1,
    false
};

// -- Shake Animation (for errors) --
static constexpr Animation ANIM_SHAKE = {
    DURATION_SLOW,
    EasingFunction::EASE_IN_OUT_SINE,
    0,
    true,
    false,
    3,
    true
};

// ============================================================================
// Component-specific Animations
// ============================================================================

namespace component_animations {

// Bar animations
static constexpr Animation BAR_SHOW = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation BAR_HIDE = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false
};

// Control Center animations
static constexpr Animation CC_OPEN = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_CUBIC,
    0,
    true,
    false
};

static constexpr Animation CC_CLOSE = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_CUBIC,
    0,
    true,
    false
};

static constexpr Animation CC_TAB_SWITCH = {
    DURATION_FAST,
    EasingFunction::EASE_IN_OUT_QUAD,
    0,
    true,
    false
};

// Widget animations
static constexpr Animation WIDGET_HOVER = {
    DURATION_FASTEST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation WIDGET_ACTIVE = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

// Button animations
static constexpr Animation BUTTON_PRESS = {
    DURATION_FASTEST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation BUTTON_RELEASE = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_ELASTIC,
    0,
    true,
    false
};

// Input animations
static constexpr Animation INPUT_FOCUS = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation INPUT_BLUR = {
    DURATION_FAST,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false
};

// Tooltip animations
static constexpr Animation TOOLTIP_SHOW = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    300, // 300ms delay
    true,
    false
};

static constexpr Animation TOOLTIP_HIDE = {
    DURATION_FAST,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false
};

// Context menu animations
static constexpr Animation CONTEXT_MENU_OPEN = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation CONTEXT_MENU_CLOSE = {
    DURATION_FAST,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false
};

// Modal animations
static constexpr Animation MODAL_OPEN = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_BACK,
    0,
    true,
    false
};

static constexpr Animation MODAL_CLOSE = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_BACK,
    0,
    true,
    false
};

// Dropdown animations
static constexpr Animation DROPDOWN_OPEN = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

static constexpr Animation DROPDOWN_CLOSE = {
    DURATION_FAST,
    EasingFunction::EASE_IN_QUAD,
    0,
    true,
    false
};

// Notification animations
static constexpr Animation NOTIFICATION_SHOW = {
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_BACK,
    0,
    true,
    false
};

static constexpr Animation NOTIFICATION_HIDE = {
    DURATION_NORMAL,
    EasingFunction::EASE_IN_BACK,
    0,
    true,
    false
};

// Progress animations
static constexpr Animation PROGRESS_INDETERMINATE = {
    DURATION_LONG,
    EasingFunction::LINEAR,
    0,
    true,
    false,
    0, // Infinite
    false
};

// Loading spinner
static constexpr Animation SPINNER_ROTATE = {
    DURATION_LONG,
    EasingFunction::LINEAR,
    0,
    true,
    false,
    0, // Infinite
    false
};

// Switch toggle
static constexpr Animation SWITCH_TOGGLE = {
    DURATION_FAST,
    EasingFunction::EASE_IN_OUT_CUBIC,
    0,
    true,
    false
};

// Checkbox/Radio
static constexpr Animation CHECKBOX_CHECK = {
    DURATION_FAST,
    EasingFunction::EASE_OUT_ELASTIC,
    0,
    true,
    false
};

// Slider thumb
static constexpr Animation SLIDER_DRAG = {
    DURATION_FASTEST,
    EasingFunction::EASE_OUT_QUAD,
    0,
    true,
    false
};

} // namespace component_animations

// ============================================================================
// Animation Timing Presets
// ============================================================================

/**
 * Timing configuration for consistent animation feel
 */
struct AnimationTiming {
    Duration enter;
    Duration exit;
    EasingFunction enter_easing;
    EasingFunction exit_easing;
    uint32_t delay;

    AnimationTiming(
        Duration enter = DURATION_NORMAL,
        Duration exit = DURATION_NORMAL,
        EasingFunction enter_easing = EasingFunction::EASE_OUT_QUAD,
        EasingFunction exit_easing = EasingFunction::EASE_IN_QUAD,
        uint32_t delay = 0
    ) : enter(enter), exit(exit), enter_easing(enter_easing),
        exit_easing(exit_easing), delay(delay) {}
};

static constexpr AnimationTiming TIMING_DEFAULT = {
    DURATION_NORMAL,
    DURATION_NORMAL,
    EasingFunction::EASE_OUT_QUAD,
    EasingFunction::EASE_IN_QUAD
};

static constexpr AnimationTiming TIMING_FAST = {
    DURATION_FAST,
    DURATION_FAST,
    EasingFunction::EASE_OUT_QUAD,
    EasingFunction::EASE_IN_QUAD
};

static constexpr AnimationTiming TIMING_SLOW = {
    DURATION_SLOW,
    DURATION_SLOW,
    EasingFunction::EASE_OUT_CUBIC,
    EasingFunction::EASE_IN_CUBIC
};

static constexpr AnimationTiming TIMING_ELASTIC = {
    DURATION_SLOW,
    DURATION_SLOW,
    EasingFunction::EASE_OUT_ELASTIC,
    EasingFunction::EASE_IN_ELASTIC
};

// ============================================================================
// Animation Callbacks
// ============================================================================

using AnimationStartCallback = std::function<void()>;
using AnimationEndCallback = std::function<void()>;
using AnimationUpdateCallback = std::function<void(float progress)>;

/**
 * Animation event handlers
 */
struct AnimationCallbacks {
    AnimationStartCallback on_start;
    AnimationEndCallback on_end;
    AnimationUpdateCallback on_update;
};

} // namespace lshell::design
