#include "InstancePhysicsEngine.h"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace Client::World {

void InstancePhysicsEngine::PushSnapshot(const MovementSnapshot& snapshot) {
    if (!m_snapshots.empty() && snapshot.timestampMs <= m_snapshots.back().timestampMs) {
        // Simple logic for unordered snapshots: just clear or ignore.
        // Usually, we just append if time is strictly increasing.
        if (snapshot.timestampMs == m_snapshots.back().timestampMs) {
            m_snapshots.back() = snapshot;
            return;
        } else {
            // Out of order, reset and start fresh with this snapshot
            m_snapshots.clear();
        }
    }
    
    m_snapshots.push_back(snapshot);
    if (m_snapshots.size() > MAX_SNAPSHOTS) {
        m_snapshots.pop_front();
    }
}

std::optional<InstancePhysicsEngine::InterpPoints> InstancePhysicsEngine::FindInterpolationPoints(uint64_t targetTimeMs) const noexcept {
    if (m_snapshots.empty()) {
        return std::nullopt;
    }

    if (m_snapshots.size() == 1) {
        return InterpPoints{&m_snapshots.front(), &m_snapshots.front(), 0.0f};
    }

    if (targetTimeMs <= m_snapshots.front().timestampMs) {
        return InterpPoints{&m_snapshots.front(), &m_snapshots.front(), 0.0f};
    }

    if (targetTimeMs >= m_snapshots.back().timestampMs) {
        return InterpPoints{&m_snapshots.back(), &m_snapshots.back(), 0.0f};
    }

    for (size_t i = 0; i < m_snapshots.size() - 1; ++i) {
        const auto& p0 = m_snapshots[i];
        const auto& p1 = m_snapshots[i + 1];

        if (targetTimeMs >= p0.timestampMs && targetTimeMs <= p1.timestampMs) {
            uint64_t dt = p1.timestampMs - p0.timestampMs;
            float t = (dt == 0) ? 0.0f : static_cast<float>(targetTimeMs - p0.timestampMs) / static_cast<float>(dt);
            return InterpPoints{&p0, &p1, t};
        }
    }

    return std::nullopt;
}

Vector3f InstancePhysicsEngine::InterpolatePosition(uint64_t targetTimeMs) const noexcept {
    auto pts = FindInterpolationPoints(targetTimeMs);
    if (!pts) {
        return Vector3f{0.0f, 0.0f, 0.0f};
    }

    const auto& p0 = *pts->p0;
    const auto& p1 = *pts->p1;
    float t = pts->t;

    return Vector3f{
        p0.position.x + (p1.position.x - p0.position.x) * t,
        p0.position.y + (p1.position.y - p0.position.y) * t,
        p0.position.z + (p1.position.z - p0.position.z) * t
    };
}

float InstancePhysicsEngine::InterpolateRotation(uint64_t targetTimeMs) const noexcept {
    auto pts = FindInterpolationPoints(targetTimeMs);
    if (!pts) {
        return 0.0f;
    }

    const auto& p0 = *pts->p0;
    const auto& p1 = *pts->p1;
    float t = pts->t;

    float r0 = p0.rotation;
    float r1 = p1.rotation;

    // Shortest path interpolation (slerp for 1D angle)
    float diff = r1 - r0;
    
    // Normalize diff to [-180, 180]
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    float res = r0 + diff * t;
    
    // Normalize result to [0, 360) or similar, but generally keeping it between -180 and 360 is fine.
    // Standardize to [-180, 180] for consistency:
    while (res > 180.0f) res -= 360.0f;
    while (res < -180.0f) res += 360.0f;

    return res;
}

float InstancePhysicsEngine::CalculateDistanceSq3d(const Vector3f& a, const Vector3f& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

float InstancePhysicsEngine::CalculateDistance2d(const Vector3f& a, const Vector3f& b) noexcept {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool InstancePhysicsEngine::IsInRange(const Vector3f& a, const Vector3f& b, float maxRange) noexcept {
    return CalculateDistanceSq3d(a, b) <= (maxRange * maxRange);
}

void InstancePhysicsEngine::Reset() noexcept {
    m_snapshots.clear();
}

} // namespace Client::World
