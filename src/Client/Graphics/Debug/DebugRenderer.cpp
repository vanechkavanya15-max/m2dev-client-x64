#include "DebugRenderer.h"
#include "Client/World/ActorRegistry.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#endif

namespace Client::Graphics::Debug
{

DebugRenderer& DebugRenderer::GetInstance() noexcept
{
    static DebugRenderer s_instance;
    return s_instance;
}

DebugRenderer& DebugRenderer::Instance() noexcept
{
    return GetInstance();
}

DebugRenderer* DebugRenderer::InstancePtr() noexcept
{
    return &GetInstance();
}

void DebugRenderer::AddLine(float sx, float sy, float sz, float ex, float ey, float ez, uint32_t color, bool xray)
{
    m_lines.push_back({sx, sy, sz, ex, ey, ez, color, xray});
}

void DebugRenderer::AddBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, uint32_t color, bool xray)
{
    m_boxes.push_back({minX, minY, minZ, maxX, maxY, maxZ, color, xray});
}

void DebugRenderer::AddCircle(float cx, float cy, float cz, float radius, uint32_t color, bool xray, uint32_t segments)
{
    m_circles.push_back({cx, cy, cz, radius, color, xray, segments});
}

void DebugRenderer::AddTracer(uint32_t targetVid, uint32_t color, bool xray)
{
    m_markers.push_back({targetVid, color, xray, false, true});
}

void DebugRenderer::AddActorBox(uint32_t targetVid, uint32_t color, bool xray)
{
    m_markers.push_back({targetVid, color, xray, true, false});
}

void DebugRenderer::AddActorMarker(uint32_t targetVid, uint32_t color, bool xray, bool drawBox, bool drawTracer)
{
    m_markers.push_back({targetVid, color, xray, drawBox, drawTracer});
}

void DebugRenderer::Clear() noexcept
{
    m_lines.clear();
    m_boxes.clear();
    m_circles.clear();
    m_markers.clear();
    m_cachedStandardVertices.clear();
    m_cachedXRayVertices.clear();
}

size_t DebugRenderer::GetLineCount() const noexcept
{
    size_t count = m_lines.size();
    for (const auto& m : m_markers)
    {
        if (m.drawTracer)
            count++;
    }
    return count;
}

size_t DebugRenderer::GetBoxCount() const noexcept
{
    size_t count = m_boxes.size();
    for (const auto& m : m_markers)
    {
        if (m.drawBox)
            count++;
    }
    return count;
}

size_t DebugRenderer::GetTotalVertexCount() const noexcept
{
    size_t count = m_lines.size() * 2;
    count += m_boxes.size() * 24;
    for (const auto& c : m_circles)
    {
        const uint32_t seg = (c.segments < 8) ? 8 : (c.segments > 64 ? 64 : c.segments);
        count += seg * 2;
    }
    for (const auto& m : m_markers)
    {
        if (m.drawBox)
            count += 24;
        if (m.drawTracer)
            count += 2;
    }
    return count;
}

bool DebugRenderer::GetActorPosition(uint32_t vid, float& x, float& y, float& z) const
{
    if (m_actorPosFn)
    {
        return m_actorPosFn(vid, x, y, z);
    }

    if (m_actorRegistry)
    {
        return m_actorRegistry->VisitActor(Client::World::EntityVid{vid}, [&](const Client::World::ActorRecord& rec) {
            x = rec.x;
            y = rec.y;
            z = rec.z;
        });
    }

    return false;
}

bool DebugRenderer::GetMainActorPosition(float& x, float& y, float& z) const
{
    if (m_mainPlayerPosFn)
    {
        return m_mainPlayerPosFn(x, y, z);
    }

    if (m_actorRegistry)
    {
        const auto mainVid = m_actorRegistry->GetMainActorVid();
        if (mainVid.value() != 0)
        {
            return GetActorPosition(mainVid.value(), x, y, z);
        }
    }

    return false;
}

void DebugRenderer::BuildVertexBatches(std::vector<DebugVertex>& outStandardVertices, std::vector<DebugVertex>& outXRayVertices)
{
    outStandardVertices.clear();
    outXRayVertices.clear();

    const size_t totalExpected = GetTotalVertexCount();
    if (totalExpected > 0)
    {
        outStandardVertices.reserve(totalExpected);
        outXRayVertices.reserve(totalExpected);
    }

    // 1. Linie
    for (const auto& line : m_lines)
    {
        auto& target = line.xray ? outXRayVertices : outStandardVertices;
        target.push_back({line.sx, line.sy, line.sz, line.color});
        target.push_back({line.ex, line.ey, line.ez, line.color});
    }

    // 2. Boxy AABB
    for (const auto& box : m_boxes)
    {
        auto& target = box.xray ? outXRayVertices : outStandardVertices;
        GenerateBoxVertices(box, target);
    }

    // 3. Okregi 3D
    for (const auto& circle : m_circles)
    {
        auto& target = circle.xray ? outXRayVertices : outStandardVertices;
        GenerateCircleVertices(circle, target);
    }

    // 4. Znaczniki aktorow (Tracery i Boxy)
    float mainX = 0.0f, mainY = 0.0f, mainZ = 0.0f;
    const bool hasMainPlayer = GetMainActorPosition(mainX, mainY, mainZ);

    for (const auto& marker : m_markers)
    {
        if (marker.targetVid == 0)
            continue;

        float actorX = 0.0f, actorY = 0.0f, actorZ = 0.0f;
        if (GetActorPosition(marker.targetVid, actorX, actorY, actorZ))
        {
            auto& target = marker.xray ? outXRayVertices : outStandardVertices;

            if (marker.drawBox)
            {
                constexpr float halfExtent = 50.0f;
                constexpr float height = 150.0f;
                DebugBox aabbBox{
                    actorX - halfExtent, actorY - halfExtent, actorZ,
                    actorX + halfExtent, actorY + halfExtent, actorZ + height,
                    marker.color,
                    marker.xray
                };
                GenerateBoxVertices(aabbBox, target);
            }

            if (marker.drawTracer && hasMainPlayer)
            {
                target.push_back({mainX, mainY, mainZ + 50.0f, marker.color});
                target.push_back({actorX, actorY, actorZ + 75.0f, marker.color});
            }
        }
    }
}

void DebugRenderer::Render(void* pD3DDevice)
{
    if (!pD3DDevice)
        return;

#if defined(_WIN32) || defined(_WIN64)
    auto* device = static_cast<IDirect3DDevice9*>(pD3DDevice);

    BuildVertexBatches(m_cachedStandardVertices, m_cachedXRayVertices);

    if (m_cachedStandardVertices.empty() && m_cachedXRayVertices.empty())
        return;

    auto drawBatched = [device](const std::vector<DebugVertex>& verts) {
        constexpr size_t MAX_BATCH_PRIMITIVES = 4096;
        const size_t maxVerticesPerChunk = MAX_BATCH_PRIMITIVES * 2;
        for (size_t offset = 0; offset < verts.size(); offset += maxVerticesPerChunk)
        {
            const size_t chunkVerts = std::min(maxVerticesPerChunk, verts.size() - offset);
            const auto primitiveCount = static_cast<UINT>(chunkVerts / 2);
            if (primitiveCount > 0)
            {
                device->DrawPrimitiveUP(D3DPT_LINELIST, primitiveCount, &verts[offset], sizeof(DebugVertex));
            }
        }
    };

    // Rysowanie wierzcholkow standardowych (ze sprawdzaniem bufora Z)
    if (!m_cachedStandardVertices.empty())
    {
        device->SetFVF(D3DFVF_DEBUG_VERTEX);
        drawBatched(m_cachedStandardVertices);
    }

    // Rysowanie wierzcholkow X-Ray (przez sciany) z zachowaniem biezacego stanu ZENABLE
    if (!m_cachedXRayVertices.empty())
    {
        DWORD dwSavedZEnable = D3DZB_TRUE;
        device->GetRenderState(D3DRS_ZENABLE, &dwSavedZEnable);

        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetFVF(D3DFVF_DEBUG_VERTEX);
        drawBatched(m_cachedXRayVertices);

        device->SetRenderState(D3DRS_ZENABLE, dwSavedZEnable);
    }
#endif
}

} // namespace Client::Graphics::Debug
