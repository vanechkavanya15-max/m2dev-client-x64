#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#define TEST_MOCK_D3D9
#include "../src/EterLib/Render/GrannyRenderBridge.h"

// Provide dummy doctest toString methods to satisfy the linker
namespace doctest {
    String toString(int) { return "int"; }
    String toString(float) { return "float"; }
    String toString(unsigned long) { return "ulong"; }
    String toString(unsigned char) { return "uchar"; }
    String::String(char const*) {}
    String::~String() {}
    String operator+(String const&, String const&) { return String(""); }
    
    namespace detail {
        int setTestSuite(const TestSuite&) noexcept { return 0; }
        TestSuite& TestSuite::operator*(char const*) noexcept { return *this; }
    }
}

// Instead of linking against the real library, we include the source directly
// to test it with our mocks and TEST_MOCK_D3D9 defined.
#include "../src/EterLib/Render/GrannyRenderBridge.cpp"

// Simple LinearFrameAllocator mock for testing
namespace EterLib::Render {
    LinearFrameAllocator::LinearFrameAllocator(size_t capacity) 
        : m_capacity(capacity), m_offset(0)
    {
        m_buffer = std::make_unique<std::byte[]>(capacity);
    }
    
    LinearFrameAllocator::~LinearFrameAllocator() = default;
    LinearFrameAllocator::LinearFrameAllocator(LinearFrameAllocator&& other) noexcept = default;
    LinearFrameAllocator& LinearFrameAllocator::operator=(LinearFrameAllocator&& other) noexcept = default;
    
    void* LinearFrameAllocator::Allocate(size_t size, size_t alignment) noexcept
    {
        // Align offset
        size_t padding = (alignment - (m_offset % alignment)) % alignment;
        if (m_offset + padding + size > m_capacity)
            return nullptr;
            
        m_offset += padding;
        void* ptr = m_buffer.get() + m_offset;
        m_offset += size;
        return ptr;
    }
    
    void LinearFrameAllocator::Reset() noexcept
    {
        m_offset = 0;
    }
    
    size_t LinearFrameAllocator::GetAllocatedBytes() const noexcept { return m_offset; }
    size_t LinearFrameAllocator::GetCapacity() const noexcept { return m_capacity; }
    
    void RenderQueue::Submit(RenderSortKey key, void* cmd, CommandType type)
    {
        RenderQueueEntry entry;
        entry.sortKey = key;
        entry.commandPtr = cmd;
        entry.type = type;
        m_entries.push_back(entry);
    }
    
    void RenderQueue::Sort() noexcept { }
    void RenderQueue::Clear() noexcept { m_entries.clear(); }
    size_t RenderQueue::GetEntryCount() const noexcept { return m_entries.size(); }
    std::span<const RenderQueueEntry> RenderQueue::GetEntries() const noexcept { return m_entries; }
}

using namespace EterLib::Render;

TEST_CASE("GrannyRenderBridge - SubmitModel queues without immediate allocation") {
    GrannyRenderBridge bridge;
    RenderQueue queue;
    LinearFrameAllocator alloc(1024 * 1024); // 1 MB

    D3DMATRIX world1 = {};
    world1._11 = 1.0f;
    
    D3DMATRIX world2 = {};
    world2._22 = 2.0f;

    bridge.SubmitModel(100, world1, queue, alloc);
    bridge.SubmitModel(100, world2, queue, alloc);
    bridge.SubmitModel(200, world1, queue, alloc);

    REQUIRE(queue.GetEntryCount() == 0);
    REQUIRE(alloc.GetAllocatedBytes() == 0);
}

TEST_CASE("GrannyRenderBridge - FlushPending generates batches") {
    GrannyRenderBridge bridge;
    RenderQueue queue;
    LinearFrameAllocator alloc(1024 * 1024);

    D3DMATRIX world1 = {}; world1._11 = 1.0f;
    D3DMATRIX world2 = {}; world2._22 = 2.0f;
    D3DMATRIX world3 = {}; world3._33 = 3.0f;

    // Mesh 1 has 2 instances
    bridge.SubmitModel(10, world1, queue, alloc);
    bridge.SubmitModel(10, world2, queue, alloc);
    
    // Mesh 2 has 1 instance
    bridge.SubmitModel(20, world3, queue, alloc);

    bridge.FlushPending(queue, alloc);

    auto entries = queue.GetEntries();
    REQUIRE(entries.size() == 2);
    REQUIRE(alloc.GetAllocatedBytes() > 0);

    bool foundMesh10 = false;
    bool foundMesh20 = false;

    for (const auto& entry : entries) {
        REQUIRE((int)entry.type == (int)CommandType::Draw);
        auto* cmd = static_cast<ActorDrawCommand*>(entry.commandPtr);
        
        if (cmd->meshId == 10) {
            foundMesh10 = true;
            REQUIRE(cmd->instanceCount == 2);
            REQUIRE(cmd->instances[0]._11 == 1.0f);
            REQUIRE(cmd->instances[1]._22 == 2.0f);
        } else if (cmd->meshId == 20) {
            foundMesh20 = true;
            REQUIRE(cmd->instanceCount == 1);
            REQUIRE(cmd->instances[0]._33 == 3.0f);
        }
    }

    REQUIRE(foundMesh10);
    REQUIRE(foundMesh20);

    // Ensure queue clears out internal batcher
    size_t prevAlloc = alloc.GetAllocatedBytes();
    
    queue.Clear();
    bridge.FlushPending(queue, alloc);
    
    REQUIRE(queue.GetEntryCount() == 0);
    REQUIRE(alloc.GetAllocatedBytes() == prevAlloc); // No new allocations
}

