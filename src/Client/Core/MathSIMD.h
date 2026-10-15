#pragma once

#include <immintrin.h>
#include <array>
#include <cstdint>

namespace Client::Core {

    struct alignas(16) Vec3 {
        union {
            __m128 m;
            struct {
                float x, y, z, w;
            };
            float v[4];
        };

        Vec3();
        Vec3(float x, float y, float z);
        Vec3(__m128 vec);

        Vec3 operator+(const Vec3& other) const;
        float Dot(const Vec3& other) const;
        Vec3 Cross(const Vec3& other) const;
        Vec3 Normalize() const;
    };

    struct alignas(32) Mat4 {
        union {
            __m256 m256[2]; 
            float m[4][4];  
            float v[16];
        };

        Mat4();
        Mat4(const float* data);

        Mat4 operator*(const Mat4& other) const;
    };

    struct alignas(32) BoundingBox {
        Vec3 min;
        Vec3 max;

        BoundingBox();
        BoundingBox(const Vec3& min_val, const Vec3& max_val);

        // AVX2 optimized intersection
        bool Intersects(const BoundingBox& other) const;
    };

    // Batch distance test for 8 points.
    // Computes squared distance from (tx, ty, tz) to 8 points (x8, y8, z8).
    void BatchDistanceTest(const float* x8, const float* y8, const float* z8,
                           float tx, float ty, float tz,
                           float* out_dist_sq8);

} // namespace Client::Core
