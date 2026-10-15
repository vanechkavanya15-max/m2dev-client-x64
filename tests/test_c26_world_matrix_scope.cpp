#include <cassert>
#include <iostream>
#include <vector>

// Mocking definitions required for testing without the full Metin2 client environment

// --- Mocking Direct3D ---
#define D3DTS_WORLD 256
typedef unsigned int D3DTRANSFORMSTATETYPE;

struct D3DXMATRIX {
    float m[4][4];
    
    D3DXMATRIX() {
        for(int i=0; i<4; ++i)
            for(int j=0; j<4; ++j)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }
    
    bool operator==(const D3DXMATRIX& other) const {
        for(int i=0; i<4; ++i)
            for(int j=0; j<4; ++j)
                if (m[i][j] != other.m[i][j]) return false;
        return true;
    }
};

void D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) {
    D3DXMATRIX out;
    for(int i=0; i<4; ++i) {
        for(int j=0; j<4; ++j) {
            out.m[i][j] = pM1->m[i][0] * pM2->m[0][j] +
                          pM1->m[i][1] * pM2->m[1][j] +
                          pM1->m[i][2] * pM2->m[2][j] +
                          pM1->m[i][3] * pM2->m[3][j];
        }
    }
    *pOut = out;
}

void D3DXMatrixTranslation(D3DXMATRIX* pOut, float x, float y, float z) {
    *pOut = D3DXMATRIX();
    pOut->m[3][0] = x;
    pOut->m[3][1] = y;
    pOut->m[3][2] = z;
}

void D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* pOut, float yaw, float pitch, float roll) {
    // Simplified mock for testing
    *pOut = D3DXMATRIX();
    pOut->m[0][1] = yaw;
    pOut->m[1][2] = pitch;
    pOut->m[2][0] = roll;
}

void D3DXMatrixScaling(D3DXMATRIX* pOut, float x, float y, float z) {
    *pOut = D3DXMATRIX();
    pOut->m[0][0] = x;
    pOut->m[1][1] = y;
    pOut->m[2][2] = z;
}

// --- Mocking CStateManager ---
class CStateManager {
public:
    static CStateManager& Instance() {
        static CStateManager instance;
        return instance;
    }

    void GetTransform(D3DTRANSFORMSTATETYPE Type, D3DXMATRIX* pMatrix) {
        if (Type == D3DTS_WORLD) {
            *pMatrix = m_worldMatrix;
        }
    }

    void SetTransform(D3DTRANSFORMSTATETYPE Type, const D3DXMATRIX* pMatrix) {
        if (Type == D3DTS_WORLD) {
            m_worldMatrix = *pMatrix;
        }
    }

    void SaveTransform(D3DTRANSFORMSTATETYPE Type, const D3DXMATRIX* pMatrix) {
        if (Type == D3DTS_WORLD) {
            m_stack.push_back(m_worldMatrix);
            m_worldMatrix = *pMatrix;
        }
    }

    void RestoreTransform(D3DTRANSFORMSTATETYPE Type) {
        if (Type == D3DTS_WORLD) {
            if (!m_stack.empty()) {
                m_worldMatrix = m_stack.back();
                m_stack.pop_back();
            }
        }
    }

    D3DXMATRIX m_worldMatrix;
    std::vector<D3DXMATRIX> m_stack;
};

#define STATEMANAGER (CStateManager::Instance())

#define MOCK_TESTING 1
#include "../src/EterLib/Render/WorldMatrixScope.h"

// --- Tests ---
void TestScopeRestoration() {
    D3DXMATRIX initial;
    initial.m[0][0] = 5.0f;
    STATEMANAGER.m_worldMatrix = initial;
    STATEMANAGER.m_stack.clear();

    {
        EterLib::Render::WorldMatrixScope scope;
        assert(STATEMANAGER.m_worldMatrix == initial);
        
        D3DXMATRIX temp;
        temp.m[0][0] = 10.0f;
        STATEMANAGER.SetTransform(D3DTS_WORLD, &temp);
        
        assert(STATEMANAGER.m_worldMatrix == temp);
    }
    
    assert(STATEMANAGER.m_worldMatrix == initial);
    assert(STATEMANAGER.m_stack.empty());
    std::cout << "TestScopeRestoration passed!" << std::endl;
}

void TestScopeWithApplyMatrix() {
    D3DXMATRIX initial;
    initial.m[0][0] = 2.0f;
    STATEMANAGER.m_worldMatrix = initial;
    STATEMANAGER.m_stack.clear();

    D3DXMATRIX apply;
    apply.m[0][0] = 3.0f;
    
    {
        EterLib::Render::WorldMatrixScope scope(&apply);
        
        D3DXMATRIX expected;
        D3DXMatrixMultiply(&expected, &apply, &initial);
        
        assert(STATEMANAGER.m_worldMatrix == expected);
        assert(scope.GetCurrentMatrix() == expected);
        assert(scope.GetOriginalMatrix() == initial);
    }
    
    assert(STATEMANAGER.m_worldMatrix == initial);
    assert(STATEMANAGER.m_stack.empty());
    std::cout << "TestScopeWithApplyMatrix passed!" << std::endl;
}

void TestScopeApplyMethods() {
    D3DXMATRIX initial;
    STATEMANAGER.m_worldMatrix = initial;
    STATEMANAGER.m_stack.clear();

    {
        EterLib::Render::WorldMatrixScope scope;
        
        scope.ApplyTranslation(10.0f, 20.0f, 30.0f);
        D3DXMATRIX current = STATEMANAGER.m_worldMatrix;
        assert(current.m[3][0] == 10.0f);
        assert(current.m[3][1] == 20.0f);
        assert(current.m[3][2] == 30.0f);

        scope.ResetToBaseState();
        assert(STATEMANAGER.m_worldMatrix == initial);
    }
    
    assert(STATEMANAGER.m_worldMatrix == initial);
    assert(STATEMANAGER.m_stack.empty());
    std::cout << "TestScopeApplyMethods passed!" << std::endl;
}

int main() {
    TestScopeRestoration();
    TestScopeWithApplyMatrix();
    TestScopeApplyMethods();
    std::cout << "All C++23 WorldMatrixScope tests passed!" << std::endl;
    return 0;
}
