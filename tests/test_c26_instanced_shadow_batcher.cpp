#include "doctest.h"

// Do not define D3D here, let the mock header do it since we updated mock_includes/d3d9.h
// Wait, we reverted mock_includes/d3d9.h because of ZERO-CONFLICT rule! 
// Let's explicitly define them inside the test file only.
#define D3DPRIMITIVETYPE int
#define UINT unsigned int
struct IDirect3DDevice9 { void DrawPrimitiveUP(int, int, const void*, int) {} };
#define LPDIRECT3DDEVICE9 IDirect3DDevice9*
#define D3DPT_POINTLIST 1
#define D3DPT_LINELIST 2
#define D3DPT_LINESTRIP 3
#define D3DPT_TRIANGLELIST 4
#define D3DPT_TRIANGLESTRIP 5
#define D3DPT_TRIANGLEFAN 6
#define _D3D9_H_ // Block the mock header from loading since it's empty anyway

#include "EterLib/Render/InstancedShadowBatcher.h"
#include "EterLib/Render/RenderQueue.h"
#include "EterLib/Render/LinearFrameAllocator.h"
#include "EterLib/Render/DrawUserPrimitiveCommand.h"

using namespace EterLib::Render;

TEST_CASE("InstancedShadowBatcher - Batches multiple shadows into one command")
{
    InstancedShadowBatcher batcher;
    RenderQueue queue;
    LinearFrameAllocator allocator(1024 * 1024);

    SUBCASE("Empty batch does not submit commands")
    {
        batcher.Submit(queue, allocator);
        CHECK(queue.GetEntryCount() == 0);
    }

    SUBCASE("Submitting two shadows results in a single Draw command")
    {
        batcher.AddShadow(10.0f, 20.0f, 0.0f, 5.0f, 0.5f);
        batcher.AddShadow(50.0f, 60.0f, 0.0f, 5.0f, 0.8f);

        batcher.Submit(queue, allocator);

        REQUIRE(queue.GetEntryCount() == 1);

        auto entries = queue.GetEntries();
        const auto& entry = entries[0];
        
        CHECK(entry.type == CommandType::Draw);
        
        uint64_t expectedKey = static_cast<uint64_t>(Pass::AlphaBlend) << 56;
        CHECK(entry.sortKey.value == expectedKey);

        auto* cmd = static_cast<DrawUserPrimitiveCommand*>(entry.commandPtr);
        REQUIRE(cmd != nullptr);
        
        CHECK(cmd->primitiveType == D3DPT_TRIANGLELIST);
        
        // 2 shadows * 6 vertices per shadow / 3 vertices per triangle = 4 primitives (triangles)
        CHECK(cmd->primitiveCount == 4);
        
        // Stride should be 24 (float x,y,z + uint32 color + float u,v = 12 + 4 + 8 = 24)
        CHECK(cmd->vertexStreamZeroStride == 24);
        CHECK(cmd->vertexStreamZeroData != nullptr);
    }

    SUBCASE("Clear removes all queued shadows")
    {
        batcher.AddShadow(10.0f, 20.0f, 0.0f, 5.0f, 0.5f);
        batcher.Clear();
        
        batcher.Submit(queue, allocator);
        CHECK(queue.GetEntryCount() == 0);
    }
}

