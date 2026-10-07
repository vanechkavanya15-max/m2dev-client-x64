#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "Client/World/TerrainHeightSampler.h"

using namespace Client::World;

void TestInitialization()
{
    TerrainHeightSampler sampler;
    
    // Test invalid zero dimensions
    auto res = sampler.Initialize(0, 0, {});
    assert(!res.has_value() && res.error() == TerrainError::InvalidGridSize);

    // Test mismatching data size
    auto res2 = sampler.Initialize(2, 2, {1.0f, 2.0f}); // expects 4 elements
    assert(!res2.has_value() && res2.error() == TerrainError::InvalidGridSize);

    // Test valid initialization
    auto res3 = sampler.Initialize(2, 2, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(res3.has_value());
}

void TestOutOfBounds()
{
    TerrainHeightSampler sampler;
    sampler.Initialize(3, 3, std::vector<float>(9, 0.0f));

    // Test Out Of Bounds negative
    auto h1 = sampler.GetHeight(-0.1f, 1.0f);
    assert(!h1.has_value() && h1.error() == TerrainError::OutOfBounds);

    // Test Out Of Bounds positive (max index is 2.0)
    auto h2 = sampler.GetHeight(2.1f, 1.0f);
    assert(!h2.has_value() && h2.error() == TerrainError::OutOfBounds);
}

void TestBilinearInterpolation()
{
    TerrainHeightSampler sampler;
    // 2x2 grid
    // 10.0  20.0
    // 30.0  40.0
    std::vector<float> heights = {
        10.0f, 20.0f,
        30.0f, 40.0f
    };
    sampler.Initialize(2, 2, heights);

    // Exact corners
    assert(sampler.GetHeight(0.0f, 0.0f).value() == 10.0f);
    assert(sampler.GetHeight(1.0f, 0.0f).value() == 20.0f);
    assert(sampler.GetHeight(0.0f, 1.0f).value() == 30.0f);
    assert(sampler.GetHeight(1.0f, 1.0f).value() == 40.0f);

    // Midpoints
    // Center of top edge (0.5, 0.0) -> lerp(10, 20, 0.5) = 15.0
    assert(sampler.GetHeight(0.5f, 0.0f).value() == 15.0f);
    
    // Center of left edge (0.0, 0.5) -> lerp(10, 30, 0.5) = 20.0
    assert(sampler.GetHeight(0.0f, 0.5f).value() == 20.0f);

    // Exact center (0.5, 0.5)
    // top = 15, bottom = 35 -> lerp(15, 35, 0.5) = 25.0
    assert(sampler.GetHeight(0.5f, 0.5f).value() == 25.0f);
}

void TestNormalCalculation()
{
    TerrainHeightSampler sampler;
    // 2x2 grid flat terrain
    sampler.Initialize(2, 2, {0.0f, 0.0f, 0.0f, 0.0f});
    auto n1 = sampler.GetNormal(0.5f, 0.5f).value();
    assert(n1.x == 0.0f && n1.y == 0.0f && n1.z == 1.0f);

    // Sloped terrain, rising in X direction
    // 0.0  1.0
    // 0.0  1.0
    sampler.Initialize(2, 2, {0.0f, 1.0f, 0.0f, 1.0f});
    auto n2 = sampler.GetNormal(0.5f, 0.5f).value();
    // tangentX = (1, 0, 1), tangentY = (0, 1, 0)
    // normal = (-1, 0, 1) normalized
    float expected_nx = -1.0f / std::sqrt(2.0f);
    float expected_ny = 0.0f;
    float expected_nz = 1.0f / std::sqrt(2.0f);
    
    assert(std::abs(n2.x - expected_nx) < 0.001f);
    assert(std::abs(n2.y - expected_ny) < 0.001f);
    assert(std::abs(n2.z - expected_nz) < 0.001f);
}

int main()
{
    std::cout << "Running TerrainHeightSampler tests...\n";
    TestInitialization();
    TestOutOfBounds();
    TestBilinearInterpolation();
    TestNormalCalculation();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
