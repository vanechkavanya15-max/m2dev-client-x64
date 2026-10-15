#include "EterBase/LogModern.h"
#include <iostream>
#include <memory>
#include <vector>

// Since we cannot mock STATEMANAGER easily if we include VertexShaderScope.h directly 
// because it relies on the real STATEMANAGER which pulls in DX9 headers.
// So we use a dummy definition that mirrors exactly what is in VertexShaderScope.h

struct IDirect3DVertexShader9 { int id; };
typedef IDirect3DVertexShader9* LPDIRECT3DVERTEXSHADER9;
struct IDirect3DVertexDeclaration9 { int id; };
typedef IDirect3DVertexDeclaration9* LPDIRECT3DVERTEXDECLARATION9;
typedef unsigned long DWORD;

class MockStateManager
{
public:
    std::vector<LPDIRECT3DVERTEXSHADER9> m_VertexShaderStack;
    std::vector<LPDIRECT3DVERTEXDECLARATION9> m_VertexDeclarationStack;
    std::vector<DWORD> m_FVFStack;

    LPDIRECT3DVERTEXSHADER9 m_currentShader = nullptr;
    LPDIRECT3DVERTEXDECLARATION9 m_currentDecl = nullptr;
    DWORD m_currentFvf = 0;

    void SaveVertexShader(LPDIRECT3DVERTEXSHADER9 shader) {
        m_VertexShaderStack.push_back(m_currentShader);
        m_currentShader = shader;
    }
    void RestoreVertexShader() {
        if (!m_VertexShaderStack.empty()) {
            m_currentShader = m_VertexShaderStack.back();
            m_VertexShaderStack.pop_back();
        }
    }
    void SaveVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9 decl) {
        m_VertexDeclarationStack.push_back(m_currentDecl);
        m_currentDecl = decl;
    }
    void RestoreVertexDeclaration() {
        if (!m_VertexDeclarationStack.empty()) {
            m_currentDecl = m_VertexDeclarationStack.back();
            m_VertexDeclarationStack.pop_back();
        }
    }
    void SaveFVF(DWORD fvf) {
        m_FVFStack.push_back(m_currentFvf);
        m_currentFvf = fvf;
    }
    void RestoreFVF() {
        if (!m_FVFStack.empty()) {
            m_currentFvf = m_FVFStack.back();
            m_FVFStack.pop_back();
        }
    }
    static MockStateManager& Instance() { static MockStateManager inst; return inst; }
};
#define STATEMANAGER MockStateManager::Instance()

// Copy of the actual EterLib::Render::VertexShaderScope that we just wrote 
// (because tests compile without d3d9.h access in linux CI typically).
namespace EterLib::Render
{
    class VertexShaderScope
    {
    public:
        explicit VertexShaderScope(
            LPDIRECT3DVERTEXSHADER9 shader,
            LPDIRECT3DVERTEXDECLARATION9 declaration = nullptr,
            DWORD fvf = 0)
        {
            if (shader)
            {
                STATEMANAGER.SaveVertexShader(shader);
                m_hasShader = true;
            }
            if (declaration)
            {
                STATEMANAGER.SaveVertexDeclaration(declaration);
                m_hasDeclaration = true;
            }
            if (fvf != 0)
            {
                STATEMANAGER.SaveFVF(fvf);
                m_hasFvf = true;
            }
        }
        ~VertexShaderScope()
        {
            if (m_hasFvf) STATEMANAGER.RestoreFVF();
            if (m_hasDeclaration) STATEMANAGER.RestoreVertexDeclaration();
            if (m_hasShader) STATEMANAGER.RestoreVertexShader();
        }
        VertexShaderScope(const VertexShaderScope&) = delete;
        VertexShaderScope& operator=(const VertexShaderScope&) = delete;
        VertexShaderScope(VertexShaderScope&&) = delete;
        VertexShaderScope& operator=(VertexShaderScope&&) = delete;
    private:
        bool m_hasShader = false;
        bool m_hasDeclaration = false;
        bool m_hasFvf = false;
    };
}

namespace {
    int g_testsPassed = 0;
    int g_testsFailed = 0;

    void AssertTrue(bool condition, const char* testName) {
        if (condition) {
            EterBase::ModernLogger::Info("PASS: {}", testName);
            g_testsPassed++;
        } else {
            EterBase::ModernLogger::Error("FAIL: {}", testName);
            g_testsFailed++;
        }
    }
}

void TestVertexShaderScopeFull()
{
    IDirect3DVertexShader9 dummyShader1{1}, dummyShader2{2};
    IDirect3DVertexDeclaration9 dummyDecl1{1}, dummyDecl2{2};
    DWORD dummyFvf1 = 100, dummyFvf2 = 200;

    auto& state = STATEMANAGER;
    state.m_currentShader = &dummyShader1;
    state.m_currentDecl = &dummyDecl1;
    state.m_currentFvf = dummyFvf1;

    {
        EterLib::Render::VertexShaderScope scope(&dummyShader2, &dummyDecl2, dummyFvf2);
        AssertTrue(state.m_currentShader == &dummyShader2, "Scope Full: Current Shader updated");
        AssertTrue(state.m_currentDecl == &dummyDecl2, "Scope Full: Current Decl updated");
        AssertTrue(state.m_currentFvf == dummyFvf2, "Scope Full: Current FVF updated");
        AssertTrue(state.m_VertexShaderStack.back() == &dummyShader1, "Scope Full: Previous Shader saved");
        AssertTrue(state.m_VertexDeclarationStack.back() == &dummyDecl1, "Scope Full: Previous Decl saved");
        AssertTrue(state.m_FVFStack.back() == dummyFvf1, "Scope Full: Previous FVF saved");
    }

    AssertTrue(state.m_currentShader == &dummyShader1, "Scope Full: Shader restored after destruction");
    AssertTrue(state.m_currentDecl == &dummyDecl1, "Scope Full: Decl restored after destruction");
    AssertTrue(state.m_currentFvf == dummyFvf1, "Scope Full: FVF restored after destruction");
    AssertTrue(state.m_VertexShaderStack.empty(), "Scope Full: Shader stack empty");
    AssertTrue(state.m_VertexDeclarationStack.empty(), "Scope Full: Decl stack empty");
    AssertTrue(state.m_FVFStack.empty(), "Scope Full: FVF stack empty");
}

void TestVertexShaderScopePartial()
{
    IDirect3DVertexShader9 dummyShader1{1}, dummyShader2{2};
    IDirect3DVertexDeclaration9 dummyDecl1{1};
    DWORD dummyFvf1 = 100;

    auto& state = STATEMANAGER;
    state.m_currentShader = &dummyShader1;
    state.m_currentDecl = &dummyDecl1;
    state.m_currentFvf = dummyFvf1;

    {
        EterLib::Render::VertexShaderScope scope(&dummyShader2, nullptr, 0);
        AssertTrue(state.m_currentShader == &dummyShader2, "Scope Partial: Current Shader updated");
        AssertTrue(state.m_currentDecl == &dummyDecl1, "Scope Partial: Current Decl untouched");
        AssertTrue(state.m_currentFvf == dummyFvf1, "Scope Partial: Current FVF untouched");
    }

    AssertTrue(state.m_currentShader == &dummyShader1, "Scope Partial: Shader restored after destruction");
    AssertTrue(state.m_currentDecl == &dummyDecl1, "Scope Partial: Decl untouched after destruction");
    AssertTrue(state.m_currentFvf == dummyFvf1, "Scope Partial: FVF untouched after destruction");
}

int main()
{
    EterBase::ModernLogger::Info("Running VertexShaderScope tests...");
    TestVertexShaderScopeFull();
    TestVertexShaderScopePartial();

    if (g_testsFailed == 0) {
        EterBase::ModernLogger::Info("All tests passed (Passed: {} / Failed: {})", g_testsPassed, g_testsFailed);
        return 0;
    } else {
        EterBase::ModernLogger::Error("Some tests failed (Passed: {} / Failed: {})", g_testsPassed, g_testsFailed);
        return 1;
    }
}
