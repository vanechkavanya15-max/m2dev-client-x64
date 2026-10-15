#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <stdexcept>

// --- Mocks ---
#define TEST_MODE_DISABLE_STDAFX
#ifndef _WIN32
#define _WIN32 1 // Force the mock to compile the Windows implementation path for testing
#endif

// Mock HRESULT
typedef long HRESULT;
#define S_OK ((HRESULT)0L)
#define E_FAIL ((HRESULT)-2147467259L) // E_FAIL as negative int
#define FAILED(hr) (((HRESULT)(hr)) < 0)

// Mock IDirect3DDevice9Ex
class IDirect3DDevice9Ex
{
public:
    std::vector<float> vConstants;
    std::vector<float> pConstants;
    bool shouldFailGetV = false;
    bool shouldFailGetP = false;

    IDirect3DDevice9Ex() : vConstants(256 * 4, 0.0f), pConstants(256 * 4, 0.0f) {}

    HRESULT GetVertexShaderConstantF(uint32_t startRegister, float* pConstantData, uint32_t registerCount)
    {
        if (shouldFailGetV) return E_FAIL;
        for (uint32_t i = 0; i < registerCount * 4; ++i)
        {
            pConstantData[i] = vConstants[startRegister * 4 + i];
        }
        return S_OK;
    }

    HRESULT GetPixelShaderConstantF(uint32_t startRegister, float* pConstantData, uint32_t registerCount)
    {
        if (shouldFailGetP) return E_FAIL;
        for (uint32_t i = 0; i < registerCount * 4; ++i)
        {
            pConstantData[i] = pConstants[startRegister * 4 + i];
        }
        return S_OK;
    }
};

typedef IDirect3DDevice9Ex* LPDIRECT3DDEVICE9EX;

// Mock CStateManager
class CStateManager
{
public:
    LPDIRECT3DDEVICE9EX m_pDevice;

    CStateManager(LPDIRECT3DDEVICE9EX pDevice) : m_pDevice(pDevice) {}

    LPDIRECT3DDEVICE9EX GetDevice() { return m_pDevice; }

    void SetVertexShaderConstant(uint32_t dwRegister, const void* pConstantData, uint32_t dwConstantCount)
    {
        const float* fData = static_cast<const float*>(pConstantData);
        for (uint32_t i = 0; i < dwConstantCount * 4; ++i)
        {
            m_pDevice->vConstants[dwRegister * 4 + i] = fData[i];
        }
    }

    void SetPixelShaderConstant(uint32_t dwRegister, const void* pConstantData, uint32_t dwConstantCount)
    {
        const float* fData = static_cast<const float*>(pConstantData);
        for (uint32_t i = 0; i < dwConstantCount * 4; ++i)
        {
            m_pDevice->pConstants[dwRegister * 4 + i] = fData[i];
        }
    }
};

// --- Include Target ---
#define CStateManager_INCLUDED // dummy
#include "../src/EterLib/Render/ShaderConstantScope.h"

using namespace EterLib::Render;

// --- Tests ---

void TestVertexShaderConstantScope()
{
    IDirect3DDevice9Ex mockDevice;
    CStateManager mockManager(&mockDevice);
    
    // Set initial state
    float initialConstants[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    mockManager.SetVertexShaderConstant(10, initialConstants, 2);

    float newConstants[8] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f };

    {
        ShaderConstantScope scope(&mockManager, ShaderConstantScope::ShaderType::Vertex, 10, newConstants, 2);

        // Verify it was set
        assert(mockDevice.vConstants[10 * 4 + 0] == 10.0f);
        assert(mockDevice.vConstants[10 * 4 + 7] == 80.0f);
    }

    // Verify it was restored
    assert(mockDevice.vConstants[10 * 4 + 0] == 1.0f);
    assert(mockDevice.vConstants[10 * 4 + 7] == 8.0f);
    
    std::cout << "TestVertexShaderConstantScope passed." << std::endl;
}

void TestPixelShaderConstantScope()
{
    IDirect3DDevice9Ex mockDevice;
    CStateManager mockManager(&mockDevice);
    
    // Set initial state
    float initialConstants[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
    mockManager.SetPixelShaderConstant(5, initialConstants, 1);

    float newConstants[4] = { 0.9f, 0.8f, 0.7f, 0.6f };

    {
        ShaderConstantScope scope(&mockManager, ShaderConstantScope::ShaderType::Pixel, 5, newConstants, 1);

        // Verify it was set
        assert(mockDevice.pConstants[5 * 4 + 0] == 0.9f);
        assert(mockDevice.pConstants[5 * 4 + 3] == 0.6f);
    }

    // Verify it was restored
    assert(mockDevice.pConstants[5 * 4 + 0] == 0.1f);
    assert(mockDevice.pConstants[5 * 4 + 3] == 0.4f);

    std::cout << "TestPixelShaderConstantScope passed." << std::endl;
}

void TestGetFailure()
{
    IDirect3DDevice9Ex mockDevice;
    CStateManager mockManager(&mockDevice);
    
    // Set initial state
    float initialConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    mockManager.SetVertexShaderConstant(0, initialConstants, 1);

    mockDevice.shouldFailGetV = true;

    float newConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    {
        ShaderConstantScope scope(&mockManager, ShaderConstantScope::ShaderType::Vertex, 0, newConstants, 1);

        // It shouldn't set because it failed to get
        assert(mockDevice.vConstants[0] == 1.0f);
    }

    // It shouldn't restore either
    assert(mockDevice.vConstants[0] == 1.0f);

    std::cout << "TestGetFailure passed." << std::endl;
}

int main()
{
    TestVertexShaderConstantScope();
    TestPixelShaderConstantScope();
    TestGetFailure();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
