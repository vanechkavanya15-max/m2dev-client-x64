#pragma once

#include <cmath>
#include <vector>
#include <optional>

namespace Client::World {

// Ensure sizeof(Vector3) == 12 as per memory guidelines
struct Vector3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    float LengthSq() const {
        return x * x + y * y + z * z;
    }

    float Length() const {
        return std::sqrt(LengthSq());
    }

    Vector3 Normalized() const {
        float lenSq = LengthSq();
        if (lenSq > 0.0f) {
            float invLen = 1.0f / std::sqrt(lenSq);
            return {x * invLen, y * invLen, z * invLen};
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

static_assert(sizeof(Vector3) == 12, "Vector3 size must be exactly 12 bytes");

struct Cylinder {
    Vector3 center; // Base center
    float radius{0.0f};
    float height{0.0f};
};

struct AABB {
    Vector3 min;
    Vector3 max;
};

struct CollisionResult {
    bool hasCollision{false};
    Vector3 contactNormal{0.0f, 0.0f, 0.0f};
    float penetrationDepth{0.0f};
};

class CollisionDetector {
public:
    // Cylinder-Cylinder collision test
    static CollisionResult TestCylinderCylinder(const Cylinder& a, const Cylinder& b);

    // Cylinder-AABB collision test
    static CollisionResult TestCylinderAABB(const Cylinder& cyl, const AABB& aabb);

    // Calculate slide vector along a wall
    static Vector3 CalculateSlideVector(const Vector3& velocity, const Vector3& normal);

    // Resolve movement by accumulating slide vectors from multiple obstacles
    static Vector3 ResolveMovement(const Cylinder& player, const Vector3& velocity, 
                                   const std::vector<Cylinder>& actors, 
                                   const std::vector<AABB>& environment);
};

} // namespace Client::World
