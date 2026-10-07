#include "StdAfx.h"
#include "ActorAlphaBlender.h"
#include "EterBase/LogModern.h"
#include <algorithm>
#include <cmath>

namespace Client::World {

EterBase::VoidResult<> ActorAlphaBlender::StartFadeIn(const AlphaBlendConfig& config) noexcept {
    if (config.duration.count() <= 0) {
        EterBase::ModernLogger::Error("ActorAlphaBlender::StartFadeIn: Duration must be > 0");
        return std::unexpected("Duration must be > 0");
    }

    m_currentConfig = config;
    m_elapsedTime = std::chrono::milliseconds(0);
    
    // Default alpha guaranteed to be 1.0f (or transitioning towards it)
    m_startAlpha = m_state.currentAlpha;
    m_targetAlpha = 1.0f;
    
    m_state.isBlending = true;
    m_state.isVisible = true; // Ensure visible when fading in

    EterBase::ModernLogger::Debug("ActorAlphaBlender::StartFadeIn started.");
    return {};
}

EterBase::VoidResult<> ActorAlphaBlender::StartFadeOut(const AlphaBlendConfig& config) noexcept {
    if (config.duration.count() <= 0) {
        EterBase::ModernLogger::Error("ActorAlphaBlender::StartFadeOut: Duration must be > 0");
        return std::unexpected("Duration must be > 0");
    }

    m_currentConfig = config;
    m_elapsedTime = std::chrono::milliseconds(0);
    
    m_startAlpha = m_state.currentAlpha;
    m_targetAlpha = 0.0f;
    
    m_state.isBlending = true;
    m_state.isVisible = true;

    EterBase::ModernLogger::Debug("ActorAlphaBlender::StartFadeOut started.");
    return {};
}

void ActorAlphaBlender::Update(std::chrono::milliseconds deltaTime) noexcept {
    if (!m_state.isBlending) {
        return;
    }

    m_elapsedTime += deltaTime;

    if (m_elapsedTime >= m_currentConfig.duration) {
        m_state.currentAlpha = m_targetAlpha;
        m_state.isBlending = false;
        
        // Enforce isVisible = true at end of transition as per requirements
        m_state.isVisible = true;
        
        EterBase::ModernLogger::Debug("ActorAlphaBlender::Update transition complete.");
        return;
    }

    float progress = static_cast<float>(m_elapsedTime.count()) / static_cast<float>(m_currentConfig.duration.count());
    progress = std::clamp(progress, 0.0f, 1.0f);

    float easedProgress = CalculateEasing(progress, m_currentConfig.easing);
    
    m_state.currentAlpha = m_startAlpha + (m_targetAlpha - m_startAlpha) * easedProgress;
}

void ActorAlphaBlender::CancelTransition() noexcept {
    if (m_state.isBlending) {
        m_state.isBlending = false;
        m_state.isVisible = true;
        m_state.currentAlpha = 1.0f; // Default alpha guaranteed on 1.0f
        EterBase::ModernLogger::Debug("ActorAlphaBlender::CancelTransition.");
    }
}

float ActorAlphaBlender::CalculateEasing(float progress, BlendEasing easing) const noexcept {
    switch (easing) {
        case BlendEasing::Linear:
            return progress;
        case BlendEasing::EaseInQuad:
            return progress * progress;
        case BlendEasing::EaseOutQuad:
            return progress * (2.0f - progress);
        case BlendEasing::EaseInOutQuad:
            return progress < 0.5f ? 2.0f * progress * progress : -1.0f + (4.0f - 2.0f * progress) * progress;
        default:
            return progress;
    }
}

} // namespace Client::World
