#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <utility>
#include <tuple>
#include <initializer_list>

class CStateManager;

/**
 * @brief Straznik RAII dla deklaracji wierzcholkow Direct3D 9 (IDirect3DVertexDeclaration9).
 * Zapobiega wyciekom deklaracji wierzcholkow do faz renderowania opartych na FVF.
 */
class ScopedD3DVertexDeclGuard
{
public:
    explicit ScopedD3DVertexDeclGuard(LPDIRECT3DVERTEXDECLARATION9 pNewDecl = nullptr, bool bRestoreOnExit = true);
    ~ScopedD3DVertexDeclGuard() noexcept;

    void Set(LPDIRECT3DVERTEXDECLARATION9 pNewDecl);
    void Clear();
    void Dismiss() noexcept;

    LPDIRECT3DVERTEXDECLARATION9 GetSavedDecl() const noexcept { return m_pSavedDecl; }
    void SetRestoreOnExit(bool bRestore) noexcept { m_bRestoreOnExit = bRestore; }
    void SetExitTarget(LPDIRECT3DVERTEXDECLARATION9 pExitDecl) noexcept { m_pExitDecl = pExitDecl; }

    ScopedD3DVertexDeclGuard(const ScopedD3DVertexDeclGuard&) = delete;
    ScopedD3DVertexDeclGuard& operator=(const ScopedD3DVertexDeclGuard&) = delete;
    ScopedD3DVertexDeclGuard(ScopedD3DVertexDeclGuard&& other) noexcept;
    ScopedD3DVertexDeclGuard& operator=(ScopedD3DVertexDeclGuard&& other) noexcept;

private:
    LPDIRECT3DVERTEXDECLARATION9 m_pSavedDecl = nullptr;
    LPDIRECT3DVERTEXDECLARATION9 m_pExitDecl = nullptr;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla shaderow Direct3D 9 (Vertex Shader i Pixel Shader).
 * Zapobiega pozostawaniu aktywnych shaderow przy powrocie do potoku Fixed-Function.
 */
class ScopedD3DShaderGuard
{
public:
    explicit ScopedD3DShaderGuard(
        LPDIRECT3DVERTEXSHADER9 pNewVS = nullptr,
        LPDIRECT3DPIXELSHADER9 pNewPS = nullptr,
        bool bRestoreOnExit = true);
    ~ScopedD3DShaderGuard() noexcept;

    void SetVertexShader(LPDIRECT3DVERTEXSHADER9 pNewVS);
    void SetPixelShader(LPDIRECT3DPIXELSHADER9 pNewPS);
    void ClearShaders();
    void Dismiss() noexcept;

    LPDIRECT3DVERTEXSHADER9 GetSavedVS() const noexcept { return m_pSavedVS; }
    LPDIRECT3DPIXELSHADER9 GetSavedPS() const noexcept { return m_pSavedPS; }
    void SetRestoreOnExit(bool bRestore) noexcept { m_bRestoreOnExit = bRestore; }

    ScopedD3DShaderGuard(const ScopedD3DShaderGuard&) = delete;
    ScopedD3DShaderGuard& operator=(const ScopedD3DShaderGuard&) = delete;
    ScopedD3DShaderGuard(ScopedD3DShaderGuard&& other) noexcept;
    ScopedD3DShaderGuard& operator=(ScopedD3DShaderGuard&& other) noexcept;

private:
    LPDIRECT3DVERTEXSHADER9 m_pSavedVS = nullptr;
    LPDIRECT3DPIXELSHADER9 m_pSavedPS = nullptr;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dedykowany wylacznie dla Vertex Shader (IDirect3DVertexShader9).
 */
class ScopedD3DVertexShaderGuard
{
public:
    explicit ScopedD3DVertexShaderGuard(LPDIRECT3DVERTEXSHADER9 pNewVS = nullptr, bool bRestoreOnExit = true);
    ~ScopedD3DVertexShaderGuard() noexcept;

    void Set(LPDIRECT3DVERTEXSHADER9 pNewVS);
    void Clear();
    void Dismiss() noexcept;

    LPDIRECT3DVERTEXSHADER9 GetSavedVS() const noexcept { return m_pSavedVS; }

    ScopedD3DVertexShaderGuard(const ScopedD3DVertexShaderGuard&) = delete;
    ScopedD3DVertexShaderGuard& operator=(const ScopedD3DVertexShaderGuard&) = delete;
    ScopedD3DVertexShaderGuard(ScopedD3DVertexShaderGuard&& other) noexcept;
    ScopedD3DVertexShaderGuard& operator=(ScopedD3DVertexShaderGuard&& other) noexcept;

private:
    LPDIRECT3DVERTEXSHADER9 m_pSavedVS = nullptr;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dedykowany wylacznie dla Pixel Shader (IDirect3DPixelShader9).
 */
class ScopedD3DPixelShaderGuard
{
public:
    explicit ScopedD3DPixelShaderGuard(LPDIRECT3DPIXELSHADER9 pNewPS = nullptr, bool bRestoreOnExit = true);
    ~ScopedD3DPixelShaderGuard() noexcept;

    void Set(LPDIRECT3DPIXELSHADER9 pNewPS);
    void Clear();
    void Dismiss() noexcept;

    LPDIRECT3DPIXELSHADER9 GetSavedPS() const noexcept { return m_pSavedPS; }

    ScopedD3DPixelShaderGuard(const ScopedD3DPixelShaderGuard&) = delete;
    ScopedD3DPixelShaderGuard& operator=(const ScopedD3DPixelShaderGuard&) = delete;
    ScopedD3DPixelShaderGuard(ScopedD3DPixelShaderGuard&& other) noexcept;
    ScopedD3DPixelShaderGuard& operator=(ScopedD3DPixelShaderGuard&& other) noexcept;

private:
    LPDIRECT3DPIXELSHADER9 m_pSavedPS = nullptr;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla Flexible Vertex Format (FVF).
 */
class ScopedD3DFVFGuard
{
public:
    explicit ScopedD3DFVFGuard(DWORD dwNewFVF = 0, bool bRestoreOnExit = true);
    ~ScopedD3DFVFGuard() noexcept;

    void Set(DWORD dwNewFVF);
    void Dismiss() noexcept;

    DWORD GetSavedFVF() const noexcept { return m_dwSavedFVF; }

    ScopedD3DFVFGuard(const ScopedD3DFVFGuard&) = delete;
    ScopedD3DFVFGuard& operator=(const ScopedD3DFVFGuard&) = delete;
    ScopedD3DFVFGuard(ScopedD3DFVFGuard&& other) noexcept;
    ScopedD3DFVFGuard& operator=(ScopedD3DFVFGuard&& other) noexcept;

private:
    DWORD m_dwSavedFVF = 0;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Glowny straznik RAII dla potoku Fixed-Function Pipeline (FFP).
 * Gwarantuje wyczyszczenie Vertex Declaration, Vertex Shader i Pixel Shader do NULL
 * oraz poprawne ustawienie FVF bez mozliwosci wycieku stanu miedzy fazami renderowania.
 */
class ScopedD3DFFPGuard
{
public:
    explicit ScopedD3DFFPGuard(DWORD dwTargetFVF = 0, bool bRestoreOnExit = true);
    ~ScopedD3DFFPGuard() noexcept;

    void SetFVF(DWORD dwTargetFVF);
    void EnsureCleanState();
    void Dismiss() noexcept;

    ScopedD3DFFPGuard(const ScopedD3DFFPGuard&) = delete;
    ScopedD3DFFPGuard& operator=(const ScopedD3DFFPGuard&) = delete;
    ScopedD3DFFPGuard(ScopedD3DFFPGuard&& other) noexcept;
    ScopedD3DFFPGuard& operator=(ScopedD3DFFPGuard&& other) noexcept;

private:
    LPDIRECT3DVERTEXDECLARATION9 m_pSavedDecl = nullptr;
    LPDIRECT3DVERTEXSHADER9 m_pSavedVS = nullptr;
    LPDIRECT3DPIXELSHADER9 m_pSavedPS = nullptr;
    DWORD m_dwSavedFVF = 0;
    bool m_bRestoreOnExit = true;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla stanow urzadzenia RenderState (D3DRENDERSTATETYPE).
 * Obsluguje zarowno pojedyncze stany, jak i listy stanow w std::initializer_list.
 */
class ScopedD3DRenderStateGuard
{
public:
    ScopedD3DRenderStateGuard(D3DRENDERSTATETYPE state, DWORD dwNewValue);
    explicit ScopedD3DRenderStateGuard(D3DRENDERSTATETYPE state);
    ScopedD3DRenderStateGuard(std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>> states);
    ~ScopedD3DRenderStateGuard() noexcept;

    void AddAndSet(D3DRENDERSTATETYPE state, DWORD dwNewValue);
    void Add(D3DRENDERSTATETYPE state);
    void Dismiss() noexcept;

    ScopedD3DRenderStateGuard(const ScopedD3DRenderStateGuard&) = delete;
    ScopedD3DRenderStateGuard& operator=(const ScopedD3DRenderStateGuard&) = delete;
    ScopedD3DRenderStateGuard(ScopedD3DRenderStateGuard&& other) noexcept;
    ScopedD3DRenderStateGuard& operator=(ScopedD3DRenderStateGuard&& other) noexcept;

private:
    std::vector<std::pair<D3DRENDERSTATETYPE, DWORD>> m_savedStates;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla TextureStageState (D3DTEXTURESTAGESTATETYPE).
 */
class ScopedD3DTextureStageStateGuard
{
public:
    ScopedD3DTextureStageStateGuard(DWORD dwStage, D3DTEXTURESTAGESTATETYPE type, DWORD dwNewValue);
    ScopedD3DTextureStageStateGuard(std::initializer_list<std::tuple<DWORD, D3DTEXTURESTAGESTATETYPE, DWORD>> states);
    ~ScopedD3DTextureStageStateGuard() noexcept;

    void AddAndSet(DWORD dwStage, D3DTEXTURESTAGESTATETYPE type, DWORD dwNewValue);
    void Dismiss() noexcept;

    ScopedD3DTextureStageStateGuard(const ScopedD3DTextureStageStateGuard&) = delete;
    ScopedD3DTextureStageStateGuard& operator=(const ScopedD3DTextureStageStateGuard&) = delete;
    ScopedD3DTextureStageStateGuard(ScopedD3DTextureStageStateGuard&& other) noexcept;
    ScopedD3DTextureStageStateGuard& operator=(ScopedD3DTextureStageStateGuard&& other) noexcept;

private:
    std::vector<std::tuple<DWORD, D3DTEXTURESTAGESTATETYPE, DWORD>> m_savedStates;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla SamplerState (D3DSAMPLERSTATETYPE).
 */
class ScopedD3DSamplerStateGuard
{
public:
    ScopedD3DSamplerStateGuard(DWORD dwStage, D3DSAMPLERSTATETYPE type, DWORD dwNewValue);
    ScopedD3DSamplerStateGuard(std::initializer_list<std::tuple<DWORD, D3DSAMPLERSTATETYPE, DWORD>> states);
    ~ScopedD3DSamplerStateGuard() noexcept;

    void AddAndSet(DWORD dwStage, D3DSAMPLERSTATETYPE type, DWORD dwNewValue);
    void Dismiss() noexcept;

    ScopedD3DSamplerStateGuard(const ScopedD3DSamplerStateGuard&) = delete;
    ScopedD3DSamplerStateGuard& operator=(const ScopedD3DSamplerStateGuard&) = delete;
    ScopedD3DSamplerStateGuard(ScopedD3DSamplerStateGuard&& other) noexcept;
    ScopedD3DSamplerStateGuard& operator=(ScopedD3DSamplerStateGuard&& other) noexcept;

private:
    std::vector<std::tuple<DWORD, D3DSAMPLERSTATETYPE, DWORD>> m_savedStates;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla biezacej tekstury (LPDIRECT3DBASETEXTURE9).
 */
class ScopedD3DTextureGuard
{
public:
    explicit ScopedD3DTextureGuard(DWORD dwStage, LPDIRECT3DBASETEXTURE9 pNewTexture = nullptr);
    ~ScopedD3DTextureGuard() noexcept;

    void Dismiss() noexcept;

    ScopedD3DTextureGuard(const ScopedD3DTextureGuard&) = delete;
    ScopedD3DTextureGuard& operator=(const ScopedD3DTextureGuard&) = delete;
    ScopedD3DTextureGuard(ScopedD3DTextureGuard&& other) noexcept;
    ScopedD3DTextureGuard& operator=(ScopedD3DTextureGuard&& other) noexcept;

private:
    DWORD m_dwStage = 0;
    LPDIRECT3DBASETEXTURE9 m_pSavedTexture = nullptr;
    bool m_bActive = true;
};

/**
 * @brief Straznik RAII dla macierzy transformacji (D3DTRANSFORMSTATETYPE).
 */
class ScopedD3DTransformGuard
{
public:
    ScopedD3DTransformGuard(D3DTRANSFORMSTATETYPE type, const D3DXMATRIX* pNewMatrix = nullptr);
    ~ScopedD3DTransformGuard() noexcept;

    void Dismiss() noexcept;

    ScopedD3DTransformGuard(const ScopedD3DTransformGuard&) = delete;
    ScopedD3DTransformGuard& operator=(const ScopedD3DTransformGuard&) = delete;
    ScopedD3DTransformGuard(ScopedD3DTransformGuard&& other) noexcept;
    ScopedD3DTransformGuard& operator=(ScopedD3DTransformGuard&& other) noexcept;

private:
    D3DTRANSFORMSTATETYPE m_type;
    D3DXMATRIX m_savedMatrix;
    bool m_bActive = true;
};
