#include "../src/EterLib/Render/InstancedMeshCollector.h"
#include <cassert>
#include <iostream>

using namespace EterLib::Render;

void TestEmptyCollector()
{
    InstancedMeshCollector collector;
    auto batches = collector.GetBatches();
    assert(batches.empty());
    std::cout << "TestEmptyCollector passed.\n";
}

void TestSingleInstance()
{
    InstancedMeshCollector collector;
    
    D3DMATRIX mat = {};
    mat._11 = 1.0f;
    mat._22 = 1.0f;
    mat._33 = 1.0f;
    mat._44 = 1.0f;
    
    collector.AddInstance(1001, mat);
    auto batches = collector.GetBatches();
    
    assert(batches.size() == 1);
    assert(batches[0].meshId == 1001);
    assert(batches[0].matrices.size() == 1);
    assert(batches[0].matrices[0]._11 == 1.0f);
    
    std::cout << "TestSingleInstance passed.\n";
}

void TestMultipleIdenticalInstances()
{
    InstancedMeshCollector collector;
    
    // Zgodnie z wymaganiami zadania (100 potworow tego samego vnum)
    for (int i = 0; i < 100; ++i)
    {
        D3DMATRIX mat = {};
        mat._41 = static_cast<float>(i);
        collector.AddInstance(2001, mat);
    }
    
    auto batches = collector.GetBatches();
    
    assert(batches.size() == 1);
    assert(batches[0].meshId == 2001);
    assert(batches[0].matrices.size() == 100);
    
    for (int i = 0; i < 100; ++i)
    {
        assert(batches[0].matrices[i]._41 == static_cast<float>(i));
    }
    
    std::cout << "TestMultipleIdenticalInstances passed.\n";
}

void TestMultipleVnumInstances()
{
    InstancedMeshCollector collector;
    
    for (int i = 0; i < 50; ++i)
    {
        D3DMATRIX mat = {};
        mat._42 = static_cast<float>(i);
        collector.AddInstance(3001, mat);
    }
    
    for (int i = 0; i < 50; ++i)
    {
        D3DMATRIX mat = {};
        mat._43 = static_cast<float>(i);
        collector.AddInstance(4001, mat);
    }
    
    auto batches = collector.GetBatches();
    
    assert(batches.size() == 2);
    
    // Kolejnosc nie jest scisle gwarantowana przez specyfikacje (ale przez nasza implementacje bedzie wg kolejnosci dodawania pierwszego elementu)
    bool found3001 = false;
    bool found4001 = false;
    
    for (const auto& batch : batches)
    {
        if (batch.meshId == 3001)
        {
            assert(batch.matrices.size() == 50);
            assert(batch.matrices[0]._42 == 0.0f);
            assert(batch.matrices[49]._42 == 49.0f);
            found3001 = true;
        }
        else if (batch.meshId == 4001)
        {
            assert(batch.matrices.size() == 50);
            assert(batch.matrices[0]._43 == 0.0f);
            assert(batch.matrices[49]._43 == 49.0f);
            found4001 = true;
        }
    }
    
    assert(found3001 && found4001);
    
    std::cout << "TestMultipleVnumInstances passed.\n";
}

void TestReset()
{
    InstancedMeshCollector collector;
    
    D3DMATRIX mat = {};
    collector.AddInstance(1001, mat);
    
    assert(collector.GetBatches().size() == 1);
    
    collector.Reset();
    
    assert(collector.GetBatches().empty());
    
    std::cout << "TestReset passed.\n";
}

void TestInterleavedAdds()
{
    InstancedMeshCollector collector;
    
    for (int i = 0; i < 10; ++i)
    {
        D3DMATRIX mat = {};
        mat._41 = static_cast<float>(i);
        collector.AddInstance(i % 2 == 0 ? 100 : 200, mat);
    }
    
    auto batches = collector.GetBatches();
    assert(batches.size() == 2);
    
    for (const auto& batch : batches)
    {
        assert(batch.matrices.size() == 5);
        if (batch.meshId == 100)
        {
            assert(batch.matrices[0]._41 == 0.0f);
            assert(batch.matrices[4]._41 == 8.0f);
        }
        else
        {
            assert(batch.matrices[0]._41 == 1.0f);
            assert(batch.matrices[4]._41 == 9.0f);
        }
    }
    std::cout << "TestInterleavedAdds passed.\n";
}

void TestMultipleGetBatchesCalls()
{
    InstancedMeshCollector collector;
    
    D3DMATRIX mat = {};
    collector.AddInstance(500, mat);
    
    auto batches1 = collector.GetBatches();
    assert(batches1.size() == 1);
    
    auto batches2 = collector.GetBatches();
    assert(batches2.size() == 1);
    assert(batches1.data() == batches2.data()); // Should point to the same cached data
    
    collector.AddInstance(500, mat);
    
    auto batches3 = collector.GetBatches();
    assert(batches3.size() == 1);
    assert(batches3[0].matrices.size() == 2);
    
    std::cout << "TestMultipleGetBatchesCalls passed.\n";
}

int main()
{
    TestEmptyCollector();
    TestSingleInstance();
    TestMultipleIdenticalInstances();
    TestMultipleVnumInstances();
    TestReset();
    TestInterleavedAdds();
    TestMultipleGetBatchesCalls();
    
    std::cout << "All InstancedMeshCollector tests passed successfully.\n";
    return 0;
}

