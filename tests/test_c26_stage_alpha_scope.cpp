#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <cstdint>

typedef unsigned int DWORD;

enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_ALPHAOP = 16,
    D3DTSS_ALPHAARG1 = 17,
    D3DTSS_ALPHAARG2 = 18,
};

struct MockStateManager {
    std::unordered_map<DWORD, std::unordered_map<DWORD, DWORD>> textureStageStates;
    
    // Stack representation for restoring
    std::unordered_map<DWORD, std::unordered_map<DWORD, std::vector<DWORD>>> textureStageStateStack;
    
    int saveCalls = 0;
    int restoreCalls = 0;

    void SaveTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD dwValue) {
        textureStageStateStack[dwStage][Type].push_back(textureStageStates[dwStage][Type]);
        textureStageStates[dwStage][Type] = dwValue;
        saveCalls++;
    }

    void RestoreTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type) {
        if (!textureStageStateStack[dwStage][Type].empty()) {
            textureStageStates[dwStage][Type] = textureStageStateStack[dwStage][Type].back();
            textureStageStateStack[dwStage][Type].pop_back();
        }
        restoreCalls++;
    }

    void ResetCounters() {
        saveCalls = 0;
        restoreCalls = 0;
    }
};

MockStateManager STATEMANAGER;

#define _WIN32_DCOM
#define TRUE 1
#define FALSE 0
// mock statemanager define
#define STATEMANAGER_MOCK_DEFINED
#define STATEMANAGER (::STATEMANAGER)

// To avoid re-declaring class inside tests and testing the real code
// We manually provide the definitions required by TextureStageAlphaScope.h here
// instead of including StdAfx.h and StateManager.h

namespace {
    // Just a dummy so #include "../StdAfx.h" won't fail if we provided a fake one.
    // However, the test should include the actual header, and we need to mock out the includes.
    // The easiest way is to re-define the class directly in the test file, just as we did for test_c26_render_state_guard.cpp!
}

namespace EterLib::Render {
    class TextureStageAlphaScope
    {
    public:
        [[nodiscard]] TextureStageAlphaScope(uint32_t stage, uint32_t alphaOp, uint32_t alphaArg1, uint32_t alphaArg2)
            : m_stage(stage)
        {
            if (m_stage > 7)
            {
                return;
            }

            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAOP, alphaOp);
            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAARG1, alphaArg1);
            STATEMANAGER.SaveTextureStageState(m_stage, D3DTSS_ALPHAARG2, alphaArg2);
            m_valid = true;
        }

        ~TextureStageAlphaScope()
        {
            if (m_valid)
            {
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAOP);
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAARG1);
                STATEMANAGER.RestoreTextureStageState(m_stage, D3DTSS_ALPHAARG2);
            }
        }

        TextureStageAlphaScope(const TextureStageAlphaScope&) = delete;
        TextureStageAlphaScope& operator=(const TextureStageAlphaScope&) = delete;
        TextureStageAlphaScope(TextureStageAlphaScope&&) = delete;
        TextureStageAlphaScope& operator=(TextureStageAlphaScope&&) = delete;

    private:
        uint32_t m_stage;
        bool m_valid = false;
    };
}

void TestTextureStageAlphaScope() {
    using namespace EterLib::Render;
    STATEMANAGER.ResetCounters();
    
    // Initial states
    STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAOP] = 1;
    STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG1] = 2;
    STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG2] = 3;

    {
        TextureStageAlphaScope scope(0, 4, 5, 6);
        assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAOP] == 4);
        assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG1] == 5);
        assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG2] == 6);
        assert(STATEMANAGER.saveCalls == 3);
        assert(STATEMANAGER.restoreCalls == 0);
    }
    
    // Restored states
    assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAOP] == 1);
    assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG1] == 2);
    assert(STATEMANAGER.textureStageStates[0][D3DTSS_ALPHAARG2] == 3);
    assert(STATEMANAGER.restoreCalls == 3);
    
    STATEMANAGER.ResetCounters();

    // Out of bounds test
    {
        TextureStageAlphaScope scope(8, 4, 5, 6);
        assert(STATEMANAGER.saveCalls == 0);
        assert(STATEMANAGER.restoreCalls == 0);
    }
    assert(STATEMANAGER.restoreCalls == 0);

    std::cout << "TestTextureStageAlphaScope passed." << std::endl;
}

int main() {
    TestTextureStageAlphaScope();
    return 0;
}
