#include "doctest.h"

// Prevent real d3d9.h from loading on MSVC to avoid COM boilerplate
#define _D3D9_H_
#define __D3D9_H__

// Mock definitions for Windows/DirectX types
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long HRESULT;

#define S_OK 0
#define E_FAIL -1
#define FAILED(hr) (((HRESULT)(hr)) < 0)

#define TRUE 1
#define FALSE 0

// DirectX Enums (as ints)
const int D3DRS_ZENABLE = 7;
const int D3DRS_ZWRITEENABLE = 14;
const int D3DRS_CULLMODE = 22;
const int D3DRS_LIGHTING = 137;

const int D3DCULL_NONE = 1;
const int D3DCULL_CW = 2;
const int D3DCULL_CCW = 3;

typedef int D3DRENDERSTATETYPE;

// Mock DirectX interfaces
struct IDirect3DDevice9 {
    int dummy = 0;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <iostream>

// Mock CStateManager
class CStateManager {
public:
    static CStateManager& Instance() {
        static CStateManager instance;
        return instance;
    }

    void SaveRenderState(D3DRENDERSTATETYPE Type, DWORD dwValue) {
        stateStack[Type].push_back(currentState[Type]);
        currentState[Type] = dwValue;
        saveCalls.push_back({Type, dwValue});
    }

    void RestoreRenderState(D3DRENDERSTATETYPE Type) {
        if (!stateStack[Type].empty()) {
            currentState[Type] = stateStack[Type].back();
            stateStack[Type].pop_back();
        }
        restoreCalls.push_back(Type);
    }

    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) {
        currentState[Type] = Value;
    }

    DWORD GetRenderState(D3DRENDERSTATETYPE Type) {
        return currentState[Type];
    }

    void ResetTestState() {
        stateStack.clear();
        currentState.clear();
        saveCalls.clear();
        restoreCalls.clear();
    }

    std::map<int, std::vector<DWORD>> stateStack;
    std::map<int, DWORD> currentState;
    
    std::vector<std::pair<int, DWORD>> saveCalls;
    std::vector<int> restoreCalls;
};

// Mock ModernLogger
namespace EterBase {
    class ModernLogger {
    public:
        static void Error(const std::string& msg) {
            lastError = msg;
        }
        static std::string lastError;
    };
    std::string ModernLogger::lastError = "";
}

// Mocks to bypass StdAfx.h requirements
#define __CSTATEMANAGER_H
#define STATEMANAGER (CStateManager::Instance())

namespace EterLib::Render {

class UIPassDispatcher {
public:
    UIPassDispatcher() = default;
    ~UIPassDispatcher() = default;

    UIPassDispatcher(const UIPassDispatcher&) = delete;
    UIPassDispatcher& operator=(const UIPassDispatcher&) = delete;

    void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept {
        if (!dev) {
            EterBase::ModernLogger::Error("UIPassDispatcher::BeginPass - null device");
            return;
        }

        auto& sm = CStateManager::Instance();
        sm.SaveRenderState(D3DRS_ZENABLE, FALSE);
        sm.SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
        sm.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        sm.SaveRenderState(D3DRS_LIGHTING, FALSE);
    }

    void EndPass(LPDIRECT3DDEVICE9 dev) noexcept {
        if (!dev) {
            EterBase::ModernLogger::Error("UIPassDispatcher::EndPass - null device");
            return;
        }

        auto& sm = CStateManager::Instance();
        sm.RestoreRenderState(D3DRS_LIGHTING);
        sm.RestoreRenderState(D3DRS_CULLMODE);
        sm.RestoreRenderState(D3DRS_ZWRITEENABLE);
        sm.RestoreRenderState(D3DRS_ZENABLE);
    }
};

} // namespace EterLib::Render

TEST_CASE("UIPassDispatcher sets and restores UI RenderStates") {
    auto& sm = CStateManager::Instance();
    sm.ResetTestState();

    // Set initial states
    sm.SetRenderState(D3DRS_ZENABLE, TRUE);
    sm.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    sm.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    sm.SetRenderState(D3DRS_LIGHTING, TRUE);

    IDirect3DDevice9 mockDevice;
    EterLib::Render::UIPassDispatcher dispatcher;

    SUBCASE("BeginPass correctly sets states") {
        dispatcher.BeginPass(&mockDevice);

        CHECK(sm.GetRenderState(D3DRS_ZENABLE) == FALSE);
        CHECK(sm.GetRenderState(D3DRS_ZWRITEENABLE) == FALSE);
        CHECK(sm.GetRenderState(D3DRS_CULLMODE) == static_cast<DWORD>(D3DCULL_NONE));
        CHECK(sm.GetRenderState(D3DRS_LIGHTING) == FALSE);

        REQUIRE(sm.saveCalls.size() == 4);
        CHECK(sm.saveCalls[0].first == D3DRS_ZENABLE);
        CHECK(sm.saveCalls[1].first == D3DRS_ZWRITEENABLE);
        CHECK(sm.saveCalls[2].first == D3DRS_CULLMODE);
        CHECK(sm.saveCalls[3].first == D3DRS_LIGHTING);
    }

    SUBCASE("EndPass correctly restores states") {
        dispatcher.BeginPass(&mockDevice);
        
        sm.restoreCalls.clear(); // Clear to check only EndPass

        dispatcher.EndPass(&mockDevice);

        CHECK(sm.GetRenderState(D3DRS_ZENABLE) == TRUE);
        CHECK(sm.GetRenderState(D3DRS_ZWRITEENABLE) == TRUE);
        CHECK(sm.GetRenderState(D3DRS_CULLMODE) == static_cast<DWORD>(D3DCULL_CW));
        CHECK(sm.GetRenderState(D3DRS_LIGHTING) == TRUE);

        REQUIRE(sm.restoreCalls.size() == 4);
        CHECK(sm.restoreCalls[0] == D3DRS_LIGHTING);
        CHECK(sm.restoreCalls[1] == D3DRS_CULLMODE);
        CHECK(sm.restoreCalls[2] == D3DRS_ZWRITEENABLE);
        CHECK(sm.restoreCalls[3] == D3DRS_ZENABLE);
    }
}

TEST_CASE("UIPassDispatcher handles null device") {
    auto& sm = CStateManager::Instance();
    sm.ResetTestState();
    EterBase::ModernLogger::lastError = "";

    EterLib::Render::UIPassDispatcher dispatcher;

    SUBCASE("BeginPass with null device") {
        dispatcher.BeginPass(nullptr);
        CHECK(EterBase::ModernLogger::lastError == "UIPassDispatcher::BeginPass - null device");
        CHECK(sm.saveCalls.empty());
    }

    SUBCASE("EndPass with null device") {
        dispatcher.EndPass(nullptr);
        CHECK(EterBase::ModernLogger::lastError == "UIPassDispatcher::EndPass - null device");
        CHECK(sm.restoreCalls.empty());
    }
}

