// L-Shell Animations - Modern Implementation
// Ultra-modern, professional design for Fedora + Hyprland
// Part of the complete redesign 2026

#include "shell/design/animations.h"
#include "shell/design/theme.h"

namespace lshell::design {

// ============================================================================
// Animation Implementation
// ============================================================================

Animation::Animation() 
    : duration(std::chrono::milliseconds(200)),
      easing(Easing::Linear),
      running(false),
      progress(0.0f),
      start_time(std::chrono::steady_clock::now()) {
}

Animation::Animation(AnimationPreset preset)
    : Animation() {
    
    switch (preset) {
        case AnimationPreset::Fast:
            duration = std::chrono::milliseconds(100);
            easing = Easing::OutQuad;
            break;
        case AnimationPreset::Normal:
            duration = std::chrono::milliseconds(200);
            easing = Easing::OutQuad;
            break;
        case AnimationPreset::Slow:
            duration = std::chrono::milliseconds(300);
            easing = Easing::OutQuad;
            break;
        case AnimationPreset::Enter:
            duration = std::chrono::milliseconds(250);
            easing = Easing::OutBack;
            break;
        case AnimationPreset::Exit:
            duration = std::chrono::milliseconds(200);
            easing = Easing::InBack;
            break;
        case AnimationPreset::Bounce:
            duration = std::chrono::milliseconds(300);
            easing = Easing::OutBounce;
            break;
        case AnimationPreset::Fade:
            duration = std::chrono::milliseconds(200);
            easing = Easing::Linear;
            break;
    }
}

void Animation::start() {
    if (running) return;
    
    running = true;
    progress = 0.0f;
    start_time = std::chrono::steady_clock::now();
}

void Animation::stop() {
    running = false;
    progress = 0.0f;
}

void Animation::reset() {
    running = false;
    progress = 0.0f;
}

bool Animation::is_running() const {
    return running;
}

bool Animation::is_complete() const {
    return !running && progress >= 1.0f;
}

float Animation::get_progress() const {
    return progress;
}

float Animation::get_eased_progress() const {
    return apply_easing(progress, easing);
}

void Animation::update() {
    if (!running) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    
    if (elapsed >= duration) {
        progress = 1.0f;
        running = false;
    } else {
        progress = static_cast<float>(elapsed.count()) / static_cast<float>(duration.count());
    }
}

// ============================================================================
// Easing Functions
// ============================================================================

float Animation::apply_easing(float t, Easing easing) const {
    switch (easing) {
        case Easing::Linear:
            return t;
        
        case Easing::InQuad:
            return t * t;
        
        case Easing::OutQuad:
            return t * (2.0f - t);
        
        case Easing::InOutQuad:
            if (t < 0.5f) return 2.0f * t * t;
            return -1.0f + (4.0f - 2.0f * t) * t;
        
        case Easing::InCubic:
            return t * t * t;
        
        case Easing::OutCubic:
            return (--t) * t * t + 1.0f;
        
        case Easing::InOutCubic:
            if (t < 0.5f) return 4.0f * t * t * t;
            return (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        
        case Easing::InBack:
            return t * t * ((1.70158f + 1.0f) * t - 1.70158f);
        
        case Easing::OutBack:
            return (t - 1.0f) * (t - 1.0f) * ((1.70158f + 1.0f) * (t - 1.0f) + 1.70158f) + 1.0f;
        
        case Easing::InOutBack:
            if (t < 0.5f) {
                float s = 1.70158f * 1.525f;
                return t * t * ((s + 1.0f) * t - s);
            }
            float s = 1.70158f * 1.525f;
            return (t - 1.0f) * (t - 1.0f) * ((s + 1.0f) * (t - 1.0f) + s) + 1.0f;
        
        case Easing::InBounce:
            return 1.0f - ease_out_bounce(1.0f - t);
        
        case Easing::OutBounce:
            return ease_out_bounce(t);
        
        case Easing::InOutBounce:
            if (t < 0.5f) return 0.5f * ease_in_bounce(t * 2.0f);
            return 0.5f * ease_out_bounce(t * 2.0f - 1.0f) + 0.5f;
        
        case Easing::InElastic:
            return ease_in_elastic(t);
        
        case Easing::OutElastic:
            return ease_out_elastic(t);
        
        case Easing::InOutElastic:
            if (t < 0.5f) return 0.5f * ease_in_elastic(t * 2.0f);
            return 0.5f * ease_out_elastic(t * 2.0f - 1.0f) + 0.5f;
        
        default:
            return t;
    }
}

// Helper functions for complex easings

float Animation::ease_out_bounce(float t) const {
    if (t < (1.0f / 2.75f)) {
        return (7.5625f * t * t);
    } else if (t < (2.0f / 2.75f)) {
        return (7.5625f * (t -= (1.5f / 2.75f)) * t + 0.75f);
    } else if (t < (2.5f / 2.75f)) {
        return (7.5625f * (t -= (2.25f / 2.75f)) * t + 0.9375f);
    } else {
        return (7.5625f * (t -= (2.625f / 2.75f)) * t + 0.984375f);
    }
}

float Animation::ease_in_bounce(float t) const {
    return 1.0f - ease_out_bounce(1.0f - t);
}

float Animation::ease_out_elastic(float t) const {
    float p = 0.3f;
    float s = p / 4.0f;
    return 1.0f - std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * M_PI) / p);
}

float Animation::ease_in_elastic(float t) const {
    float p = 0.3f;
    float s = p / 4.0f;
    return -std::pow(2.0f, 10.0f * (t -= 1.0f)) * std::sin((t - s) * (2.0f * M_PI) / p);
}

// ============================================================================
// AnimationManager Implementation
// ============================================================================

AnimationManager& AnimationManager::get() {
    static AnimationManager instance;
    return instance;
}

void AnimationManager::add_animation(Animation* animation) {
    std::lock_guard lock(m_mutex);
    m_animations.insert(animation);
}

void AnimationManager::remove_animation(Animation* animation) {
    std::lock_guard lock(m_mutex);
    m_animations.erase(animation);
}

void AnimationManager::update_all() {
    std::lock_guard lock(m_mutex);
    for (auto* animation : m_animations) {
        animation->update();
    }
    
    // Remove completed animations
    auto it = m_animations.begin();
    while (it != m_animations.end()) {
        if ((*it)->is_complete()) {
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }
}

bool AnimationManager::has_active_animations() const {
    std::lock_guard lock(m_mutex);
    for (auto* animation : m_animations) {
        if (animation->is_running()) {
            return true;
        }
    }
    return false;
}

} // namespace lshell::design
