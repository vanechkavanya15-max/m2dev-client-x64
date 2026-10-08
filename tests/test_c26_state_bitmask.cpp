#include "../src/EterLib/Render/StateBitmask256.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <random>

using namespace EterLib::Render;

void TestInitialization()
{
    std::cout << "[TEST] TestInitialization...\n";
    StateBitmask256 mask;
    assert(!mask.IsAnyDirty() && "Mask should be clean upon initialization");
    for (size_t i = 0; i < 256; ++i)
    {
        assert(!mask.TestBit(i) && "All bits should be 0 upon initialization");
    }
    std::cout << "[OK] TestInitialization\n";
}

void TestSingleBits()
{
    std::cout << "[TEST] TestSingleBits...\n";
    for (size_t i = 0; i < 256; ++i)
    {
        StateBitmask256 mask;
        mask.SetBit(i);
        assert(mask.TestBit(i) && "Bit should be set");
        assert(mask.IsAnyDirty() && "Mask should be dirty after setting a bit");

        // Verify other bits are not set
        for (size_t j = 0; j < 256; ++j)
        {
            if (i != j)
            {
                assert(!mask.TestBit(j) && "Other bits should remain unset");
            }
        }

        mask.ClearBit(i);
        assert(!mask.TestBit(i) && "Bit should be cleared");
        assert(!mask.IsAnyDirty() && "Mask should be clean after clearing the only set bit");
    }
    std::cout << "[OK] TestSingleBits\n";
}

void TestOutOfBounds()
{
    std::cout << "[TEST] TestOutOfBounds...\n";
    StateBitmask256 mask;
    mask.SetBit(256); // Should do nothing
    assert(!mask.IsAnyDirty() && "Setting out of bounds should not affect mask");
    assert(!mask.TestBit(256) && "Testing out of bounds should return false");

    mask.SetBit(1000);
    assert(!mask.IsAnyDirty());
    assert(!mask.TestBit(1000));
    
    mask.ClearBit(300); // Should do nothing
    assert(!mask.IsAnyDirty());
    std::cout << "[OK] TestOutOfBounds\n";
}

void TestClearAll()
{
    std::cout << "[TEST] TestClearAll...\n";
    StateBitmask256 mask;
    mask.SetBit(0);
    mask.SetBit(63);
    mask.SetBit(64);
    mask.SetBit(127);
    mask.SetBit(128);
    mask.SetBit(191);
    mask.SetBit(192);
    mask.SetBit(255);
    
    assert(mask.IsAnyDirty());
    mask.ClearAll();
    assert(!mask.IsAnyDirty());
    
    for(size_t i=0; i<256; ++i)
    {
        assert(!mask.TestBit(i));
    }
    std::cout << "[OK] TestClearAll\n";
}

void TestStressPerformance()
{
    std::cout << "[TEST] TestStressPerformance...\n";
    StateBitmask256 mask;
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, 255);

    auto start = std::chrono::high_resolution_clock::now();

    const int iterations = 1000000;
    int dirtyCount = 0;

    for (int i = 0; i < iterations; ++i)
    {
        size_t bit = dist(rng);
        mask.SetBit(bit);
        if (mask.IsAnyDirty())
        {
            dirtyCount++;
        }
        mask.ClearBit(bit);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "[INFO] Stress test completed in " << diff.count() << " seconds.\n";
    assert(dirtyCount == iterations && "Should be dirty after every set");
    std::cout << "[OK] TestStressPerformance\n";
}

void TestRandomCombinations()
{
    std::cout << "[TEST] TestRandomCombinations...\n";
    StateBitmask256 mask;
    std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> dist(0, 255);
    
    std::vector<size_t> setBits;
    for(int i = 0; i < 50; ++i)
    {
        size_t bit = dist(rng);
        mask.SetBit(bit);
        setBits.push_back(bit);
    }
    
    assert(mask.IsAnyDirty());
    
    for(size_t bit : setBits)
    {
        assert(mask.TestBit(bit));
    }
    
    for(size_t bit : setBits)
    {
        mask.ClearBit(bit);
    }
    
    // There might be duplicates in setBits, so we just clear all in case any were left
    mask.ClearAll();
    assert(!mask.IsAnyDirty());
    
    std::cout << "[OK] TestRandomCombinations\n";
}

void TestSequentialFills()
{
    std::cout << "[TEST] TestSequentialFills...\n";
    StateBitmask256 mask;
    for (size_t i = 0; i < 256; ++i)
    {
        mask.SetBit(i);
        assert(mask.IsAnyDirty());
    }
    for (size_t i = 0; i < 256; ++i)
    {
        assert(mask.TestBit(i));
    }
    for (size_t i = 0; i < 256; ++i)
    {
        mask.ClearBit(i);
    }
    assert(!mask.IsAnyDirty());
    std::cout << "[OK] TestSequentialFills\n";
}

void TestBoundaryBits()
{
    std::cout << "[TEST] TestBoundaryBits...\n";
    StateBitmask256 mask;
    size_t boundaries[] = {0, 63, 64, 127, 128, 191, 192, 255};
    for(size_t b : boundaries)
    {
        mask.SetBit(b);
        assert(mask.TestBit(b));
        assert(mask.IsAnyDirty());
        mask.ClearBit(b);
        assert(!mask.TestBit(b));
        assert(!mask.IsAnyDirty());
    }
    std::cout << "[OK] TestBoundaryBits\n";
}

int main()
{
    std::cout << "Running StateBitmask256 Tests...\n";
    
    TestInitialization();
    TestSingleBits();
    TestOutOfBounds();
    TestClearAll();
    TestSequentialFills();
    TestBoundaryBits();
    TestRandomCombinations();
    TestStressPerformance();

    std::cout << "All StateBitmask256 tests passed successfully.\n";
    return 0;
}
