#pragma once
#include <cmath>
#include <vector>
#include <cstdint>

namespace EterModelLib {

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
    static float Dot(const Vector3& a, const Vector3& b);
    static Vector3 Cross(const Vector3& a, const Vector3& b);
    float Length() const;
    void Normalize();
};

struct Matrix4x4;

struct Quaternion {
    float x, y, z, w;
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
    void Normalize();
    static Quaternion Identity();
    Quaternion operator*(const Quaternion& q) const;
    Matrix4x4 ToMatrix4x4() const;
};

struct Matrix4x4 {
    float m[4][4];
    Matrix4x4();
    
    static Matrix4x4 Identity();
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Matrix4x4 Inverse() const;
    Matrix4x4 Transpose() const;
    static Matrix4x4 TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale);
};

void EvaluateHierarchy(const std::vector<int32_t>& parentIndices, const std::vector<Matrix4x4>& localTransforms, std::vector<Matrix4x4>& outWorldTransforms);
void ComputeSkinningMatrices(const std::vector<Matrix4x4>& worldTransforms, const std::vector<Matrix4x4>& invBindMatrices, std::vector<Matrix4x4>& outSkinMatrices);

} // namespace EterModelLib
