#pragma once

#include <cstdint>
#include <deque>
#include <optional>

namespace Client::World {

struct Vector3f {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

struct MovementSnapshot {
    uint64_t timestampMs{0};
    Vector3f position{};
    float rotation{0.0f};
};

class InstancePhysicsEngine {
public:
    InstancePhysicsEngine() = default;
    ~InstancePhysicsEngine() = default;

    InstancePhysicsEngine(const InstancePhysicsEngine&) = delete;
    InstancePhysicsEngine& operator=(const InstancePhysicsEngine&) = delete;

    void PushSnapshot(const MovementSnapshot& snapshot);
    
    [[nodiscard]] Vector3f InterpolatePosition(uint64_t targetTimeMs) const noexcept;
    [[nodiscard]] float InterpolateRotation(uint64_t targetTimeMs) const noexcept;
    
    [[nodiscard]] static float CalculateDistanceSq3d(const Vector3f& a, const Vector3f& b) noexcept;
    [[nodiscard]] static float CalculateDistance2d(const Vector3f& a, const Vector3f& b) noexcept;
    [[nodiscard]] static bool IsInRange(const Vector3f& a, const Vector3f& b, float maxRange) noexcept;
    
    void Reset() noexcept;

private:
    std::deque<MovementSnapshot> m_snapshots;
    static constexpr size_t MAX_SNAPSHOTS = 32;

    struct InterpPoints {
        const MovementSnapshot* p0;
        const MovementSnapshot* p1;
        float t;
    };

    [[nodiscard]] std::optional<InterpPoints> FindInterpolationPoints(uint64_t targetTimeMs) const noexcept;
};

} // namespace Client::World
