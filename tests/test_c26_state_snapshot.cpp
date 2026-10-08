#include <iostream>
#include <cassert>
#include <map>

// --- Mocking Dependencies ---
typedef unsigned long DWORD;

struct D3DXMATRIX {
    float m[4][4];
};

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7, D3DRS_ZWRITEENABLE = 14, D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_CULLMODE = 22, D3DRS_LIGHTING = 137, D3DRS_FOGENABLE = 28,
    D3DRS_SRCBLEND = 19, D3DRS_DESTBLEND = 20
};

enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1, D3DTSS_ALPHAOP = 4
};

enum D3DSAMPLERSTATETYPE {
    D3DSAMP_MINFILTER = 5, D3DSAMP_MAGFILTER = 6, D3DSAMP_MIPFILTER = 7
};

enum D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2, D3DTS_PROJECTION = 3, D3DTS_WORLD = 256
};

typedef void* LPDIRECT3DBASETEXTURE9;

// Mock CStateManager
class MockStateManager {
public:
    std::map<D3DRENDERSTATETYPE, DWORD> renderStates;
    std::map<std::pair<DWORD, D3DTEXTURESTAGESTATETYPE>, DWORD> textureStageStates;
    std::map<std::pair<DWORD, D3DSAMPLERSTATETYPE>, DWORD> samplerStates;
    std::map<DWORD, LPDIRECT3DBASETEXTURE9> textures;
    std::map<D3DTRANSFORMSTATETYPE, D3DXMATRIX> transforms;

    static MockStateManager& Instance() { static MockStateManager instance; return instance; }

    void GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue) { *pdwValue = renderStates[Type]; }
    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) { renderStates[Type] = Value; }

    void GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pdwValue) { *pdwValue = textureStageStates[{dwStage, Type}]; }
    void SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD dwValue) { textureStageStates[{dwStage, Type}] = dwValue; }

    void GetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD* pdwValue) { *pdwValue = samplerStates[{dwStage, Type}]; }
    void SetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD dwValue) { samplerStates[{dwStage, Type}] = dwValue; }

    void GetTexture(DWORD dwStage, LPDIRECT3DBASETEXTURE9* ppTexture) { *ppTexture = textures[dwStage]; }
    void SetTexture(DWORD dwStage, LPDIRECT3DBASETEXTURE9 pTexture) { textures[dwStage] = pTexture; }

    void GetTransform(D3DTRANSFORMSTATETYPE Type, D3DXMATRIX* pMatrix) { *pMatrix = transforms[Type]; }
    void SetTransform(D3DTRANSFORMSTATETYPE Type, const D3DXMATRIX* pMatrix) { transforms[Type] = *pMatrix; }
};

#define STATEMANAGER (MockStateManager::Instance())

#define IS_TESTING 1
namespace EterLib::Render {} // Pre-declare so includes work if needed

// We simulate headers being empty so we can include the cpp file directly
#define D3D9_H_MOCKED
#define D3DX9_H_MOCKED
#define STATEMANAGER_H_MOCKED

// Include the target to compile it as one unit
#include "../src/EterLib/Render/RenderStateSnapshot.cpp"

void Tests()
{
    EterLib::Render::RenderStateSnapshot snapshots[5];
    for (int i = 0; i < 5; ++i) snapshots[i].Capture();
    for (int i = 0; i < 5; ++i) snapshots[i].Apply();
}

int main()
{
    std::cout << "Running tests...\n";

    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, 1);
    STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, 6);

    EterLib::Render::RenderStateSnapshot snapshot;
    snapshot.Apply(); // Shouldn't crash

    snapshot.Capture();
    
    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, 0);
    snapshot.Apply();

    DWORD zEnable = 0;
    STATEMANAGER.GetRenderState(D3DRS_ZENABLE, &zEnable);
    assert(zEnable == 1);

    Tests();
    static_assert(sizeof(snapshot) > 0);
    
    std::cout << "Success!\n";
    return 0;
}
