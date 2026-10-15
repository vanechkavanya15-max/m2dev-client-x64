#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>

// Dummy d3d9 types and STATEMANAGER mock
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

enum D3DRENDERSTATETYPE {
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25
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

struct MockStateManager {
    std::unordered_map<DWORD, DWORD> renderStates;
    int setRenderStateCalls = 0;
    int getRenderStateCalls = 0;

    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) {
        renderStates[Type] = Value;
        setRenderStateCalls++;
    }

    void GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue) {
        auto it = renderStates.find(Type);
        if (it != renderStates.end()) {
            *pdwValue = it->second;
        } else {
            *pdwValue = 0; // default state if missing
        }
        getRenderStateCalls++;
    }

    void ResetCounters() {
        setRenderStateCalls = 0;
        getRenderStateCalls = 0;
    }
    
    void Clear() {
        renderStates.clear();
        ResetCounters();
    }
};

MockStateManager STATEMANAGER;

// Include the class to test
#include "../src/EterLib/Render/AlphaTestScope.h"

using namespace EterLib::Render;

void TestBasicScopeInitialization()
{
    STATEMANAGER.Clear();
    STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] = FALSE;
    STATEMANAGER.renderStates[D3DRS_ALPHAREF] = 0;
    STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
    STATEMANAGER.ResetCounters();

    {
        AlphaTestScope scope(true, 150, D3DCMP_LESS);

        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 150);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_LESS);
        assert(STATEMANAGER.getRenderStateCalls == 3);
        assert(STATEMANAGER.setRenderStateCalls == 3);
    }

    assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 0);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_ALWAYS);
    assert(STATEMANAGER.setRenderStateCalls == 6); // 3 on init, 3 on destroy
    
    std::cout << "TestBasicScopeInitialization passed." << std::endl;
}

void TestFactoryCreateForTreeLeaves()
{
    STATEMANAGER.Clear();
    STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] = FALSE;
    STATEMANAGER.renderStates[D3DRS_ALPHAREF] = 10;
    STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] = D3DCMP_LESS;
    STATEMANAGER.ResetCounters();

    {
        auto scope = AlphaTestScope::CreateForTreeLeaves();

        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 128);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_GREATEREQUAL);
        assert(STATEMANAGER.getRenderStateCalls == 3);
        assert(STATEMANAGER.setRenderStateCalls == 3);
    }

    assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 10);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_LESS);
    assert(STATEMANAGER.setRenderStateCalls == 6); // 3 on init, 3 on destroy

    std::cout << "TestFactoryCreateForTreeLeaves passed." << std::endl;
}

void TestMoveSemantics()
{
    STATEMANAGER.Clear();
    STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] = FALSE;
    STATEMANAGER.renderStates[D3DRS_ALPHAREF] = 0;
    STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
    STATEMANAGER.ResetCounters();

    {
        AlphaTestScope scope1(true, 200, D3DCMP_EQUAL);

        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == TRUE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 200);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_EQUAL);
        
        {
            AlphaTestScope scope2(std::move(scope1));
            
            // Stats should not change from move constructor
            assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == TRUE);
            assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 200);
            assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_EQUAL);
            assert(STATEMANAGER.getRenderStateCalls == 3);
            assert(STATEMANAGER.setRenderStateCalls == 3);
        }
        
        // Scope 2 is destroyed, should restore states
        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 0);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_ALWAYS);
        assert(STATEMANAGER.setRenderStateCalls == 6);
    }
    
    // Scope 1 is destroyed, should NOT restore states (already restored by scope 2)
    assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 0);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_ALWAYS);
    assert(STATEMANAGER.setRenderStateCalls == 6); // Remains 6

    std::cout << "TestMoveSemantics passed." << std::endl;
}

void TestMoveAssignment()
{
    STATEMANAGER.Clear();
    STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] = FALSE;
    STATEMANAGER.renderStates[D3DRS_ALPHAREF] = 10;
    STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] = D3DCMP_NEVER;
    STATEMANAGER.ResetCounters();

    {
        AlphaTestScope scope1(true, 50, D3DCMP_LESS);
        AlphaTestScope scope2(false, 20, D3DCMP_GREATER);
        
        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 20);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_GREATER);
        assert(STATEMANAGER.getRenderStateCalls == 6);
        assert(STATEMANAGER.setRenderStateCalls == 6);

        scope1 = std::move(scope2);
        
        // At this point, scope1 had its old states restored during assignment
        // Wait, the assignment operator in the header does:
        // if (m_active) restore_states...
        // Which means scope1's original restored states: FALSE, 10, D3DCMP_NEVER are pushed back to STATEMANAGER.
        
        // And scope2 is marked inactive.
        assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == FALSE);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 10);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_NEVER);
        assert(STATEMANAGER.setRenderStateCalls == 9);
    }

    // Now scope1 is destroyed, which holds the restored states for scope2 (which was initialized when state was TRUE, 50, LESS)
    // Wait, scope2's original states when it was constructed were TRUE, 50, LESS.
    assert(STATEMANAGER.renderStates[D3DRS_ALPHATESTENABLE] == TRUE);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAREF] == 50);
    assert(STATEMANAGER.renderStates[D3DRS_ALPHAFUNC] == D3DCMP_LESS);
    assert(STATEMANAGER.setRenderStateCalls == 12);
    
    std::cout << "TestMoveAssignment passed." << std::endl;
}

int main()
{
    TestBasicScopeInitialization();
    TestFactoryCreateForTreeLeaves();
    TestMoveSemantics();
    TestMoveAssignment();

    std::cout << "All AlphaTestScope tests passed successfully." << std::endl;
    return 0;
}
