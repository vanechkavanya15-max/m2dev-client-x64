#define __CSTATEMANAGER_H
#include "doctest.h"

// Define required mock D3D types directly in the test file to avoid modifying tests/mock_includes/d3d9.h
// Note on Memory rule: "In the Metin2 project, the mock headers `tests/mock_includes/d3d9.h` and `windows.h` are intentionally empty.
// When writing unit tests for DirectX components, manually define the required minimal mock interfaces and basic types directly in the test file."

typedef unsigned int UINT;
typedef int INT;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_TRIANGLELIST = 4,
} D3DPRIMITIVETYPE;

struct IDirect3DVertexBuffer9 {};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

struct IDirect3DIndexBuffer9 {};
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9;

struct IDirect3DDevice9 {
    void SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) {}
    void SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData) {}
    void DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {}
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

#include "src/EterLib/Render/TerrainPatchBatcher.h"
#include "src/EterLib/Render/RenderQueue.h"
#include "src/EterLib/Render/LinearFrameAllocator.h"
#include "src/EterLib/Render/DrawIndexedCommand.h"
#include <vector>

TEST_CASE("TerrainPatchBatcher SubmitPatches Verification")
{
    EterLib::Render::TerrainPatchBatcher batcher;
    EterLib::Render::RenderQueue targetQueue;
    EterLib::Render::LinearFrameAllocator allocator(1024 * 1024);

    std::vector<uint32_t> visibleNodes = {10, 20, 30};

    batcher.SubmitPatches(visibleNodes, targetQueue, allocator);

    auto entries = targetQueue.GetEntries();
    REQUIRE(entries.size() == 3);

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& entry = entries[i];
        REQUIRE(entry.type == EterLib::Render::CommandType::Draw);
        REQUIRE(entry.commandPtr != nullptr);

        auto* cmd = static_cast<EterLib::Render::DrawIndexedCommand*>(entry.commandPtr);
        REQUIRE(cmd->primitiveType == D3DPT_TRIANGLELIST);

        // Verify the SortKey
        uint64_t keyVal = entry.sortKey.value;
        uint64_t passVal = keyVal >> 56;
        uint64_t shaderVal = (keyVal >> 40) & 0xFFF;
        uint64_t depthVal = keyVal & 0xFFF;

        REQUIRE(passVal == static_cast<uint64_t>(EterLib::Render::Pass::Opaque));
        REQUIRE(shaderVal == 42); // TERRAIN_SHADER_ID
        REQUIRE(depthVal == visibleNodes[i]);
    }
}

