// In Metin2 C++ tests, Doctest is included via tests/test_main.cpp as an executable.
#include "doctest.h"

#define TEST_MODE_DISABLE_STDAFX 1

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <span>

typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7, D3DRS_ALPHABLENDENABLE = 27, D3DRS_ZFUNC = 23,
    D3DRS_COLORWRITEENABLE = 168, D3DRS_SRCBLEND = 19, D3DRS_DESTBLEND = 20,
    D3DRS_ZWRITEENABLE = 14, D3DRS_ALPHATESTENABLE = 15, D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25, D3DRS_CULLMODE = 22, D3DRS_LIGHTING = 137
};
enum D3DBLEND { D3DBLEND_ONE = 2, D3DBLEND_SRCALPHA = 5, D3DBLEND_INVSRCALPHA = 6 };
enum D3DCMPFUNC { D3DCMP_NEVER = 1, D3DCMP_LESS = 2, D3DCMP_EQUAL = 3, D3DCMP_LESSEQUAL = 4, D3DCMP_GREATER = 5, D3DCMP_NOTEQUAL = 6, D3DCMP_GREATEREQUAL = 7, D3DCMP_ALWAYS = 8 };
enum D3DZBUFFERTYPE { D3DZB_FALSE = 0, D3DZB_TRUE = 1, D3DZB_USEW = 2 };
enum D3DCULL { D3DCULL_NONE = 1, D3DCULL_CW = 2, D3DCULL_CCW = 3 };

namespace EterBase { struct ModernLogger { static void Error(const char*) {} }; }
class CStateManager {
public:
    static CStateManager& Instance() { static CStateManager instance; return instance; }
    void SaveRenderState(D3DRENDERSTATETYPE state, DWORD val) {}
    void RestoreRenderState(D3DRENDERSTATETYPE state) { }
};
#define STATEMANAGER CStateManager::Instance()

#define RenderQueue ExecutorRenderQueue
#include "../src/EterLib/Render/RenderPipelineExecutor.h"
#undef RenderQueue

#include "../src/EterLib/Render/RenderQueue.h"
#include "../src/EterLib/Render/FrameStatisticsTracker.h"
#include "../src/EterLib/Render/LinearFrameAllocator.h"
#include "../src/EterLib/Render/FramePipelineCoordinator.h"

// MOCK ONLY DEPENDENCIES. DO NOT MOCK THE CLASS WE ARE TESTING!

namespace EterLib::Render {
    LinearFrameAllocator::LinearFrameAllocator(size_t capacity) : m_capacity(capacity > 0 ? capacity : DEFAULT_CAPACITY), m_offset(0) { m_buffer = std::make_unique<std::byte[]>(m_capacity); }
    LinearFrameAllocator::~LinearFrameAllocator() = default;
    void* LinearFrameAllocator::Allocate(size_t size, size_t alignment) noexcept { return nullptr; }
    void LinearFrameAllocator::Reset() noexcept { m_offset = 0; }
    size_t LinearFrameAllocator::GetAllocatedBytes() const noexcept { return m_offset; }
    size_t LinearFrameAllocator::GetCapacity() const noexcept { return m_capacity; }

    void RenderQueue::Submit(RenderSortKey key, void* cmd, CommandType type) { m_entries.push_back(RenderQueueEntry{key, cmd, type}); }
    void RenderQueue::Sort() noexcept { }
    void RenderQueue::Clear() noexcept { m_entries.clear(); }
    size_t RenderQueue::GetEntryCount() const noexcept { return m_entries.size(); }
    std::span<const RenderQueueEntry> RenderQueue::GetEntries() const noexcept { return m_entries; }
    
    void FrameStatisticsTracker::BeginFrame() noexcept { totalDrawCallsSubmitted=0; actualDrawCallsExecuted=0; drawCallsSavedByBatching=0; stateChangesAttempted=0; stateChangesSkippedByDeduplication=0; sortTimeMicroseconds=0; executionTimeMicroseconds=0; }
    void FrameStatisticsTracker::EndFrame() noexcept {}
    FrameStatsSummary FrameStatisticsTracker::GetSummary() const noexcept { return FrameStatsSummary{}; }
    std::string FrameStatisticsTracker::FormatTelemetryJSON() const { return "{}"; }
    
    void RenderPipelineExecutor::ExecuteQueue(LPDIRECT3DDEVICE9 dev, ExecutorRenderQueue& queue) noexcept {}
}

TEST_CASE("FramePipelineCoordinator Full Cycle")
{
    // Mock device cast
    IDirect3DDevice9* mockDevice = nullptr;
    EterLib::Render::FramePipelineCoordinator coordinator(mockDevice);

    SUBCASE("BeginFrame resets state") {
        coordinator.BeginFrame();
        CHECK(coordinator.GetAllocator().GetAllocatedBytes() == 0);
        CHECK(coordinator.GetRenderQueue().GetEntryCount() == 0);
    }

    SUBCASE("PhaseCollect to PhaseDispatch flow") {
        coordinator.BeginFrame();
        
        EterLib::Render::RenderSortKey key; key.value = 100;
        coordinator.GetRenderQueue().Submit(key, nullptr, EterLib::Render::CommandType::Draw);
        
        coordinator.PhaseCollect(); 
        coordinator.PhaseSort();
        coordinator.PhaseDispatch();
        coordinator.EndFrame();
    }
}

