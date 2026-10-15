#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "EterLib/Render/FrameMemoryPool.h"

struct TestNode
{
    uint32_t id{0};
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    TestNode() = default;
    TestNode(uint32_t inId, float inX, float inY, float inZ)
        : id(inId), x(inX), y(inY), z(inZ) {}
};

struct TrackerNode
{
    static inline int liveCount{0};

    int value{0};

    explicit TrackerNode(int v) : value(v)
    {
        ++liveCount;
    }

    ~TrackerNode()
    {
        --liveCount;
    }
};

void TestBasicAllocation()
{
    EterLib::Render::FrameMemoryPool<TestNode, 16> pool;
    assert(pool.ActiveCount() == 0);
    assert(pool.Capacity() == 0);
    assert(pool.Empty());

    TestNode* n1 = pool.Allocate(1, 10.0f, 20.0f, 30.0f);
    assert(n1 != nullptr);
    assert(n1->id == 1);
    assert(n1->x == 10.0f);
    assert(pool.ActiveCount() == 1);
    assert(pool.Capacity() == 16);
    assert(!pool.Empty());

    TestNode* n2 = pool.Allocate(2, 40.0f, 50.0f, 60.0f);
    assert(n2 != nullptr);
    assert(n2->id == 2);
    assert(pool.ActiveCount() == 2);

    pool.Deallocate(n1);
    assert(pool.ActiveCount() == 1);

    // Kolejna alokacja powinna ponownie uzyc zwolnionego slotu n1
    TestNode* n3 = pool.Allocate(3, 70.0f, 80.0f, 90.0f);
    assert(n3 == n1); // Zweryfikowano ponowne uzycie slotu z free-list
    assert(n3->id == 3);
    assert(pool.ActiveCount() == 2);

    pool.Deallocate(n2);
    pool.Deallocate(n3);
    assert(pool.ActiveCount() == 0);
    assert(pool.Empty());

    std::cout << "[PASS] TestBasicAllocation" << std::endl;
}

void TestDestructorInvocation()
{
    TrackerNode::liveCount = 0;
    {
        EterLib::Render::FrameMemoryPool<TrackerNode, 32> pool;
        std::vector<TrackerNode*> nodes;

        for (int i = 0; i < 50; ++i)
        {
            nodes.push_back(pool.Allocate(i));
        }

        assert(TrackerNode::liveCount == 50);
        assert(pool.ActiveCount() == 50);
        assert(pool.Capacity() >= 50);

        for (int i = 0; i < 20; ++i)
        {
            pool.Deallocate(nodes[i]);
        }

        assert(TrackerNode::liveCount == 30);
        assert(pool.ActiveCount() == 30);
    }

    std::cout << "[PASS] TestDestructorInvocation" << std::endl;
}

void TestMassAllocationAndReset()
{
    constexpr size_t kBlockSize = 64;
    EterLib::Render::FrameMemoryPool<TestNode, kBlockSize> pool;

    std::vector<TestNode*> nodes;
    nodes.reserve(1000);

    for (size_t i = 0; i < 1000; ++i)
    {
        nodes.push_back(pool.Allocate(static_cast<uint32_t>(i), 1.0f, 2.0f, 3.0f));
    }

    assert(pool.ActiveCount() == 1000);
    assert(pool.Capacity() >= 1000);

    pool.Reset();
    assert(pool.ActiveCount() == 0);
    assert(pool.Capacity() == 0);
    assert(pool.Empty());

    // Ponowna alokacja po resecie
    TestNode* fresh = pool.Allocate(9999, 0.0f, 0.0f, 0.0f);
    assert(fresh != nullptr);
    assert(fresh->id == 9999);
    assert(pool.ActiveCount() == 1);
    assert(pool.Capacity() == kBlockSize);

    std::cout << "[PASS] TestMassAllocationAndReset" << std::endl;
}

void TestMoveSemantics()
{
    EterLib::Render::FrameMemoryPool<TestNode, 16> poolA;
    TestNode* n1 = poolA.Allocate(100, 1.0f, 2.0f, 3.0f);
    assert(poolA.ActiveCount() == 1);

    EterLib::Render::FrameMemoryPool<TestNode, 16> poolB = std::move(poolA);
    assert(poolA.ActiveCount() == 0);
    assert(poolA.Capacity() == 0);
    assert(poolB.ActiveCount() == 1);
    assert(n1->id == 100);

    poolB.Deallocate(n1);
    assert(poolB.ActiveCount() == 0);

    std::cout << "[PASS] TestMoveSemantics" << std::endl;
}

int main()
{
    std::cout << "=== Running test_c26_mem_pool ===" << std::endl;
    TestBasicAllocation();
    TestDestructorInvocation();
    TestMassAllocationAndReset();
    TestMoveSemantics();
    std::cout << "=== All Memory Pool Tests PASSED ===" << std::endl;
    return 0;
}
