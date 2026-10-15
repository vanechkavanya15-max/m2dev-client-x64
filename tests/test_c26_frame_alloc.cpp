#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>
#include <vector>
#include "../src/EterLib/Render/LinearFrameAllocator.h"

using EterLib::Render::LinearFrameAllocator;

struct TestObject {
    int x;
    double y;
    std::string s;

    TestObject(int x, double y, const std::string& s) : x(x), y(y), s(s) {}
};

struct alignas(32) AlignedObject {
    int data[8];
};

struct ComplexObject {
    std::vector<int> data;
    ComplexObject(std::initializer_list<int> list) : data(list) {}
    ~ComplexObject() {
        data.clear();
    }
};

void TestAllocationAndReset() {
    std::cout << "Running TestAllocationAndReset...\n";
    LinearFrameAllocator allocator(1024); // 1 KB capacity

    assert(allocator.GetCapacity() == 1024);
    assert(allocator.GetAllocatedBytes() == 0);

    void* ptr1 = allocator.Allocate(100);
    assert(ptr1 != nullptr);
    assert(allocator.GetAllocatedBytes() >= 100);

    void* ptr2 = allocator.Allocate(200);
    assert(ptr2 != nullptr);
    assert(allocator.GetAllocatedBytes() >= 300);

    // Reset should be O(1) and set offset to 0
    allocator.Reset();
    assert(allocator.GetAllocatedBytes() == 0);

    void* ptr3 = allocator.Allocate(50);
    assert(ptr3 != nullptr);
    // ptr3 does not have to be strictly equal to ptr1 because std::align modifies ptr, but it should be very close.
    assert(allocator.GetAllocatedBytes() >= 50); 
    std::cout << "TestAllocationAndReset passed.\n";
}

void TestObjectAllocation() {
    std::cout << "Running TestObjectAllocation...\n";
    LinearFrameAllocator allocator(4096);

    TestObject* obj = allocator.AllocateObject<TestObject>(42, 3.14, "Hello Allocator");
    assert(obj != nullptr);
    assert(obj->x == 42);
    assert(obj->y == 3.14);
    assert(obj->s == "Hello Allocator");

    // Manually call destructor as the allocator doesn't call destructors
    obj->~TestObject();
    std::cout << "TestObjectAllocation passed.\n";
}

void TestAlignment() {
    std::cout << "Running TestAlignment...\n";
    LinearFrameAllocator allocator(4096);
    
    // Allocate a single byte to offset the allocator
    (void)allocator.Allocate(1);

    // Allocate an object with 32-byte alignment requirement
    AlignedObject* alignedObj = allocator.AllocateObject<AlignedObject>();
    assert(alignedObj != nullptr);

    // Verify alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(alignedObj);
    assert((addr % 32) == 0);

    std::cout << "TestAlignment passed.\n";
}

void TestOutOfBounds() {
    std::cout << "Running TestOutOfBounds...\n";
    LinearFrameAllocator allocator(100);

    void* ptr1 = allocator.Allocate(60);
    assert(ptr1 != nullptr);

    // This should fail and return nullptr
    void* ptr2 = allocator.Allocate(50);
    assert(ptr2 == nullptr);

    // Allocator state should not change after failure
    assert(allocator.GetAllocatedBytes() >= 60 && allocator.GetAllocatedBytes() < 100);
    
    std::cout << "TestOutOfBounds passed.\n";
}

void TestMoveSemantics() {
    std::cout << "Running TestMoveSemantics...\n";
    LinearFrameAllocator allocator(2048);
    [[maybe_unused]] auto _ = allocator.Allocate(100);
    
    size_t allocatedBytes = allocator.GetAllocatedBytes();
    
    LinearFrameAllocator movedAllocator(std::move(allocator));
    
    assert(movedAllocator.GetCapacity() == 2048);
    assert(movedAllocator.GetAllocatedBytes() == allocatedBytes);
    
    // Source should be empty
    assert(allocator.GetCapacity() == 0);
    assert(allocator.GetAllocatedBytes() == 0);

    void* ptr = movedAllocator.Allocate(100);
    assert(ptr != nullptr);
    
    std::cout << "TestMoveSemantics passed.\n";
}

void TestEdgeCases() {
    std::cout << "Running TestEdgeCases...\n";
    LinearFrameAllocator allocator(1024);

    // Zero size allocation
    void* ptr1 = allocator.Allocate(0);
    assert(ptr1 == nullptr);

    // Requesting more than capacity
    void* ptr2 = allocator.Allocate(2048);
    assert(ptr2 == nullptr);

    std::cout << "TestEdgeCases passed.\n";
}

void TestComplexObjectAllocation() {
    std::cout << "Running TestComplexObjectAllocation...\n";
    LinearFrameAllocator allocator(8192);

    ComplexObject* obj = allocator.AllocateObject<ComplexObject>(std::initializer_list<int>{1, 2, 3, 4, 5});
    assert(obj != nullptr);
    assert(obj->data.size() == 5);
    assert(obj->data[0] == 1);
    assert(obj->data[4] == 5);

    obj->~ComplexObject();
    std::cout << "TestComplexObjectAllocation passed.\n";
}

void TestRepeatedAllocations() {
    std::cout << "Running TestRepeatedAllocations...\n";
    LinearFrameAllocator allocator(1024 * 1024); // 1 MB

    for (int i = 0; i < 1000; ++i) {
        void* ptr = allocator.Allocate(16);
        assert(ptr != nullptr);
    }
    
    assert(allocator.GetAllocatedBytes() >= 16000);
    
    allocator.Reset();
    assert(allocator.GetAllocatedBytes() == 0);
    
    std::cout << "TestRepeatedAllocations passed.\n";
}

void TestMoveAssignment() {
    std::cout << "Running TestMoveAssignment...\n";
    LinearFrameAllocator allocator1(1024);
    [[maybe_unused]] auto _ = allocator1.Allocate(50);
    
    LinearFrameAllocator allocator2(2048);
    allocator2 = std::move(allocator1);
    
    assert(allocator2.GetCapacity() == 1024);
    assert(allocator2.GetAllocatedBytes() >= 50);
    
    assert(allocator1.GetCapacity() == 0);
    assert(allocator1.GetAllocatedBytes() == 0);
    
    std::cout << "TestMoveAssignment passed.\n";
}


int main() {
    std::cout << "Starting tests for LinearFrameAllocator...\n";
    
    TestAllocationAndReset();
    TestObjectAllocation();
    TestAlignment();
    TestOutOfBounds();
    TestMoveSemantics();
    TestEdgeCases();
    TestComplexObjectAllocation();
    TestRepeatedAllocations();
    TestMoveAssignment();

    std::cout << "All tests passed successfully!\n";
    return 0;
}

