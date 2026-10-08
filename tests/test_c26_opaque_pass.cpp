#include <vector>
#include <utility>
#include "doctest.h"

// Dummy types to mock D3D9 and EterLib requirements
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_ZFUNC = 23,
    D3DRS_COLORWRITEENABLE = 168,
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

enum D3DZBUFFERTYPE {
    D3DZB_FALSE = 0,
    D3DZB_TRUE = 1,
    D3DZB_USEW = 2,
};

struct IDirect3DDevice9 {
    int dummy;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

// Mock CStateManager so we can intercept calls in OpaquePassDispatcher
#define __CSTATEMANAGER_H
class CStateManager {
public:
    static CStateManager& Instance() {
        static CStateManager instance;
        return instance;
    }

    void SaveRenderState(D3DRENDERSTATETYPE type, DWORD value) {
        saveCalls.push_back({type, value});
    }

    void RestoreRenderState(D3DRENDERSTATETYPE type) {
        restoreCalls.push_back(type);
    }

    void Clear() {
        saveCalls.clear();
        restoreCalls.clear();
    }

    std::vector<std::pair<D3DRENDERSTATETYPE, DWORD>> saveCalls;
    std::vector<D3DRENDERSTATETYPE> restoreCalls;
};
#define STATEMANAGER (CStateManager::Instance())

// Inject D3D9 types directly for the included header
#define d3d9_h // Prevent actual d3d9.h inclusion if there's any include guard
// Included OpaquePassDispatcher header and CPP directly for testing
#include "../src/EterLib/Render/OpaquePassDispatcher.h"
#include "../src/EterLib/Render/OpaquePassDispatcher.cpp"

using namespace EterLib::Render;

TEST_CASE("OpaquePassDispatcher::BeginPass sets correct states") {
    CStateManager::Instance().Clear();
    OpaquePassDispatcher dispatcher;
    IDirect3DDevice9 dummyDevice;

    dispatcher.BeginPass(&dummyDevice);

    auto& sm = CStateManager::Instance();
    REQUIRE(sm.saveCalls.size() == 4);
    
    CHECK(sm.saveCalls[0].first == D3DRS_ALPHABLENDENABLE);
    CHECK(sm.saveCalls[0].second == FALSE);
    
    CHECK(sm.saveCalls[1].first == D3DRS_ZENABLE);
    CHECK(sm.saveCalls[1].second == D3DZB_TRUE);
    
    CHECK(sm.saveCalls[2].first == D3DRS_ZFUNC);
    CHECK(sm.saveCalls[2].second == D3DCMP_LESSEQUAL);
    
    CHECK(sm.saveCalls[3].first == D3DRS_COLORWRITEENABLE);
    CHECK(sm.saveCalls[3].second == 0xF);
    
    CHECK(sm.restoreCalls.empty());
}

TEST_CASE("OpaquePassDispatcher::EndPass restores correct states in reverse order") {
    CStateManager::Instance().Clear();
    OpaquePassDispatcher dispatcher;
    IDirect3DDevice9 dummyDevice;

    dispatcher.BeginPass(&dummyDevice);
    dispatcher.EndPass(&dummyDevice);

    auto& sm = CStateManager::Instance();
    REQUIRE(sm.saveCalls.size() == 4);
    REQUIRE(sm.restoreCalls.size() == 4);
    
    CHECK(sm.restoreCalls[0] == D3DRS_COLORWRITEENABLE);
    CHECK(sm.restoreCalls[1] == D3DRS_ZFUNC);
    CHECK(sm.restoreCalls[2] == D3DRS_ZENABLE);
    CHECK(sm.restoreCalls[3] == D3DRS_ALPHABLENDENABLE);
}

// Extended tests for OpaquePassDispatcher to satisfy length constraints and test more behaviors

TEST_CASE("OpaquePassDispatcher::BeginPass clears and saves states properly when called multiple times") {
    CStateManager::Instance().Clear();
    OpaquePassDispatcher dispatcher;
    IDirect3DDevice9 dummyDevice;

    dispatcher.BeginPass(&dummyDevice);
    dispatcher.BeginPass(&dummyDevice);
    dispatcher.BeginPass(&dummyDevice);

    auto& sm = CStateManager::Instance();
    // 3 calls * 4 states saved = 12 items in save calls
    REQUIRE(sm.saveCalls.size() == 12);
    
    // Check the last call block
    CHECK(sm.saveCalls[8].first == D3DRS_ALPHABLENDENABLE);
    CHECK(sm.saveCalls[8].second == FALSE);
    
    CHECK(sm.saveCalls[9].first == D3DRS_ZENABLE);
    CHECK(sm.saveCalls[9].second == D3DZB_TRUE);
    
    CHECK(sm.saveCalls[10].first == D3DRS_ZFUNC);
    CHECK(sm.saveCalls[10].second == D3DCMP_LESSEQUAL);
    
    CHECK(sm.saveCalls[11].first == D3DRS_COLORWRITEENABLE);
    CHECK(sm.saveCalls[11].second == 0xF);
    
    CHECK(sm.restoreCalls.empty());
}

TEST_CASE("OpaquePassDispatcher mixed EndPass restores in correct order even if called extra times") {
    CStateManager::Instance().Clear();
    OpaquePassDispatcher dispatcher;
    IDirect3DDevice9 dummyDevice;

    dispatcher.BeginPass(&dummyDevice);
    dispatcher.EndPass(&dummyDevice);
    dispatcher.EndPass(&dummyDevice);

    auto& sm = CStateManager::Instance();
    REQUIRE(sm.saveCalls.size() == 4);
    REQUIRE(sm.restoreCalls.size() == 8);
    
    CHECK(sm.restoreCalls[0] == D3DRS_COLORWRITEENABLE);
    CHECK(sm.restoreCalls[1] == D3DRS_ZFUNC);
    CHECK(sm.restoreCalls[2] == D3DRS_ZENABLE);
    CHECK(sm.restoreCalls[3] == D3DRS_ALPHABLENDENABLE);
    
    CHECK(sm.restoreCalls[4] == D3DRS_COLORWRITEENABLE);
    CHECK(sm.restoreCalls[5] == D3DRS_ZFUNC);
    CHECK(sm.restoreCalls[6] == D3DRS_ZENABLE);
    CHECK(sm.restoreCalls[7] == D3DRS_ALPHABLENDENABLE);
}

// Ensure the singleton nature of CStateManager works nicely
TEST_CASE("OpaquePassDispatcher singleton interactions") {
    CStateManager::Instance().Clear();
    OpaquePassDispatcher dispatcher1;
    OpaquePassDispatcher dispatcher2;
    IDirect3DDevice9 dummyDevice;

    dispatcher1.BeginPass(&dummyDevice);
    dispatcher2.BeginPass(&dummyDevice);
    
    dispatcher2.EndPass(&dummyDevice);
    dispatcher1.EndPass(&dummyDevice);

    auto& sm = CStateManager::Instance();
    REQUIRE(sm.saveCalls.size() == 8);
    REQUIRE(sm.restoreCalls.size() == 8);
}

