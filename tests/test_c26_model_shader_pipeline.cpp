#define TEST_MOCK_D3D9

#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>
#include <span>
#include <cstring>

typedef unsigned int UINT;

struct D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

struct D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};

// Mock IDirect3DDevice9
struct IDirect3DDevice9 {
    std::vector<float> vsConstants;
    
    IDirect3DDevice9() : vsConstants(1024, 0.0f) {}

    void SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) {
        if (StartRegister * 4 + Vector4fCount * 4 <= vsConstants.size()) {
            std::memcpy(&vsConstants[StartRegister * 4], pConstantData, Vector4fCount * 4 * sizeof(float));
        }
    }
};

typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

#include "../src/EterLib/Render/ModelShaderPipeline.cpp"

void TestModelShaderPipeline()
{
    EterLib::Render::ModelShaderPipeline pipeline;

    D3DMATRIX bone1 = {};
    bone1._11 = 1.0f;
    bone1._22 = 2.0f;
    D3DMATRIX bone2 = {};
    bone2._33 = 3.0f;
    bone2._44 = 4.0f;
    
    std::vector<D3DMATRIX> bones = {bone1, bone2};
    pipeline.SetBonePalette(bones);

    D3DCOLORVALUE diffuse = {0.5f, 0.6f, 0.7f, 1.0f};
    pipeline.SetMaterial(16.0f, diffuse);

    IDirect3DDevice9 mockDevice;
    pipeline.Bind(&mockDevice);

    // Verify diffuse color (register 4)
    assert(mockDevice.vsConstants[4 * 4 + 0] == 0.5f);
    assert(mockDevice.vsConstants[4 * 4 + 1] == 0.6f);
    assert(mockDevice.vsConstants[4 * 4 + 2] == 0.7f);
    assert(mockDevice.vsConstants[4 * 4 + 3] == 1.0f);

    // Verify specular power (register 5)
    assert(mockDevice.vsConstants[5 * 4 + 0] == 16.0f);
    assert(mockDevice.vsConstants[5 * 4 + 1] == 0.0f);

    // Verify bone palette (register 10, 8 registers for 2 matrices)
    assert(mockDevice.vsConstants[10 * 4 + 0] == 1.0f); // bone1._11
    assert(mockDevice.vsConstants[10 * 4 + 5] == 2.0f); // bone1._22
    assert(mockDevice.vsConstants[14 * 4 + 10] == 3.0f); // bone2._33
    assert(mockDevice.vsConstants[14 * 4 + 15] == 4.0f); // bone2._44
    
    std::cout << "All ModelShaderPipeline tests passed!\n";
}

int main()
{
    TestModelShaderPipeline();
    return 0;
}

