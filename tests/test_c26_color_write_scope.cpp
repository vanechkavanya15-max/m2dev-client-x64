#include <iostream>
#include <cassert>
#include <expected>
#include <string>

// Minimal mock setup to bypass deep dependencies
#define _INC_STDAFX_H_
#define __CSTATEMANAGER_H

#include "../test_env/d3d9.h"
#include "../test_env/d3dx9.h"

namespace EterBase {
    template<class E> using VoidResult = std::expected<void, E>;
    
    class ModernLogger {
    public:
        template<typename... Args>
        static void Info(const char* fmt, Args&&... args) {
            std::cout << "[INFO] Test execution..." << std::endl;
        }
        template<typename... Args>
        static void Error(const char* fmt, Args&&... args) {
            std::cerr << "[ERROR] Test failed..." << std::endl;
        }
    };
}

namespace {
    DWORD g_lastSavedState = 0xFFFFFFFF;
    DWORD g_lastSavedValue = 0xFFFFFFFF;
    DWORD g_lastRestoredState = 0xFFFFFFFF;
    DWORD g_renderStateValue = 0x0F; 
}

class CStateManager
{
public:
    static CStateManager& Instance()
    {
        static CStateManager instance;
        return instance;
    }

    void SaveRenderState(D3DRENDERSTATETYPE type, DWORD value)
    {
        g_lastSavedState = type;
        g_lastSavedValue = value;
        g_renderStateValue = value;
    }

    void RestoreRenderState(D3DRENDERSTATETYPE type)
    {
        g_lastRestoredState = type;
        g_renderStateValue = 0x0F; 
    }
};

#define STATEMANAGER (CStateManager::Instance())

// Include the class under test
namespace EterLib::Render { class ColorWriteScope; } // forward decl to ensure namespace
// In order to avoid including StdAfx.h and StateManager.h which cause errors:
#include "../src/EterLib/Render/ColorWriteScope.h"

EterBase::VoidResult<std::string> TestZPrepassScope()
{
    g_lastSavedState = 0;
    g_lastSavedValue = 0;
    g_lastRestoredState = 0;
    
    {
        ColorWriteScope scope(0); // Z-Prepass
        
        if (g_lastSavedState != D3DRS_COLORWRITEENABLE) {
            return std::unexpected("SaveRenderState not called with D3DRS_COLORWRITEENABLE");
        }
        if (g_lastSavedValue != 0) {
            return std::unexpected("SaveRenderState not called with value 0");
        }
    }
    
    if (g_lastRestoredState != D3DRS_COLORWRITEENABLE) {
        return std::unexpected("RestoreRenderState not called with D3DRS_COLORWRITEENABLE");
    }
    
    return {};
}

EterBase::VoidResult<std::string> TestAlphaOnlyScope()
{
    g_lastSavedState = 0;
    g_lastSavedValue = 0;
    g_lastRestoredState = 0;
    
    {
        ColorWriteScope scope(D3DCOLORWRITEENABLE_ALPHA);
        
        if (g_lastSavedState != D3DRS_COLORWRITEENABLE) {
            return std::unexpected("SaveRenderState not called with D3DRS_COLORWRITEENABLE");
        }
        if (g_lastSavedValue != D3DCOLORWRITEENABLE_ALPHA) {
            return std::unexpected("SaveRenderState not called with D3DCOLORWRITEENABLE_ALPHA");
        }
    }
    
    if (g_lastRestoredState != D3DRS_COLORWRITEENABLE) {
        return std::unexpected("RestoreRenderState not called with D3DRS_COLORWRITEENABLE");
    }
    
    return {};
}

int main()
{
    EterBase::ModernLogger::Info("Running TestZPrepassScope");
    auto res1 = TestZPrepassScope();
    if (!res1) {
        EterBase::ModernLogger::Error("TestZPrepassScope failed: %s", res1.error().c_str());
        return 1;
    }

    EterBase::ModernLogger::Info("Running TestAlphaOnlyScope");
    auto res2 = TestAlphaOnlyScope();
    if (!res2) {
        EterBase::ModernLogger::Error("TestAlphaOnlyScope failed: %s", res2.error().c_str());
        return 1;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
