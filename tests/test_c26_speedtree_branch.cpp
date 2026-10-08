#define TEST_MOCK_D3D9

// Mock out RenderSortKey redefinition in SortKeyBuilder
#define RenderSortKey RenderSortKey_mock
#include "SortKeyBuilder.h"
#undef RenderSortKey

#include "SpeedTreeBranchCollector.h"

namespace EterLib::Render {
    // Redefining SortKeyBuilder to use the proper RenderSortKey since the original file redefines it
    class SortKeyBuilderHack {
    public:
        constexpr SortKeyBuilderHack& WithPass(uint8_t passId) noexcept {
            m_key = (m_key & ~(0xFFULL << 56)) | ((static_cast<uint64_t>(passId) & 0xFFULL) << 56);
            return *this;
        }
        constexpr EterLib::Render::RenderSortKey Build() const noexcept {
            return EterLib::Render::RenderSortKey{m_key};
        }
    private:
        uint64_t m_key{0};
    };
}

#define SortKeyBuilder SortKeyBuilderHack
#include "SpeedTreeBranchCollector.cpp"
#undef SortKeyBuilder

#include "RenderQueue.cpp"
#include "LinearFrameAllocator.cpp"
#include <cassert>
#include <iostream>

using namespace EterLib::Render;

int main() {
    LinearFrameAllocator alloc;
    RenderQueue queue;
    SpeedTreeBranchCollector collector;

    D3DMATRIX world = {};
    LPDIRECT3DVERTEXBUFFER9 vb = (LPDIRECT3DVERTEXBUFFER9)0x1234;
    LPDIRECT3DINDEXBUFFER9 ib = (LPDIRECT3DINDEXBUFFER9)0x5678;

    collector.SubmitBranch(vb, ib, 10, world, queue, alloc);

    assert(queue.GetEntryCount() == 1);
    
    auto entries = queue.GetEntries();
    auto& entry = entries[0];
    
    // Pass::Opaque should be 1, in top 8 bits of 64 bit key
    uint64_t expectedKey = (static_cast<uint64_t>(1) << 56);
    assert(entry.sortKey.value == expectedKey);
    assert(entry.type == CommandType::Draw);
    
    DrawIndexedCommand* cmd = static_cast<DrawIndexedCommand*>(entry.commandPtr);
    assert(cmd != nullptr);
    assert(cmd->vertexBuffer == vb);
    assert(cmd->indexBuffer == ib);
    assert(cmd->primitiveCount == 10);

    std::cout << "SpeedTreeBranchCollector tests passed." << std::endl;
    return 0;
}

