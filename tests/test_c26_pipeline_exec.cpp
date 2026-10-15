#include "../src/EterLib/Render/RenderPipelineExecutor.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>

using namespace EterLib::Render;

// Global tracking for test assertions
static std::vector<std::string> executionLog;

namespace EterLib::Render {

    // Mock RadixSort64
    void RadixSort64::Sort(RenderEntry* entries, size_t count) {
        executionLog.push_back("RadixSort64::Sort");
        std::sort(entries, entries + count, [](const RenderEntry& a, const RenderEntry& b) {
            return a.key < b.key;
        });
    }

    // Mock PassDispatcher
    void PassDispatcher::Activate(uint8_t passId) {
        executionLog.push_back("PassDispatcher::Activate(" + std::to_string(passId) + ")");
    }

    // Mock RenderStateDeduplicator
    void RenderStateDeduplicator::Apply(LPDIRECT3DDEVICE9 dev) {
        (void)dev;
        executionLog.push_back("RenderStateDeduplicator::Apply");
    }

}

class MockRenderCommand : public RenderCommand {
public:
    int id;
    MockRenderCommand(int id) : id(id) {}
    void Execute(LPDIRECT3DDEVICE9 dev) override {
        (void)dev;
        executionLog.push_back("MockRenderCommand::Execute(" + std::to_string(id) + ")");
    }
};

class MockRenderQueue : public RenderQueue {
    std::vector<RenderEntry> entries;
public:
    void AddEntry(uint64_t key, RenderCommand* cmd) {
        entries.push_back({key, cmd});
    }
    RenderEntry* GetEntries() override {
        return entries.data();
    }
    size_t GetCount() const override {
        return entries.size();
    }
};

void TestRenderPipelineExecutor() {
    executionLog.clear();

    RenderPipelineExecutor executor;
    MockRenderQueue queue;

    // Keys format: passId (top 8 bits) ...
    // Pass 1: 0x01
    // Pass 2: 0x02
    MockRenderCommand cmd1(1);
    MockRenderCommand cmd2(2);
    MockRenderCommand cmd3(3);
    MockRenderCommand cmd4(4);

    queue.AddEntry((2ULL << 56) | 100, &cmd3); // Pass 2, key 100
    queue.AddEntry((1ULL << 56) | 200, &cmd2); // Pass 1, key 200
    queue.AddEntry((1ULL << 56) | 100, &cmd1); // Pass 1, key 100
    queue.AddEntry((2ULL << 56) | 200, &cmd4); // Pass 2, key 200

    LPDIRECT3DDEVICE9 dev = nullptr; // Mock device
    executor.ExecuteQueue(dev, queue);

    // Expected Sequence:
    // 1. RadixSort64::Sort
    // Ordered:
    // - (1ULL << 56) | 100 (cmd1)
    // - (1ULL << 56) | 200 (cmd2)
    // - (2ULL << 56) | 100 (cmd3)
    // - (2ULL << 56) | 200 (cmd4)
    //
    // 2. PassDispatcher::Activate(1)
    // 3. RenderStateDeduplicator::Apply
    // 4. MockRenderCommand::Execute(1)
    // 5. RenderStateDeduplicator::Apply
    // 6. MockRenderCommand::Execute(2)
    // 7. PassDispatcher::Activate(2)
    // 8. RenderStateDeduplicator::Apply
    // 9. MockRenderCommand::Execute(3)
    // 10. RenderStateDeduplicator::Apply
    // 11. MockRenderCommand::Execute(4)

    assert(executionLog.size() == 11);
    assert(executionLog[0] == "RadixSort64::Sort");
    assert(executionLog[1] == "PassDispatcher::Activate(1)");
    assert(executionLog[2] == "RenderStateDeduplicator::Apply");
    assert(executionLog[3] == "MockRenderCommand::Execute(1)");
    assert(executionLog[4] == "RenderStateDeduplicator::Apply");
    assert(executionLog[5] == "MockRenderCommand::Execute(2)");
    assert(executionLog[6] == "PassDispatcher::Activate(2)");
    assert(executionLog[7] == "RenderStateDeduplicator::Apply");
    assert(executionLog[8] == "MockRenderCommand::Execute(3)");
    assert(executionLog[9] == "RenderStateDeduplicator::Apply");
    assert(executionLog[10] == "MockRenderCommand::Execute(4)");

    std::cout << "All RenderPipelineExecutor tests passed!" << std::endl;
}

int main() {
    TestRenderPipelineExecutor();
    return 0;
}

