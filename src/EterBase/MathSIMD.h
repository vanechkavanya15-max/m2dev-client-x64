#pragma once

/**
 * @file MathSIMD.h
 * @brief Fundament wektoryzacji SIMD (DirectXMath AVX2 / SSE4.1) dla standardu C++23.
 * 
 * Zapewnia operacje wektorowe odporne na brak wyrownania pamieci (Unaligned Memory Loads),
 * dzieki czemu jest w 100% bezpieczny w polaczeniu ze starymi strukturami (#pragma pack(1)).
 */

#include <DirectXMath.h>
#include <immintrin.h>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace MathSIMD
{
    using namespace DirectX;

    // Bezpieczne ladowanie wektora 3D z pamieci o dowolnym wyrownaniu
    [[nodiscard]] inline XMVECTOR Load3(const float* pData) noexcept
    {
        return XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(pData));
    }

    // Bezpieczne zapisanie wektora 3D do pamieci o dowolnym wyrownaniu
    inline void Store3(float* pDest, FXMVECTOR v) noexcept
    {
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(pDest), v);
    }

    // Bezpieczne ladowanie wektora 4D
    [[nodiscard]] inline XMVECTOR Load4(const float* pData) noexcept
    {
        return XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(pData));
    }

    // Bezpieczne zapisanie wektora 4D
    inline void Store4(float* pDest, FXMVECTOR v) noexcept
    {
        XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(pDest), v);
    }

    // Dystans euklidesowy pomiedzy dwoma punktami 3D (AVX2 / SSE)
    [[nodiscard]] inline float Distance(FXMVECTOR a, FXMVECTOR b) noexcept
    {
        const XMVECTOR diff = XMVectorSubtract(a, b);
        return XMVectorGetX(XMVector3Length(diff));
    }

    // Kwadrat dystansu pomiedzy dwoma punktami 3D (szybsze od Distance, bez sqrt)
    [[nodiscard]] inline float DistanceSq(FXMVECTOR a, FXMVECTOR b) noexcept
    {
        const XMVECTOR diff = XMVectorSubtract(a, b);
        return XMVectorGetX(XMVector3LengthSq(diff));
    }

    // Iloczyn skalarny (Dot Product)
    [[nodiscard]] inline float Dot3(FXMVECTOR a, FXMVECTOR b) noexcept
    {
        return XMVectorGetX(XMVector3Dot(a, b));
    }

    // Iloczyn wektorowy (Cross Product)
    [[nodiscard]] inline XMVECTOR Cross3(FXMVECTOR a, FXMVECTOR b) noexcept
    {
        return XMVector3Cross(a, b);
    }

    // Znormalizowany wektor 3D
    [[nodiscard]] inline XMVECTOR Normalize3(FXMVECTOR v) noexcept
    {
        return XMVector3Normalize(v);
    }

    // Dlugosc wektora 3D
    [[nodiscard]] inline float Length3(FXMVECTOR v) noexcept
    {
        return XMVectorGetX(XMVector3Length(v));
    }

    // Test kolizji sfera-sfera (Sphere-Sphere)
    [[nodiscard]] inline bool IntersectSphereSphere(FXMVECTOR centerA, float radiusA, FXMVECTOR centerB, float radiusB) noexcept
    {
        const float distSq = DistanceSq(centerA, centerB);
        const float totalRadius = radiusA + radiusB;
        return distSq <= (totalRadius * totalRadius);
    }

    // Wektorowy test przeciecia promienia z prostopadloscianem AABB (Kay-Kajiya slab method)
    [[nodiscard]] inline bool IntersectRayAABB(
        FXMVECTOR rayOrigin,
        FXMVECTOR rayDir,
        FXMVECTOR boxMin,
        FXMVECTOR boxMax,
        float& outTMin,
        float& outTMax) noexcept
    {
        // Odwrotnosc kierunku promienia z ochrona przed dzieleniem przez zero
        const XMVECTOR vZero = XMVectorZero();
        const XMVECTOR vOne = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
        const XMVECTOR vInvDir = XMVectorDivide(vOne, XMVectorSelect(rayDir, XMVectorSet(1e-7f, 1e-7f, 1e-7f, 1e-7f), XMVectorEqual(rayDir, vZero)));

        const XMVECTOR t0 = XMVectorMultiply(XMVectorSubtract(boxMin, rayOrigin), vInvDir);
        const XMVECTOR t1 = XMVectorMultiply(XMVectorSubtract(boxMax, rayOrigin), vInvDir);

        const XMVECTOR tSmall = XMVectorMin(t0, t1);
        const XMVECTOR tBig = XMVectorMax(t0, t1);

        float tSmallArr[4], tBigArr[4];
        XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(tSmallArr), tSmall);
        XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(tBigArr), tBig);

        outTMin = (std::max)({ tSmallArr[0], tSmallArr[1], tSmallArr[2] });
        outTMax = (std::min)({ tBigArr[0], tBigArr[1], tBigArr[2] });

        return (outTMax >= (std::max)(outTMin, 0.0f));
    }

    // Wektorowy test przeciecia promienia ze sfera (Ray-Sphere)
    [[nodiscard]] inline bool IntersectRaySphere(
        FXMVECTOR rayOrigin,
        FXMVECTOR rayDir,
        FXMVECTOR sphereCenter,
        float sphereRadius,
        float& outT) noexcept
    {
        const XMVECTOR m = XMVectorSubtract(rayOrigin, sphereCenter);
        const float b = Dot3(m, rayDir);
        const float c = Dot3(m, m) - (sphereRadius * sphereRadius);

        if (c > 0.0f && b > 0.0f)
            return false;

        const float discr = b * b - c;
        if (discr < 0.0f)
            return false;

        outT = -b - std::sqrt(discr);
        if (outT < 0.0f)
            outT = 0.0f;

        return true;
    }

    // Wektorowy Frustum Culling dla sfery (test wzgledem 6 plaszczyzn kamery)
    [[nodiscard]] inline bool FrustumContainsSphere(
        const XMVECTOR* planes,
        size_t planeCount,
        FXMVECTOR center,
        float radius) noexcept
    {
        for (size_t i = 0; i < planeCount; ++i)
        {
            // Odleglosc od plaszczyzny: dot(plane.xyz, center) + plane.w
            const XMVECTOR planeXYZ = planes[i];
            const float dotVal = XMVectorGetX(XMVector3Dot(planeXYZ, center));
            const float planeW = XMVectorGetW(planes[i]);
            const float dist = dotVal + planeW;

            if (dist < -radius)
                return false; // Poza frustumem
        }
        return true;
    }
}
