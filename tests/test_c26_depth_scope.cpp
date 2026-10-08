#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>
// Dummy types to mock D3D9 and EterLib requirements
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ZFUNC = 23,
    D3DRS_STENCILENABLE = 52,
};
enum D3DCMPFUNC {
    D3DCMP_NEVER = 1,
    D3DCMP_LESS = 2,
    D3DCMP_EQUAL = 3,
    D3DCMP_LESSEQUAL = 4,
    D3DCMP_GREATER = 5,
    D3DCMP_NOTEQUAL = 6,
    D3DCMP_GREATEREQUAL = 7,
    D3DCMP_ALWAYS = 8,
};
class MockStateManager {
public:
    std::unordered_map<DWORD, DWORD> renderStates;
    std::unordered_map<DWORD, std::vector<DWORD>> renderStateStack;
    MockStateManager() {
        // Initialize default states
        renderStates[D3DRS_ZENABLE] = TRUE;
        renderStates[D3DRS_ZWRITEENABLE] = TRUE;
        renderStates[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
        renderStates[D3DRS_STENCILENABLE] = FALSE;
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
    }
    void RestoreRenderState(D3DRENDERSTATETYPE Type) {
        if (!renderStateStack[Type].empty()) {
            SetRenderState(Type, renderStateStack[Type].back());
            renderStateStack[Type].pop_back();
        }
    }
    void Reset() {
        renderStates.clear();
        renderStateStack.clear();
        renderStates[D3DRS_ZENABLE] = TRUE;
        renderStates[D3DRS_ZWRITEENABLE] = TRUE;
        renderStates[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
        renderStates[D3DRS_STENCILENABLE] = FALSE;
    }
};
// Global singleton instance for STATEMANAGER macro
MockStateManager STATEMANAGER;
// Mock CStateManager just for include purposes
class CStateManager {
public:
    static MockStateManager& Instance() {
        return STATEMANAGER;
    }
};
// Include the newly created headers
// For testing purposes, we need to bypass EterLib/StateManager.h which we can't easily include in isolated test
// so we'll stub out the file directly instead of full inclusion, or create a mock.
// Instead of including the real StateManager.h which depends on actual D3D9, 
// we will inject the code of DepthStencilScope directly here for the test.
#define __CSTATEMANAGER_H // Mock EterLib/StateManager.h include guard
#include "../src/EterLib/Render/DepthStencilScope.h"
using namespace EterLib::Render;
void TestReadWriteMode() {
    STATEMANAGER.Reset();
    // Mess up the state beforehand
    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, FALSE);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    STATEMANAGER.SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    STATEMANAGER.SetRenderState(D3DRS_STENCILENABLE, TRUE);
    {
        DepthStencilScope scope(DepthMode::ReadWrite);
        assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
        assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == FALSE);
    }
    // Assert restoration
    assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_ALWAYS);
    assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == TRUE);
    std::cout << "TestReadWriteMode passed.\n";
}
void TestReadOnlyMode() {
    STATEMANAGER.Reset();
    // Preset state
    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, FALSE);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    STATEMANAGER.SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
    STATEMANAGER.SetRenderState(D3DRS_STENCILENABLE, TRUE);
    {
        DepthStencilScope scope(DepthMode::ReadOnly);
        assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
        assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == FALSE);
    }
    // Assert restoration
    assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_EQUAL);
    assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == TRUE);
    std::cout << "TestReadOnlyMode passed.\n";
}
void TestDisabledMode() {
    STATEMANAGER.Reset();
    // Preset state
    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, TRUE);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    STATEMANAGER.SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    STATEMANAGER.SetRenderState(D3DRS_STENCILENABLE, TRUE);
    {
        DepthStencilScope scope(DepthMode::Disabled);
        assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_ALWAYS);
        assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == FALSE);
    }
    // Assert restoration
    assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
    assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == TRUE);
    std::cout << "TestDisabledMode passed.\n";
}
void TestNestedScopes() {
    STATEMANAGER.Reset();
    STATEMANAGER.SetRenderState(D3DRS_ZENABLE, TRUE);
    STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    STATEMANAGER.SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    STATEMANAGER.SetRenderState(D3DRS_STENCILENABLE, FALSE);
    {
        DepthStencilScope scope1(DepthMode::ReadOnly);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
        {
            DepthStencilScope scope2(DepthMode::Disabled);
            assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == FALSE);
            assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_ALWAYS);
            {
                DepthStencilScope scope3(DepthMode::ReadWrite);
                assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
                assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
                assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
            }
            // Should restore to scope2
            assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == FALSE);
            assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
            assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_ALWAYS);
        }
        // Should restore to scope1
        assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
    }
    // Should restore to original
    assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ZFUNC] == D3DCMP_LESSEQUAL);
    assert(STATEMANAGER.renderStates[D3DRS_STENCILENABLE] == FALSE);
    std::cout << "TestNestedScopes passed.\n";
}
int main() {
    std::cout << "Running DepthStencilScope tests...\n";
    TestReadWriteMode();
    TestReadOnlyMode();
    TestDisabledMode();
    TestNestedScopes();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
