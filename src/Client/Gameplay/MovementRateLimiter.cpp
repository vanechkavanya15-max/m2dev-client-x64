#include "MovementRateLimiter.h"

namespace Client::Gameplay {

MovementRateLimiter::MovementRateLimiter(Duration interval)
    : m_interval(interval)
    , m_lastSendTime(TimePoint::min())
    , m_hasSentFirstPacket(false)
{
}

bool MovementRateLimiter::ShouldSendMovementPacket(TimePoint currentTime, bool force) {
    if (force || !m_hasSentFirstPacket) {
        m_lastSendTime = currentTime;
        m_hasSentFirstPacket = true;
        return true;
    }

    if (currentTime - m_lastSendTime >= m_interval) {
        m_lastSendTime = currentTime;
        return true;
    }

    return false;
}

void MovementRateLimiter::Reset() noexcept {
    m_lastSendTime = TimePoint::min();
    m_hasSentFirstPacket = false;
}

} // namespace Client::Gameplay
