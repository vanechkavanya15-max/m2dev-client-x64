#include <iostream>
#include <cassert>
#include <unordered_map>
#include <cstring>

// Dummy d3d9 types
typedef unsigned long DWORD;
typedef int BOOL;

struct D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};

struct D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};

enum D3DRENDERSTATETYPE {
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
};

enum D3DMATERIALCOLORSOURCE {
    D3DMCS_MATERIAL = 0,
    D3DMCS_COLOR1   = 1,
    D3DMCS_COLOR2   = 2,
};

// Dummy STATEMANAGER mock
struct MockStateManager {
    std::unordered_map<DWORD, DWORD> renderStates;
    D3DMATERIAL9 currentMaterial;

    int getMaterialCalls = 0;
    int setMaterialCalls = 0;
    int getRenderStateCalls = 0;
    int setRenderStateCalls = 0;

    void GetMaterial(D3DMATERIAL9* pMaterial) {
        *pMaterial = currentMaterial;
        getMaterialCalls++;
    }

    void SetMaterial(const D3DMATERIAL9* pMaterial) {
        currentMaterial = *pMaterial;
        setMaterialCalls++;
    }

    void GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue) {
        *pdwValue = renderStates[Type];
        getRenderStateCalls++;
    }

    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) {
        renderStates[Type] = Value;
        setRenderStateCalls++;
    }

    void ResetCounters() {
        getMaterialCalls = 0;
        setMaterialCalls = 0;
        getRenderStateCalls = 0;
        setRenderStateCalls = 0;
    }
};

MockStateManager STATEMANAGER;

// We provide dummy StdAfx.h by defining the mock headers during compilation.
#include "../src/EterLib/Render/MaterialScope.h"

using namespace EterLib::Render;

void TestMaterialScopeBasic()
{
    STATEMANAGER.ResetCounters();
    
    // Ustawienie początkowe
    D3DMATERIAL9 defaultMat;
    std::memset(&defaultMat, 0, sizeof(D3DMATERIAL9));
    defaultMat.Diffuse.r = 1.0f;
    STATEMANAGER.SetMaterial(&defaultMat);
    STATEMANAGER.SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    
    STATEMANAGER.ResetCounters();
    
    // Nowy materiał do przetestowania
    D3DMATERIAL9 newMat;
    std::memset(&newMat, 0, sizeof(D3DMATERIAL9));
    newMat.Diffuse.g = 1.0f;
    
    {
        MaterialScope scope(newMat, D3DMCS_MATERIAL);
        
        // Sprawdzenie czy ustawiono nowe wartości
        assert(STATEMANAGER.currentMaterial.Diffuse.g == 1.0f);
        assert(STATEMANAGER.renderStates[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_MATERIAL);
        
        // Sprawdzenie liczby wywołań pobierania i ustawiania
        assert(STATEMANAGER.getMaterialCalls == 1);
        assert(STATEMANAGER.getRenderStateCalls == 1);
        assert(STATEMANAGER.setMaterialCalls == 1);
        assert(STATEMANAGER.setRenderStateCalls == 1);
    }
    
    // Po wyjściu ze scope przywracane są stare wartości
    assert(STATEMANAGER.currentMaterial.Diffuse.r == 1.0f);
    assert(STATEMANAGER.currentMaterial.Diffuse.g == 0.0f);
    assert(STATEMANAGER.renderStates[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1);
    
    // Sprawdzenie liczby wywołań powrotu wartości
    assert(STATEMANAGER.setMaterialCalls == 2);
    assert(STATEMANAGER.setRenderStateCalls == 2);
    
    std::cout << "TestMaterialScopeBasic passed." << std::endl;
}

void TestMaterialScopeDefaultArg()
{
    STATEMANAGER.ResetCounters();
    
    D3DMATERIAL9 defaultMat;
    std::memset(&defaultMat, 0, sizeof(D3DMATERIAL9));
    STATEMANAGER.SetMaterial(&defaultMat);
    STATEMANAGER.SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR2);
    
    STATEMANAGER.ResetCounters();
    
    D3DMATERIAL9 newMat;
    std::memset(&newMat, 0, sizeof(D3DMATERIAL9));
    
    {
        // Sprawdzenie czy domyślny argument (D3DMCS_MATERIAL) działa poprawnie
        MaterialScope scope(newMat);
        assert(STATEMANAGER.renderStates[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_MATERIAL);
    }
    
    assert(STATEMANAGER.renderStates[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR2);
    
    std::cout << "TestMaterialScopeDefaultArg passed." << std::endl;
}

int main()
{
    TestMaterialScopeBasic();
    TestMaterialScopeDefaultArg();
    std::cout << "All C++26 RAII MaterialScope tests passed successfully!" << std::endl;
    return 0;
}
