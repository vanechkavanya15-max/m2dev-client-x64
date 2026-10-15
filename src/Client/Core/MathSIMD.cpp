#include "MathSIMD.h"
#include <cstring>

namespace Client::Core {

    Vec3::Vec3() {
        m = _mm_setzero_ps();
    }

    Vec3::Vec3(float x_val, float y_val, float z_val) {
        m = _mm_set_ps(0.0f, z_val, y_val, x_val);
    }

    Vec3::Vec3(__m128 vec) {
        m = vec;
    }

    Vec3 Vec3::operator+(const Vec3& other) const {
        return Vec3(_mm_add_ps(m, other.m));
    }

    float Vec3::Dot(const Vec3& other) const {
        __m128 dp = _mm_dp_ps(m, other.m, 0x71);
        return _mm_cvtss_f32(dp);
    }

    Vec3 Vec3::Cross(const Vec3& other) const {
        __m128 tmp0 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 tmp1 = _mm_shuffle_ps(other.m, other.m, _MM_SHUFFLE(3, 1, 0, 2));
        __m128 tmp2 = _mm_mul_ps(tmp0, other.m);
        __m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
        __m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
        return Vec3(_mm_sub_ps(tmp3, tmp4));
    }

    Vec3 Vec3::Normalize() const {
        __m128 dp = _mm_dp_ps(m, m, 0x7F);
        __m128 rsqrt = _mm_rsqrt_ps(dp);
        
        // Newton-Raphson iteration: r = r * (1.5 - 0.5 * dp * r * r)
        __m128 half = _mm_set1_ps(0.5f);
        __m128 three_halfs = _mm_set1_ps(1.5f);
        __m128 nr = _mm_mul_ps(rsqrt, _mm_sub_ps(three_halfs, _mm_mul_ps(_mm_mul_ps(half, dp), _mm_mul_ps(rsqrt, rsqrt))));
        
        return Vec3(_mm_mul_ps(m, nr));
    }

    Mat4::Mat4() {
        std::memset(v, 0, sizeof(v));
        v[0] = 1.0f; v[5] = 1.0f; v[10] = 1.0f; v[15] = 1.0f;
    }

    Mat4::Mat4(const float* data) {
        std::memcpy(v, data, sizeof(v));
    }

    Mat4 Mat4::operator*(const Mat4& other) const {
        Mat4 res;
        for (int i = 0; i < 2; i++) {
            // Process 2 rows at a time
            __m256 r0 = _mm256_set_m128(_mm_set1_ps(m[i*2+1][0]), _mm_set1_ps(m[i*2][0]));
            __m256 r1 = _mm256_set_m128(_mm_set1_ps(m[i*2+1][1]), _mm_set1_ps(m[i*2][1]));
            __m256 r2 = _mm256_set_m128(_mm_set1_ps(m[i*2+1][2]), _mm_set1_ps(m[i*2][2]));
            __m256 r3 = _mm256_set_m128(_mm_set1_ps(m[i*2+1][3]), _mm_set1_ps(m[i*2][3]));

            __m256 row0 = _mm256_broadcast_ps((const __m128*)&other.m[0][0]);
            __m256 row1 = _mm256_broadcast_ps((const __m128*)&other.m[1][0]);
            __m256 row2 = _mm256_broadcast_ps((const __m128*)&other.m[2][0]);
            __m256 row3 = _mm256_broadcast_ps((const __m128*)&other.m[3][0]);

            __m256 sum = _mm256_add_ps(
                _mm256_add_ps(_mm256_mul_ps(r0, row0), _mm256_mul_ps(r1, row1)),
                _mm256_add_ps(_mm256_mul_ps(r2, row2), _mm256_mul_ps(r3, row3))
            );
            
            res.m256[i] = sum;
        }
        return res;
    }

    BoundingBox::BoundingBox() {
        min = Vec3(0, 0, 0);
        max = Vec3(0, 0, 0);
    }

    BoundingBox::BoundingBox(const Vec3& min_val, const Vec3& max_val) {
        min = min_val;
        max = max_val;
    }

    bool BoundingBox::Intersects(const BoundingBox& other) const {
        // We pack min and max of a bounding box into 256 bit registers.
        // A = [A.min, A.max], B = [B.min, B.max]
        // But it's easier to load 2 Vec3s (2x16 bytes = 32 bytes)
        // Since BoundingBox is min followed by max, it's 32 bytes.
        
        __m256 a = _mm256_load_ps((const float*)this); // [A.min, A.max]
        __m256 b = _mm256_loadu_ps((const float*)&other); // [B.min, B.max]

        // We want to check: A.min <= B.max AND A.max >= B.min
        // Rearranging: A.min <= B.max AND -A.max <= -B.min
        // We can do this with one CMP instruction if we negate the max components.
        
        __m256 sign_mask = _mm256_set_ps(-0.0f, -0.0f, -0.0f, -0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m256 a_neg = _mm256_xor_ps(a, sign_mask); // [A.min, -A.max]
        __m256 b_neg = _mm256_xor_ps(b, sign_mask); // [B.min, -B.max]

        __m256 b_swapped = _mm256_permute2f128_ps(b_neg, b_neg, 1); // [-B.max, B.min]
        __m256 b_shuf = _mm256_xor_ps(b_swapped, _mm256_set1_ps(-0.0f)); // [B.max, -B.min]
        
        __m256 cmp = _mm256_cmp_ps(a_neg, b_shuf, _CMP_LE_OQ);
        int mask = _mm256_movemask_ps(cmp);
        
        // We only care about x, y, z (bits 0,1,2 and 4,5,6), so mask & 0x77 should be 0x77
        return (mask & 0x77) == 0x77;
    }

    void BatchDistanceTest(const float* x8, const float* y8, const float* z8,
                           float tx, float ty, float tz,
                           float* out_dist_sq8) {
        __m256 target_x = _mm256_set1_ps(tx);
        __m256 target_y = _mm256_set1_ps(ty);
        __m256 target_z = _mm256_set1_ps(tz);

        __m256 p_x = _mm256_loadu_ps(x8);
        __m256 p_y = _mm256_loadu_ps(y8);
        __m256 p_z = _mm256_loadu_ps(z8);

        __m256 dx = _mm256_sub_ps(p_x, target_x);
        __m256 dy = _mm256_sub_ps(p_y, target_y);
        __m256 dz = _mm256_sub_ps(p_z, target_z);

        __m256 dsq = _mm256_add_ps(
            _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)),
            _mm256_mul_ps(dz, dz)
        );

        _mm256_storeu_ps(out_dist_sq8, dsq);
    }

} // namespace Client::Core
