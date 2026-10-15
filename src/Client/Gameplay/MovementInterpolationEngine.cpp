#include "StdAfx.h"
#include "Client/Gameplay/MovementInterpolationEngine.h"
#include <algorithm>
#include <cmath>

namespace Client::Gameplay {

void MovementInterpolationEngine::AddSnapshot(const Client::Core::MapCoords& position,
                                              const Client::Core::MapCoords& velocity,
                                              float rotation,
                                              uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Maintain a simple history of the last 2 snapshots for interpolation
    m_snapshots.push_back({position, velocity, rotation, timestamp});
    
    // Sort just in case they arrive out of order
    std::sort(m_snapshots.begin(), m_snapshots.end(), [](const Snapshot& a, const Snapshot& b) {
        return a.timestamp < b.timestamp;
    });

    if (m_snapshots.size() > 2) {
        m_snapshots.erase(m_snapshots.begin());
    }
}

Client::Core::MapCoords MovementInterpolationEngine::GetInterpolatedPosition(uint64_t timestamp) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_snapshots.empty()) {
        return {};
    }

    if (m_snapshots.size() == 1 || timestamp >= m_snapshots.back().timestamp) {
        // Dead-reckoning from the last known state
        const auto& last = m_snapshots.back();
        if (timestamp < last.timestamp) {
            return last.position; // Time in the past, just return position
        }
        
        float dt = static_cast<float>(timestamp - last.timestamp) / 1000.0f; // Assuming ms
        return last.position + (last.velocity * dt);
    }

    const auto& prev = m_snapshots[0];
    const auto& next = m_snapshots[1];

    if (timestamp <= prev.timestamp) {
        return prev.position;
    }

    float t = static_cast<float>(timestamp - prev.timestamp) / static_cast<float>(next.timestamp - prev.timestamp);
    
    // Dead reckoning positions
    float dt0 = static_cast<float>(timestamp - prev.timestamp) / 1000.0f;
    float dt1 = -static_cast<float>(next.timestamp - timestamp) / 1000.0f;
    
    Client::Core::MapCoords p0 = prev.position + (prev.velocity * dt0);
    Client::Core::MapCoords p1 = next.position + (next.velocity * dt1);

    // LERP between the dead-reckoned positions
    return {
        std::lerp(p0.x, p1.x, t),
        std::lerp(p0.y, p1.y, t),
        std::lerp(p0.z, p1.z, t)
    };
}

float MovementInterpolationEngine::SlerpRotation(float currentYaw, float targetYaw, float alpha) const {
    // Normalize angles to [0, 360)
    currentYaw = std::fmod(currentYaw, 360.0f);
    if (currentYaw < 0.0f) currentYaw += 360.0f;
    
    targetYaw = std::fmod(targetYaw, 360.0f);
    if (targetYaw < 0.0f) targetYaw += 360.0f;

    float diff = targetYaw - currentYaw;

    // Shortest path
    if (diff > 180.0f) diff -= 360.0f;
    else if (diff < -180.0f) diff += 360.0f;

    float result = currentYaw + diff * alpha;

    result = std::fmod(result, 360.0f);
    if (result < 0.0f) result += 360.0f;

    return result;
}

float MovementInterpolationEngine::GetInterpolatedRotation(uint64_t timestamp) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_snapshots.empty()) {
        return 0.0f;
    }

    if (m_snapshots.size() == 1 || timestamp >= m_snapshots.back().timestamp) {
        return m_snapshots.back().rotation;
    }

    const auto& prev = m_snapshots[0];
    const auto& next = m_snapshots[1];

    if (timestamp <= prev.timestamp) {
        return prev.rotation;
    }

    float t = static_cast<float>(timestamp - prev.timestamp) / static_cast<float>(next.timestamp - prev.timestamp);

    return SlerpRotation(prev.rotation, next.rotation, t);
}

void MovementInterpolationEngine::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshots.clear();
}

} // namespace Client::Gameplay
