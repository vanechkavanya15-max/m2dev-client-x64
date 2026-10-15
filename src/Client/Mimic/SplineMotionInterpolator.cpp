#include "SplineMotionInterpolator.h"
#include <algorithm>
#include <cmath>

namespace Client::Mimic {

bool SplineMotionInterpolator::AddPoint(const Vector3& point) {
    if (m_pointCount >= kMaxPoints) {
        return false;
    }
    m_points[m_pointCount++] = point;
    return true;
}

void SplineMotionInterpolator::Clear() {
    m_pointCount = 0;
}

size_t SplineMotionInterpolator::GetPointCount() const {
    return m_pointCount;
}

Vector3 SplineMotionInterpolator::Evaluate(float t) const {
    if (m_pointCount == 0) {
        return Vector3();
    }
    if (m_pointCount == 1) {
        return m_points[0];
    }

    t = std::max(0.0f, std::min(t, static_cast<float>(m_pointCount - 1)));
    
    int p = static_cast<int>(std::floor(t));
    float u = t - static_cast<float>(p);

    if (p >= static_cast<int>(m_pointCount) - 1) {
        return m_points[m_pointCount - 1];
    }

    int p0 = std::max(0, p - 1);
    int p1 = p;
    int p2 = std::min(static_cast<int>(m_pointCount) - 1, p + 1);
    int p3 = std::min(static_cast<int>(m_pointCount) - 1, p + 2);

    const Vector3& v0 = m_points[p0];
    const Vector3& v1 = m_points[p1];
    const Vector3& v2 = m_points[p2];
    const Vector3& v3 = m_points[p3];

    // Catmull-Rom spline formula
    // P(t) = 0.5 * ( (2*P1) + (-P0 + P2)*t + (2*P0 - 5*P1 + 4*P2 - P3)*t^2 + (-P0 + 3*P1 - 3*P2 + P3)*t^3 )
    
    float u2 = u * u;
    float u3 = u2 * u;

    Vector3 term0 = v1 * 2.0f;
    Vector3 term1 = (v2 - v0) * u;
    Vector3 term2 = (v0 * 2.0f - v1 * 5.0f + v2 * 4.0f - v3) * u2;
    Vector3 term3 = (v0 * -1.0f + v1 * 3.0f - v2 * 3.0f + v3) * u3;

    return (term0 + term1 + term2 + term3) * 0.5f;
}

Vector3 SplineMotionInterpolator::EvaluateVelocity(float t) const {
    if (m_pointCount <= 1) {
        return Vector3();
    }

    t = std::max(0.0f, std::min(t, static_cast<float>(m_pointCount - 1)));
    
    int p = static_cast<int>(std::floor(t));
    float u = t - static_cast<float>(p);

    if (p >= static_cast<int>(m_pointCount) - 1) {
        // Derivative at the end is defined by the last segment as it approaches 1
        p = m_pointCount - 2;
        u = 1.0f;
    }

    int p0 = std::max(0, p - 1);
    int p1 = p;
    int p2 = std::min(static_cast<int>(m_pointCount) - 1, p + 1);
    int p3 = std::min(static_cast<int>(m_pointCount) - 1, p + 2);

    const Vector3& v0 = m_points[p0];
    const Vector3& v1 = m_points[p1];
    const Vector3& v2 = m_points[p2];
    const Vector3& v3 = m_points[p3];

    // Catmull-Rom derivative (velocity) formula:
    // P'(t) = 0.5 * ( (-P0 + P2) + 2*(2*P0 - 5*P1 + 4*P2 - P3)*t + 3*(-P0 + 3*P1 - 3*P2 + P3)*t^2 )

    float u2 = u * u;

    Vector3 term1 = v2 - v0;
    Vector3 term2 = (v0 * 2.0f - v1 * 5.0f + v2 * 4.0f - v3) * (2.0f * u);
    Vector3 term3 = (v0 * -1.0f + v1 * 3.0f - v2 * 3.0f + v3) * (3.0f * u2);

    return (term1 + term2 + term3) * 0.5f;
}

} // namespace Client::Mimic
