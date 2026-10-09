#include "SkeletalMath.h"

namespace EterModelLib {

Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, float t) {
    return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}
float Vector3::Dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vector3 Vector3::Cross(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}
float Vector3::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}
void Vector3::Normalize() {
    float len = Length();
    if (len > 0.000001f) {
        x /= len; y /= len; z /= len;
    }
}

Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    Quaternion end = b;
    if (dot < 0.0f) {
        dot = -dot;
        end.x = -end.x; end.y = -end.y; end.z = -end.z; end.w = -end.w;
    }
    const float THRESHOLD = 0.9995f;
    if (dot > THRESHOLD) {
        Quaternion res(
            a.x + (end.x - a.x) * t,
            a.y + (end.y - a.y) * t,
            a.z + (end.z - a.z) * t,
            a.w + (end.w - a.w) * t
        );
        res.Normalize();
        return res;
    }
    float theta_0 = std::acos(dot);
    float theta = theta_0 * t;
    float sin_theta = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);
    float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    return Quaternion(
        s0 * a.x + s1 * end.x,
        s0 * a.y + s1 * end.y,
        s0 * a.z + s1 * end.z,
        s0 * a.w + s1 * end.w
    );
}
void Quaternion::Normalize() {
    float len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len > 0.000001f) {
        x /= len; y /= len; z /= len; w /= len;
    }
}
Quaternion Quaternion::Identity() {
    return Quaternion(0, 0, 0, 1);
}
Quaternion Quaternion::operator*(const Quaternion& q) const {
    return Quaternion(
        w * q.x + x * q.w + y * q.z - z * q.y,
        w * q.y - x * q.z + y * q.w + z * q.x,
        w * q.z + x * q.y - y * q.x + z * q.w,
        w * q.w - x * q.x - y * q.y - z * q.z
    );
}
Matrix4x4 Quaternion::ToMatrix4x4() const {
    Matrix4x4 mat;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, xw = x * w;
    float yz = y * z, yw = y * w, zw = z * w;
    mat.m[0][0] = 1 - 2 * (yy + zz);
    mat.m[0][1] = 2 * (xy - zw);
    mat.m[0][2] = 2 * (xz + yw);
    mat.m[0][3] = 0;
    mat.m[1][0] = 2 * (xy + zw);
    mat.m[1][1] = 1 - 2 * (xx + zz);
    mat.m[1][2] = 2 * (yz - xw);
    mat.m[1][3] = 0;
    mat.m[2][0] = 2 * (xz - yw);
    mat.m[2][1] = 2 * (yz + xw);
    mat.m[2][2] = 1 - 2 * (xx + yy);
    mat.m[2][3] = 0;
    mat.m[3][0] = 0; mat.m[3][1] = 0; mat.m[3][2] = 0; mat.m[3][3] = 1;
    return mat;
}

Matrix4x4::Matrix4x4() {
    for(int i=0; i<4; ++i)
        for(int j=0; j<4; ++j)
            m[i][j] = 0.0f;
}
Matrix4x4 Matrix4x4::Identity() {
    Matrix4x4 mat;
    for(int i=0; i<4; ++i) mat.m[i][i] = 1.0f;
    return mat;
}
Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 res;
    for(int i=0; i<4; ++i)
        for(int j=0; j<4; ++j)
            for(int k=0; k<4; ++k)
                res.m[i][j] += m[i][k] * other.m[k][j];
    return res;
}
Matrix4x4 Matrix4x4::Inverse() const {
    Matrix4x4 inv;
    float t[6];
    float a = m[0][0], b = m[0][1], c = m[0][2], d = m[0][3],
          e = m[1][0], f = m[1][1], g = m[1][2], h = m[1][3],
          i = m[2][0], j = m[2][1], k = m[2][2], l = m[2][3],
          p = m[3][0], q = m[3][1], r = m[3][2], s = m[3][3];

    t[0] = k * s - l * r;
    t[1] = j * s - l * q;
    t[2] = j * r - k * q;
    t[3] = i * s - l * p;
    t[4] = i * r - k * p;
    t[5] = i * q - j * p;

    inv.m[0][0] =  (f * t[0] - g * t[1] + h * t[2]);
    inv.m[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
    inv.m[0][2] =  (q * (c * h - d * g) - r * (b * h - d * f) + s * (b * g - c * f));
    inv.m[0][3] = -(j * (c * h - d * g) - k * (b * h - d * f) + l * (b * g - c * f));

    inv.m[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
    inv.m[1][1] =  (a * t[0] - c * t[3] + d * t[4]);
    inv.m[1][2] = -(p * (c * h - d * g) - r * (a * h - d * e) + s * (a * g - c * e));
    inv.m[1][3] =  (i * (c * h - d * g) - k * (a * h - d * e) + l * (a * g - c * e));

    inv.m[2][0] =  (e * t[1] - f * t[3] + h * t[5]);
    inv.m[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
    inv.m[2][2] =  (p * (b * h - d * f) - q * (a * h - d * e) + s * (a * f - b * e));
    inv.m[2][3] = -(i * (b * h - d * f) - j * (a * h - d * e) + l * (a * f - b * e));

    inv.m[3][0] = -(e * t[2] - f * t[4] + g * t[5]);
    inv.m[3][1] =  (a * t[2] - b * t[4] + c * t[5]);
    inv.m[3][2] = -(p * (b * g - c * f) - q * (a * g - c * e) + r * (a * f - b * e));
    inv.m[3][3] =  (i * (b * g - c * f) - j * (a * g - c * e) + k * (a * f - b * e));

    float det = a * inv.m[0][0] + b * inv.m[1][0] + c * inv.m[2][0] + d * inv.m[3][0];
    if (std::abs(det) > 0.000001f) {
        float invDet = 1.0f / det;
        for(int row=0; row<4; ++row)
            for(int col=0; col<4; ++col)
                inv.m[row][col] *= invDet;
    }
    return inv;
}
Matrix4x4 Matrix4x4::Transpose() const {
    Matrix4x4 res;
    for(int i=0; i<4; ++i)
        for(int j=0; j<4; ++j)
            res.m[i][j] = m[j][i];
    return res;
}
Matrix4x4 Matrix4x4::TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale) {
    Matrix4x4 s = Matrix4x4::Identity();
    s.m[0][0] = scale.x; s.m[1][1] = scale.y; s.m[2][2] = scale.z;
    Matrix4x4 r = rotation.ToMatrix4x4();
    Matrix4x4 t = Matrix4x4::Identity();
    t.m[3][0] = translation.x; t.m[3][1] = translation.y; t.m[3][2] = translation.z;
    return s * r * t; 
}

void EvaluateHierarchy(const std::vector<int32_t>& parentIndices, const std::vector<Matrix4x4>& localTransforms, std::vector<Matrix4x4>& outWorldTransforms) {
    outWorldTransforms.resize(localTransforms.size());
    for(size_t i=0; i<localTransforms.size(); ++i) {
        int p = parentIndices[i];
        if (p >= 0 && p < (int)i) {
            outWorldTransforms[i] = localTransforms[i] * outWorldTransforms[p];
        } else {
            outWorldTransforms[i] = localTransforms[i];
        }
    }
}
void ComputeSkinningMatrices(const std::vector<Matrix4x4>& worldTransforms, const std::vector<Matrix4x4>& invBindMatrices, std::vector<Matrix4x4>& outSkinMatrices) {
    outSkinMatrices.resize(worldTransforms.size());
    for(size_t i=0; i<worldTransforms.size(); ++i) {
        outSkinMatrices[i] = invBindMatrices[i] * worldTransforms[i];
    }
}

} // namespace EterModelLib
