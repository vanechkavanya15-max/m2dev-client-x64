#define _D3D9_H_
#define _D3DX9_H_

typedef unsigned int UINT;
typedef unsigned int DWORD;

struct IDirect3DDevice9 {
    virtual void DrawPrimitiveUP(int primitiveType, int primitiveCount, const void* vertexStreamZeroData, int vertexStreamZeroStride) = 0;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

enum D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
};

// Include d3d9 definitions inline instead of creating a file that could cause conflicts later.
// We must ensure the actual code includes DrawUserPrimitiveCommand which indirectly wants d3d9.h
// By defining guards, we prevented real d3d9.h from being needed.

#include "EterLib/Render/UIPrimitiveBatcher.cpp"
#include "EterLib/Render/LinearFrameAllocator.cpp"
#include "EterLib/Render/RenderQueue.cpp"

#include <gtest/gtest.h>

using namespace EterLib::Render;

TEST(UIPrimitiveBatcherTest, TestAddRectAndLine)
{
    UIPrimitiveBatcher batcher;
    LinearFrameAllocator alloc(1024 * 1024);
    RenderQueue queue;

    batcher.AddRect(10.0f, 10.0f, 100.0f, 50.0f, 0xFFFFFFFF);
    batcher.AddLine(0.0f, 0.0f, 100.0f, 100.0f, 0xFFFF0000);

    batcher.Submit(queue, alloc);

    EXPECT_EQ(queue.GetEntryCount(), 2);

    auto entries = queue.GetEntries();
    
    // First is likely Rect (triangles), second Line, or vice versa depending on Submit implementation
    bool foundRect = false;
    bool foundLine = false;

    for (const auto& entry : entries)
    {
        EXPECT_EQ(entry.type, CommandType::Draw);
        auto* cmd = static_cast<DrawUserPrimitiveCommand*>(entry.commandPtr);
        
        if (cmd->primitiveType == D3DPT_TRIANGLELIST)
        {
            EXPECT_EQ(cmd->primitiveCount, 2); // 1 rect = 2 triangles
            foundRect = true;
        }
        else if (cmd->primitiveType == D3DPT_LINELIST)
        {
            EXPECT_EQ(cmd->primitiveCount, 1); // 1 line segment
            foundLine = true;
        }
    }

    EXPECT_TRUE(foundRect);
    EXPECT_TRUE(foundLine);

    // After submit, batches should be cleared.
    // If we submit again, queue count should not increase
    batcher.Submit(queue, alloc);
    EXPECT_EQ(queue.GetEntryCount(), 2); // No new commands added
}

