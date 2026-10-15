#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include "DebugVertex.h"
#include "DebugRenderPrimitives.h"

namespace Client::World
{
class ActorRegistry;
}

namespace Client::Graphics::Debug
{

class DebugRenderer
{
public:
    using ActorPositionProvider = std::function<bool(uint32_t vid, float& x, float& y, float& z)>;
    using MainPlayerPositionProvider = std::function<bool(float& x, float& y, float& z)>;

    static DebugRenderer& GetInstance() noexcept;
    static DebugRenderer& Instance() noexcept;
    static DebugRenderer* InstancePtr() noexcept;

    // Dependency Injection dla pozycji encji w swiecie gry
    void SetActorRegistry(Client::World::ActorRegistry* pRegistry) noexcept { m_actorRegistry = pRegistry; }
    void SetPositionProviders(ActorPositionProvider actorFn, MainPlayerPositionProvider mainFn) noexcept
    {
        m_actorPosFn = std::move(actorFn);
        m_mainPlayerPosFn = std::move(mainFn);
    }
    void SetActorPositionResolver(ActorPositionProvider actorFn) noexcept { m_actorPosFn = std::move(actorFn); }
    void SetMainPlayerPositionResolver(MainPlayerPositionProvider mainFn) noexcept { m_mainPlayerPosFn = std::move(mainFn); }

    bool GetActorPosition(uint32_t vid, float& x, float& y, float& z) const;
    bool GetMainActorPosition(float& x, float& y, float& z) const;

    // Dodawanie prymitywow
    void AddLine(float sx, float sy, float sz, float ex, float ey, float ez, uint32_t color, bool xray = false);
    void AddBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, uint32_t color, bool xray = false);
    void AddCircle(float cx, float cy, float cz, float radius, uint32_t color, bool xray = false, uint32_t segments = 24);

    // Oznaczanie aktorow
    void AddTracer(uint32_t targetVid, uint32_t color, bool xray = false);
    void AddActorBox(uint32_t targetVid, uint32_t color, bool xray = false);
    void AddActorMarker(uint32_t targetVid, uint32_t color, bool xray = false, bool drawBox = true, bool drawTracer = true);

    // Zarzadzanie buforem
    void Clear() noexcept;
    [[nodiscard]] size_t GetLineCount() const noexcept;
    [[nodiscard]] size_t GetBoxCount() const noexcept;
    [[nodiscard]] size_t GetCircleCount() const noexcept { return m_circles.size(); }
    [[nodiscard]] size_t GetMarkerCount() const noexcept { return m_markers.size(); }
    [[nodiscard]] size_t GetTotalVertexCount() const noexcept;

    // Generowanie buforow wierzcholkow
    void BuildVertexBatches(std::vector<DebugVertex>& outStandardVertices, std::vector<DebugVertex>& outXRayVertices);

    // Renderowanie Direct3D 9
    void Render(void* pD3DDevice);

private:
    DebugRenderer() = default;
    ~DebugRenderer() = default;

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    std::vector<DebugLine> m_lines;
    std::vector<DebugBox> m_boxes;
    std::vector<DebugCircle> m_circles;
    std::vector<DebugActorMarker> m_markers;

    Client::World::ActorRegistry* m_actorRegistry = nullptr;
    ActorPositionProvider m_actorPosFn;
    MainPlayerPositionProvider m_mainPlayerPosFn;

    std::vector<DebugVertex> m_cachedStandardVertices;
    std::vector<DebugVertex> m_cachedXRayVertices;
};

} // namespace Client::Graphics::Debug
