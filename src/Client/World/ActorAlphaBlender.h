#pragma once

#include "StdAfx.h"
#include <cstdint>
#include <chrono>
#include <expected>
#include <string_view>
#include "EterBase/Result.h"

namespace Client::World {

enum class BlendEasing : uint8_t {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad
};

struct AlphaBlendConfig {
    std::chrono::milliseconds duration{1000};
    BlendEasing easing{BlendEasing::Linear};
};

struct AlphaBlendState {
    float currentAlpha{1.0f};
    bool isVisible{true};
    bool isBlending{false};
};

class ActorAlphaBlender {
public:
    ActorAlphaBlender() noexcept = default;
    ~ActorAlphaBlender() = default;

    ActorAlphaBlender(const ActorAlphaBlender&) = default;
    ActorAlphaBlender& operator=(const ActorAlphaBlender&) = default;
    ActorAlphaBlender(ActorAlphaBlender&&) noexcept = default;
    ActorAlphaBlender& operator=(ActorAlphaBlender&&) noexcept = default;

    EterBase::VoidResult<> StartFadeIn(const AlphaBlendConfig& config) noexcept;
    EterBase::VoidResult<> StartFadeOut(const AlphaBlendConfig& config) noexcept;

    void Update(std::chrono::milliseconds deltaTime) noexcept;
    void CancelTransition() noexcept;

    [[nodiscard]] const AlphaBlendState& GetState() const noexcept { return m_state; }

private:
    [[nodiscard]] float CalculateEasing(float progress, BlendEasing easing) const noexcept;

    AlphaBlendState m_state{};
    AlphaBlendConfig m_currentConfig{};
    
    std::chrono::milliseconds m_elapsedTime{0};
    float m_startAlpha{1.0f};
    float m_targetAlpha{1.0f};
};

} // namespace Client::World
