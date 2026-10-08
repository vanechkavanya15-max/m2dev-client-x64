#include "../src/EterLib/Render/WaterSurfaceGenerator.h"
#include <iostream>
#include <cassert>

using namespace EterLib::Render;

int main()
{
    WaterSurfaceGenerator gen;
    std::vector<D3DVERTEX> vertices;
    std::vector<uint16_t> indices;

    gen.BuildWaterPatch(0.0f, 0.0f, 100.0f, 0.0f, vertices, indices);

    // 16x16 grid means 17x17 vertices
    assert(vertices.size() == 17 * 17);
    
    // 16x16 grid means 256 cells, each cell has 2 triangles, each triangle has 3 indices
    assert(indices.size() == 16 * 16 * 2 * 3);

    std::cout << "Test passed: WaterSurfaceGeneratorTest" << std::endl;
    return 0;
}

