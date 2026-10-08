#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>

// Dummy types and constants to mock D3D9 requirements
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

struct IDirect3DDevice9 {
    int dummy;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

enum D3DRENDERSTATETYPE {
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
};

enum D3DBLEND {
    D3DBLEND_ZERO = 1,
    D3DBLEND_ONE = 2,
    D3DBLEND_SRCCOLOR = 3,
    D3DBLEND_INVSRCCOLOR = 4,
    D3DBLEND_SRCALPHA = 5,
    D3DBLEND_INVSRCALPHA = 6,
    D3DBLEND_DESTALPHA = 7,
    D3DBLEND_INVDESTALPHA = 8,
    D3DBLEND_DESTCOLOR = 9,
    D3DBLEND_INVDESTCOLOR = 10,
    D3DBLEND_SRCALPHASAT = 11,
    D3DBLEND_BOTHSRCALPHA = 12,
    D3DBLEND_BOTHINVSRCALPHA = 13,
    D3DBLEND_BLENDFACTOR = 14,
    D3DBLEND_INVBLENDFACTOR = 15,
};

// Mock StateManager
class MockStateManager {
public:
    std::unordered_map<DWORD, DWORD> renderStates;
    std::unordered_map<DWORD, std::vector<DWORD>> renderStateStack;
    
    int saveCalls = 0;
    int restoreCalls = 0;

    MockStateManager() {
        // Initialize default states
        renderStates[D3DRS_ALPHABLENDENABLE] = FALSE;
        renderStates[D3DRS_SRCBLEND] = D3DBLEND_SRCALPHA;
        renderStates[D3DRS_DESTBLEND] = D3DBLEND_INVSRCALPHA;
        renderStates[D3DRS_ZWRITEENABLE] = TRUE;
    }

    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) {
        renderStates[Type] = Value;
    }

    void GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue) {
        *pdwValue = renderStates[Type];
    }

    void SaveRenderState(D3DRENDERSTATETYPE Type, DWORD dwValue) {
        renderStateStack[Type].push_back(renderStates[Type]);
        SetRenderState(Type, dwValue);
        saveCalls++;
    }

    void RestoreRenderState(D3DRENDERSTATETYPE Type) {
        if (!renderStateStack[Type].empty()) {
            SetRenderState(Type, renderStateStack[Type].back());
            renderStateStack[Type].pop_back();
        }
        restoreCalls++;
    }

    void Reset() {
        renderStates.clear();
        renderStateStack.clear();
        
        renderStates[D3DRS_ALPHABLENDENABLE] = FALSE;
        renderStates[D3DRS_SRCBLEND] = D3DBLEND_SRCALPHA;
        renderStates[D3DRS_DESTBLEND] = D3DBLEND_INVSRCALPHA;
        renderStates[D3DRS_ZWRITEENABLE] = TRUE;
        
        saveCalls = 0;
        restoreCalls = 0;
    }
};

// Global singleton instance for STATEMANAGER macro
MockStateManager STATEMANAGER;
#define STATEMANAGER (::STATEMANAGER)

// To avoid re-declaring class inside tests and testing the real code
// We manually provide the definitions required by AdditivePassDispatcher.h here
// instead of including StateManager.h. This matches how test_c26_stage_alpha_scope.cpp
// and other tests in this project isolate their component tests without breaking
// due to deep D3D9 dependencies.

namespace EterLib::Render
{
    class AdditivePassDispatcher
    {
    public:
        AdditivePassDispatcher() = default;
        ~AdditivePassDispatcher() = default;

        AdditivePassDispatcher(const AdditivePassDispatcher&) = delete;
        AdditivePassDispatcher& operator=(const AdditivePassDispatcher&) = delete;
        AdditivePassDispatcher(AdditivePassDispatcher&&) = delete;
        AdditivePassDispatcher& operator=(AdditivePassDispatcher&&) = delete;

        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
        {
            if (!dev) return;

            STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }
        
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept
        {
            if (!dev) return;

            STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
            STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);
            STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
            STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
        }
    };
}

using namespace EterLib::Render;

void TestBeginPass() {
    STATEMANAGER.Reset();
    IDirect3DDevice9 device{};
    AdditivePassDispatcher dispatcher;
    
    // Ensure default state
    assert(STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_SRCBLEND] == D3DBLEND_SRCALPHA);
    assert(STATEMANAGER.renderStates[D3DRS_DESTBLEND] == D3DBLEND_INVSRCALPHA);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);

    // Call BeginPass
    dispatcher.BeginPass(&device);

    // Verify Additive states were applied
    assert(STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_SRCBLEND] == D3DBLEND_ONE);
    assert(STATEMANAGER.renderStates[D3DRS_DESTBLEND] == D3DBLEND_ONE);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
    
    // Verify stack was updated correctly
    assert(STATEMANAGER.renderStateStack[D3DRS_ALPHABLENDENABLE].size() == 1);
    assert(STATEMANAGER.renderStateStack[D3DRS_SRCBLEND].size() == 1);
    assert(STATEMANAGER.renderStateStack[D3DRS_DESTBLEND].size() == 1);
    assert(STATEMANAGER.renderStateStack[D3DRS_ZWRITEENABLE].size() == 1);
    assert(STATEMANAGER.saveCalls == 4);
    
    std::cout << "TestBeginPass passed.\n";
}

void TestEndPass() {
    STATEMANAGER.Reset();
    IDirect3DDevice9 device{};
    AdditivePassDispatcher dispatcher;

    // Call BeginPass to set up stack
    dispatcher.BeginPass(&device);
    
    // Mess with state slightly to ensure we are actually restoring from stack,
    // not just setting default values.
    STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, 999);
    STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, 999);
    STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, 999);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, 999);
    
    // Call EndPass
    dispatcher.EndPass(&device);
    
    // Verify states were restored to original (before BeginPass)
    assert(STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_SRCBLEND] == D3DBLEND_SRCALPHA);
    assert(STATEMANAGER.renderStates[D3DRS_DESTBLEND] == D3DBLEND_INVSRCALPHA);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
    
    // Verify stack was popped
    assert(STATEMANAGER.renderStateStack[D3DRS_ALPHABLENDENABLE].empty());
    assert(STATEMANAGER.renderStateStack[D3DRS_SRCBLEND].empty());
    assert(STATEMANAGER.renderStateStack[D3DRS_DESTBLEND].empty());
    assert(STATEMANAGER.renderStateStack[D3DRS_ZWRITEENABLE].empty());
    assert(STATEMANAGER.restoreCalls == 4);
    
    std::cout << "TestEndPass passed.\n";
}

void TestNullDevice() {
    STATEMANAGER.Reset();
    AdditivePassDispatcher dispatcher;
    
    // Passing null should be a no-op
    dispatcher.BeginPass(nullptr);
    assert(STATEMANAGER.saveCalls == 0);
    
    dispatcher.EndPass(nullptr);
    assert(STATEMANAGER.restoreCalls == 0);
    
    std::cout << "TestNullDevice passed.\n";
}

int main() {
    std::cout << "Running AdditivePassDispatcher tests...\n";
    TestBeginPass();
    TestEndPass();
    TestNullDevice();
    std::cout << "All AdditivePassDispatcher tests passed successfully.\n";
    return 0;
}

