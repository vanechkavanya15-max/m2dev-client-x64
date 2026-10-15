#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <cstdint>

// -----------------------------------------------------------------------------
// D3D9 MOCK DEFINITIONS
// -----------------------------------------------------------------------------
typedef unsigned int DWORD;

enum D3DTEXTUREOP {
    D3DTOP_DISABLE = 1,
    D3DTOP_SELECTARG1 = 2,
    D3DTOP_MODULATE = 4,
};

#define D3DTA_TEXTURE 2
#define D3DTA_CURRENT 1

#define D3DTSS_COLOROP 1
#define D3DTSS_COLORARG1 2
#define D3DTSS_COLORARG2 3

enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP_TYPE = 1,
    D3DTSS_COLORARG1_TYPE = 2,
    D3DTSS_COLORARG2_TYPE = 3,
};

// -----------------------------------------------------------------------------
// MOCK STATE MANAGER
// -----------------------------------------------------------------------------
struct MockStateManager {
    std::unordered_map<DWORD, std::unordered_map<DWORD, std::vector<DWORD>>> textureStageStatesStack;
    std::unordered_map<DWORD, std::unordered_map<DWORD, DWORD>> textureStageStates;

    void SaveTextureStageState(DWORD stage, DWORD type, DWORD value) {
        textureStageStatesStack[stage][type].push_back(textureStageStates[stage][type]);
        textureStageStates[stage][type] = value;
    }

    void RestoreTextureStageState(DWORD stage, DWORD type) {
        if (!textureStageStatesStack[stage][type].empty()) {
            textureStageStates[stage][type] = textureStageStatesStack[stage][type].back();
            textureStageStatesStack[stage][type].pop_back();
        }
    }

    void SetTextureStageState(DWORD stage, DWORD type, DWORD value) {
        textureStageStates[stage][type] = value;
    }
    
    void GetTextureStageState(DWORD stage, DWORD type, DWORD* value) {
        *value = textureStageStates[stage][type];
    }

    static MockStateManager& Instance() {
        static MockStateManager instance;
        return instance;
    }

    void Reset() {
        textureStageStates.clear();
        textureStageStatesStack.clear();
    }
};

#define STATEMANAGER (MockStateManager::Instance())

namespace EterLib { namespace Render { } }
#define _WIN32
// In a standalone testing context without DX9, we remove the includes to avoid missing header errors.



namespace EterLib::Render
{
    class TextureStageColorScope
    {
    public:
        explicit TextureStageColorScope(DWORD stage) : m_stage(stage)
        {
            DWORD op = 0, arg1 = 0, arg2 = 0;
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, &op);
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, &arg1);
            STATEMANAGER.GetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, &arg2);

            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, op);
            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, arg1);
            STATEMANAGER.SaveTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, arg2);
        }

        ~TextureStageColorScope()
        {
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP);
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1);
            STATEMANAGER.RestoreTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2);
        }

        TextureStageColorScope(const TextureStageColorScope&) = delete;
        TextureStageColorScope& operator=(const TextureStageColorScope&) = delete;

        void Modulate()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_MODULATE);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, D3DTA_TEXTURE);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG2, D3DTA_CURRENT);
        }

        void SelectArg1()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLORARG1, D3DTA_TEXTURE);
        }

        void Disable()
        {
            STATEMANAGER.SetTextureStageState(m_stage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_COLOROP, D3DTOP_DISABLE);
        }

    private:
        DWORD m_stage;
    };
}

void TestInit() {
    MockStateManager::Instance().Reset();
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] = 0;
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG1] = 0;
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG2] = 0;

    {
        EterLib::Render::TextureStageColorScope scope(0);
        assert(MockStateManager::Instance().textureStageStatesStack[0][D3DTSS_COLOROP].size() == 1);
        assert(MockStateManager::Instance().textureStageStatesStack[0][D3DTSS_COLORARG1].size() == 1);
        assert(MockStateManager::Instance().textureStageStatesStack[0][D3DTSS_COLORARG2].size() == 1);
    }
    assert(MockStateManager::Instance().textureStageStatesStack[0][D3DTSS_COLOROP].empty());
    std::cout << "TestInit passed!" << std::endl;
}

void TestModulate() {
    MockStateManager::Instance().Reset();
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] = D3DTOP_DISABLE;
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG1] = 0;
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG2] = 0;

    {
        EterLib::Render::TextureStageColorScope scope(0);
        scope.Modulate();
        assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] == D3DTOP_MODULATE);
        assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG1] == D3DTA_TEXTURE);
        assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLORARG2] == D3DTA_CURRENT);
    }
    assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] == D3DTOP_DISABLE);
    std::cout << "TestModulate passed!" << std::endl;
}

void TestSelectArg1() {
    MockStateManager::Instance().Reset();
    MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] = D3DTOP_DISABLE;
    MockStateManager::Instance().textureStageStates[1][D3DTSS_COLORARG1] = 0;
    MockStateManager::Instance().textureStageStates[1][D3DTSS_COLORARG2] = 999; 

    {
        EterLib::Render::TextureStageColorScope scope(1);
        scope.SelectArg1();
        assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] == D3DTOP_SELECTARG1);
        assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLORARG1] == D3DTA_TEXTURE);
        assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLORARG2] == 999);
    }
    assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] == D3DTOP_DISABLE);
    std::cout << "TestSelectArg1 passed!" << std::endl;
}

void TestDisable() {
    MockStateManager::Instance().Reset();
    MockStateManager::Instance().textureStageStates[7][D3DTSS_COLOROP] = D3DTOP_MODULATE;

    {
        EterLib::Render::TextureStageColorScope scope(7);
        scope.Disable();
        assert(MockStateManager::Instance().textureStageStates[7][D3DTSS_COLOROP] == D3DTOP_DISABLE);
    }
    assert(MockStateManager::Instance().textureStageStates[7][D3DTSS_COLOROP] == D3DTOP_MODULATE);
    std::cout << "TestDisable passed!" << std::endl;
}

void TestMultipleStages() {
    MockStateManager::Instance().Reset();
    MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] = D3DTOP_DISABLE;
    MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] = D3DTOP_DISABLE;

    {
        EterLib::Render::TextureStageColorScope scope0(0);
        EterLib::Render::TextureStageColorScope scope1(1);
        
        scope0.Modulate();
        scope1.SelectArg1();

        assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] == D3DTOP_MODULATE);
        assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] == D3DTOP_SELECTARG1);
    }
    assert(MockStateManager::Instance().textureStageStates[0][D3DTSS_COLOROP] == D3DTOP_DISABLE);
    assert(MockStateManager::Instance().textureStageStates[1][D3DTSS_COLOROP] == D3DTOP_DISABLE);
    std::cout << "TestMultipleStages passed!" << std::endl;
}

int main() {
    std::cout << "Running TextureStageColorScope tests..." << std::endl;
    TestInit();
    TestModulate();
    TestSelectArg1();
    TestDisable();
    TestMultipleStages();
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
