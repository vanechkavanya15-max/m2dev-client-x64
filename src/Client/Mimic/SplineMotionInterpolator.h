#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Client::Mimic {

struct Vector3 {
    std::array<float, 3> data;

    constexpr Vector3() : data{0.0f, 0.0f, 0.0f} {}
    constexpr Vector3(float x, float y, float z) : data{x, y, z} {}

    constexpr float x() const { return data[0]; }
    constexpr float y() const { return data[1]; }
    constexpr float z() const { return data[2]; }

    constexpr Vector3 operator+(const Vector3& other) const {
        return Vector3(data[0] + other.data[0], data[1] + other.data[1], data[2] + other.data[2]);
    }

    constexpr Vector3 operator-(const Vector3& other) const {
        return Vector3(data[0] - other.data[0], data[1] - other.data[1], data[2] - other.data[2]);
    }

    constexpr Vector3 operator*(float scalar) const {
        return Vector3(data[0] * scalar, data[1] * scalar, data[2] * scalar);
    }
};

class SplineMotionInterpolator {
public:
    static constexpr size_t kMaxPoints = 64;

    SplineMotionInterpolator() = default;

    bool AddPoint(const Vector3& point);
    void Clear();
    size_t GetPointCount() const;

    // Evaluates the spline position at time t, where t represents the continuous index.
    // e.g., t=0 is the first point, t=1 is the second point.
    Vector3 Evaluate(float t) const;

    // Evaluates the derivative (velocity) at time t.
    Vector3 EvaluateVelocity(float t) const;

private:
    std::array<Vector3, kMaxPoints> m_points{};
    size_t m_pointCount = 0;
};

} // namespace Client::Mimic
