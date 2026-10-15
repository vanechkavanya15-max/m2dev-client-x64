#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include "DebugVertex.h"

namespace Client::Graphics::Debug
{

struct DebugLine
{
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    float ex = 0.0f, ey = 0.0f, ez = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    bool xray = false;
};

struct DebugBox
{
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    bool xray = false;
};

struct DebugCircle
{
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    float radius = 100.0f;
    uint32_t color = 0xFFFFFFFF;
    bool xray = false;
    uint32_t segments = 24;
};

struct DebugActorMarker
{
    uint32_t targetVid = 0;
    uint32_t color = 0xFFFFFFFF;
    bool xray = false;
    bool drawBox = true;
    bool drawTracer = true;
};

// Generatory geometrii do bufora wierzcholkow
inline void GenerateBoxVertices(const DebugBox& box, std::vector<DebugVertex>& outVertices)
{
    // 8 wierzcholkow prostopadloscianu AABB
    const float x0 = box.minX, x1 = box.maxX;
    const float y0 = box.minY, y1 = box.maxY;
    const float z0 = box.minZ, z1 = box.maxZ;
    const uint32_t col = box.color;

    // 12 krawedzi (24 wierzcholki dla D3DPT_LINELIST)
    // Dolna podstawa (z0)
    outVertices.push_back({x0, y0, z0, col}); outVertices.push_back({x1, y0, z0, col});
    outVertices.push_back({x1, y0, z0, col}); outVertices.push_back({x1, y1, z0, col});
    outVertices.push_back({x1, y1, z0, col}); outVertices.push_back({x0, y1, z0, col});
    outVertices.push_back({x0, y1, z0, col}); outVertices.push_back({x0, y0, z0, col});

    // Gorna podstawa (z1)
    outVertices.push_back({x0, y0, z1, col}); outVertices.push_back({x1, y0, z1, col});
    outVertices.push_back({x1, y0, z1, col}); outVertices.push_back({x1, y1, z1, col});
    outVertices.push_back({x1, y1, z1, col}); outVertices.push_back({x0, y1, z1, col});
    outVertices.push_back({x0, y1, z1, col}); outVertices.push_back({x0, y0, z1, col});

    // 4 pionowe filary
    outVertices.push_back({x0, y0, z0, col}); outVertices.push_back({x0, y0, z1, col});
    outVertices.push_back({x1, y0, z0, col}); outVertices.push_back({x1, y0, z1, col});
    outVertices.push_back({x1, y1, z0, col}); outVertices.push_back({x1, y1, z1, col});
    outVertices.push_back({x0, y1, z0, col}); outVertices.push_back({x0, y1, z1, col});
}

inline void GenerateCircleVertices(const DebugCircle& circle, std::vector<DebugVertex>& outVertices)
{
    const uint32_t seg = (circle.segments < 8) ? 8 : (circle.segments > 64 ? 64 : circle.segments);
    constexpr float PI = 3.14159265358979323846f;
    const float step = (2.0f * PI) / static_cast<float>(seg);

    const float elevatedZ = circle.cz + 2.5f; // Eliminacja z-fightingu z podlozem terenu

    for (uint32_t i = 0; i < seg; ++i)
    {
        const float a0 = static_cast<float>(i) * step;
        const float a1 = static_cast<float>((i + 1) % seg) * step;

        const float x0 = circle.cx + circle.radius * std::cos(a0);
        const float y0 = circle.cy + circle.radius * std::sin(a0);
        const float x1 = circle.cx + circle.radius * std::cos(a1);
        const float y1 = circle.cy + circle.radius * std::sin(a1);

        outVertices.push_back({x0, y0, elevatedZ, circle.color});
        outVertices.push_back({x1, y1, elevatedZ, circle.color});
    }
}

} // namespace Client::Graphics::Debug
