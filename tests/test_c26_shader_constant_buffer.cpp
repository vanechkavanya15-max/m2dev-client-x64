#define TEST_MODE_DISABLE_STDAFX
#include "../src/EterLib/Render/ShaderConstantBuffer.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

// Mock Device Implementation
class MockD3DDevice : public IDirect3DDevice9 {
public:
    struct CallRecord {
        uint32_t startRegister;
        uint32_t vector4fCount;
        std::vector<float> data;
    };

    std::vector<CallRecord> calls;

    int SetVertexShaderConstantF(uint32_t StartRegister, const float* pConstantData, uint32_t Vector4fCount) override {
        CallRecord record;
        record.startRegister = StartRegister;
        record.vector4fCount = Vector4fCount;
        record.data.assign(pConstantData, pConstantData + (Vector4fCount * 4));
        calls.push_back(record);
        return 0; // Success
    }
};

void TestSetVector4_SingleCall() {
    EterLib::Render::ShaderConstantBuffer buffer;
    MockD3DDevice dev;

    buffer.SetVector4(10, 1.0f, 2.0f, 3.0f, 4.0f);
    buffer.Commit(&dev);

    assert(dev.calls.size() == 1);
    assert(dev.calls[0].startRegister == 10);
    assert(dev.calls[0].vector4fCount == 1);
    assert(dev.calls[0].data[0] == 1.0f);
    assert(dev.calls[0].data[1] == 2.0f);
    assert(dev.calls[0].data[2] == 3.0f);
    assert(dev.calls[0].data[3] == 4.0f);

    std::cout << "TestSetVector4_SingleCall passed.\n";
}

void TestSetMatrix_SingleCall() {
    EterLib::Render::ShaderConstantBuffer buffer;
    MockD3DDevice dev;

    D3DMATRIX mat;
    std::memset(&mat, 0, sizeof(mat));
    mat._11 = 1.0f; mat._22 = 1.0f; mat._33 = 1.0f; mat._44 = 1.0f;

    buffer.SetMatrix(20, mat);
    buffer.Commit(&dev);

    assert(dev.calls.size() == 1);
    assert(dev.calls[0].startRegister == 20);
    assert(dev.calls[0].vector4fCount == 4);
    assert(dev.calls[0].data[0] == 1.0f);
    assert(dev.calls[0].data[5] == 1.0f);
    assert(dev.calls[0].data[10] == 1.0f);
    assert(dev.calls[0].data[15] == 1.0f);

    std::cout << "TestSetMatrix_SingleCall passed.\n";
}

void TestNoDuplicateCommit() {
    EterLib::Render::ShaderConstantBuffer buffer;
    MockD3DDevice dev;

    buffer.SetVector4(5, 0.5f, 0.5f, 0.5f, 0.5f);
    buffer.Commit(&dev);
    assert(dev.calls.size() == 1);

    // Call commit again without changing anything
    buffer.Commit(&dev);
    assert(dev.calls.size() == 1); // Should not increase

    // Set the same values again
    buffer.SetVector4(5, 0.5f, 0.5f, 0.5f, 0.5f);
    buffer.Commit(&dev);
    assert(dev.calls.size() == 1); // Should still not increase

    std::cout << "TestNoDuplicateCommit passed.\n";
}

void TestDirtyRangeTracking() {
    EterLib::Render::ShaderConstantBuffer buffer;
    MockD3DDevice dev;

    // Set register 10
    buffer.SetVector4(10, 1.0f, 1.0f, 1.0f, 1.0f);
    // Set register 15
    buffer.SetVector4(15, 2.0f, 2.0f, 2.0f, 2.0f);
    
    buffer.Commit(&dev);

    assert(dev.calls.size() == 1);
    assert(dev.calls[0].startRegister == 10);
    assert(dev.calls[0].vector4fCount == 6); // Range from 10 to 15 (inclusive) is 6 registers

    std::cout << "TestDirtyRangeTracking passed.\n";
}

int main() {
    TestSetVector4_SingleCall();
    TestSetMatrix_SingleCall();
    TestNoDuplicateCommit();
    TestDirtyRangeTracking();
    
    std::cout << "All ShaderConstantBuffer tests passed successfully.\n";
    return 0;
}

