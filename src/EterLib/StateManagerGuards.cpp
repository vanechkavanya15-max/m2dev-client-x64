#include "StdAfx.h"
#include "StateManagerGuards.h"
#include "StateManager.h"

// ============================================================================
// ScopedD3DVertexDeclGuard
// ============================================================================
ScopedD3DVertexDeclGuard::ScopedD3DVertexDeclGuard(LPDIRECT3DVERTEXDECLARATION9 pNewDecl, bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_pExitDecl(nullptr), m_bActive(true)
{
    STATEMANAGER.GetVertexDeclaration(&m_pSavedDecl);
    if (pNewDecl != nullptr || !bRestoreOnExit)
    {
        STATEMANAGER.SetVertexDeclaration(pNewDecl);
    }
}

ScopedD3DVertexDeclGuard::~ScopedD3DVertexDeclGuard() noexcept
{
    if (m_bActive)
    {
        if (m_bRestoreOnExit)
        {
            STATEMANAGER.SetVertexDeclaration(m_pSavedDecl);
        }
        else
        {
            STATEMANAGER.SetVertexDeclaration(m_pExitDecl);
        }
    }
}

void ScopedD3DVertexDeclGuard::Set(LPDIRECT3DVERTEXDECLARATION9 pNewDecl)
{
    STATEMANAGER.SetVertexDeclaration(pNewDecl);
}

void ScopedD3DVertexDeclGuard::Clear()
{
    STATEMANAGER.SetVertexDeclaration(nullptr);
}

void ScopedD3DVertexDeclGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DVertexDeclGuard::ScopedD3DVertexDeclGuard(ScopedD3DVertexDeclGuard&& other) noexcept
    : m_pSavedDecl(other.m_pSavedDecl),
      m_pExitDecl(other.m_pExitDecl),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DVertexDeclGuard& ScopedD3DVertexDeclGuard::operator=(ScopedD3DVertexDeclGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            if (m_bRestoreOnExit)
                STATEMANAGER.SetVertexDeclaration(m_pSavedDecl);
            else
                STATEMANAGER.SetVertexDeclaration(m_pExitDecl);
        }
        m_pSavedDecl = other.m_pSavedDecl;
        m_pExitDecl = other.m_pExitDecl;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DShaderGuard
// ============================================================================
ScopedD3DShaderGuard::ScopedD3DShaderGuard(
    LPDIRECT3DVERTEXSHADER9 pNewVS,
    LPDIRECT3DPIXELSHADER9 pNewPS,
    bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_bActive(true)
{
    STATEMANAGER.GetVertexShader(&m_pSavedVS);
    STATEMANAGER.GetPixelShader(&m_pSavedPS);

    STATEMANAGER.SetVertexShader(pNewVS);
    STATEMANAGER.SetPixelShader(pNewPS);
}

ScopedD3DShaderGuard::~ScopedD3DShaderGuard() noexcept
{
    if (m_bActive)
    {
        if (m_bRestoreOnExit)
        {
            STATEMANAGER.SetVertexShader(m_pSavedVS);
            STATEMANAGER.SetPixelShader(m_pSavedPS);
        }
        else
        {
            STATEMANAGER.SetVertexShader(nullptr);
            STATEMANAGER.SetPixelShader(nullptr);
        }
    }
}

void ScopedD3DShaderGuard::SetVertexShader(LPDIRECT3DVERTEXSHADER9 pNewVS)
{
    STATEMANAGER.SetVertexShader(pNewVS);
}

void ScopedD3DShaderGuard::SetPixelShader(LPDIRECT3DPIXELSHADER9 pNewPS)
{
    STATEMANAGER.SetPixelShader(pNewPS);
}

void ScopedD3DShaderGuard::ClearShaders()
{
    STATEMANAGER.SetVertexShader(nullptr);
    STATEMANAGER.SetPixelShader(nullptr);
}

void ScopedD3DShaderGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DShaderGuard::ScopedD3DShaderGuard(ScopedD3DShaderGuard&& other) noexcept
    : m_pSavedVS(other.m_pSavedVS),
      m_pSavedPS(other.m_pSavedPS),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DShaderGuard& ScopedD3DShaderGuard::operator=(ScopedD3DShaderGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            if (m_bRestoreOnExit)
            {
                STATEMANAGER.SetVertexShader(m_pSavedVS);
                STATEMANAGER.SetPixelShader(m_pSavedPS);
            }
            else
            {
                STATEMANAGER.SetVertexShader(nullptr);
                STATEMANAGER.SetPixelShader(nullptr);
            }
        }
        m_pSavedVS = other.m_pSavedVS;
        m_pSavedPS = other.m_pSavedPS;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DVertexShaderGuard
// ============================================================================
ScopedD3DVertexShaderGuard::ScopedD3DVertexShaderGuard(LPDIRECT3DVERTEXSHADER9 pNewVS, bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_bActive(true)
{
    STATEMANAGER.GetVertexShader(&m_pSavedVS);
    STATEMANAGER.SetVertexShader(pNewVS);
}

ScopedD3DVertexShaderGuard::~ScopedD3DVertexShaderGuard() noexcept
{
    if (m_bActive)
    {
        if (m_bRestoreOnExit)
            STATEMANAGER.SetVertexShader(m_pSavedVS);
        else
            STATEMANAGER.SetVertexShader(nullptr);
    }
}

void ScopedD3DVertexShaderGuard::Set(LPDIRECT3DVERTEXSHADER9 pNewVS)
{
    STATEMANAGER.SetVertexShader(pNewVS);
}

void ScopedD3DVertexShaderGuard::Clear()
{
    STATEMANAGER.SetVertexShader(nullptr);
}

void ScopedD3DVertexShaderGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DVertexShaderGuard::ScopedD3DVertexShaderGuard(ScopedD3DVertexShaderGuard&& other) noexcept
    : m_pSavedVS(other.m_pSavedVS),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DVertexShaderGuard& ScopedD3DVertexShaderGuard::operator=(ScopedD3DVertexShaderGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            if (m_bRestoreOnExit)
                STATEMANAGER.SetVertexShader(m_pSavedVS);
            else
                STATEMANAGER.SetVertexShader(nullptr);
        }
        m_pSavedVS = other.m_pSavedVS;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DPixelShaderGuard
// ============================================================================
ScopedD3DPixelShaderGuard::ScopedD3DPixelShaderGuard(LPDIRECT3DPIXELSHADER9 pNewPS, bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_bActive(true)
{
    STATEMANAGER.GetPixelShader(&m_pSavedPS);
    STATEMANAGER.SetPixelShader(pNewPS);
}

ScopedD3DPixelShaderGuard::~ScopedD3DPixelShaderGuard() noexcept
{
    if (m_bActive)
    {
        if (m_bRestoreOnExit)
            STATEMANAGER.SetPixelShader(m_pSavedPS);
        else
            STATEMANAGER.SetPixelShader(nullptr);
    }
}

void ScopedD3DPixelShaderGuard::Set(LPDIRECT3DPIXELSHADER9 pNewPS)
{
    STATEMANAGER.SetPixelShader(pNewPS);
}

void ScopedD3DPixelShaderGuard::Clear()
{
    STATEMANAGER.SetPixelShader(nullptr);
}

void ScopedD3DPixelShaderGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DPixelShaderGuard::ScopedD3DPixelShaderGuard(ScopedD3DPixelShaderGuard&& other) noexcept
    : m_pSavedPS(other.m_pSavedPS),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DPixelShaderGuard& ScopedD3DPixelShaderGuard::operator=(ScopedD3DPixelShaderGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            if (m_bRestoreOnExit)
                STATEMANAGER.SetPixelShader(m_pSavedPS);
            else
                STATEMANAGER.SetPixelShader(nullptr);
        }
        m_pSavedPS = other.m_pSavedPS;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DFVFGuard
// ============================================================================
ScopedD3DFVFGuard::ScopedD3DFVFGuard(DWORD dwNewFVF, bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_bActive(true)
{
    STATEMANAGER.GetFVF(&m_dwSavedFVF);
    if (dwNewFVF != 0)
    {
        STATEMANAGER.SetFVF(dwNewFVF);
    }
}

ScopedD3DFVFGuard::~ScopedD3DFVFGuard() noexcept
{
    if (m_bActive && m_bRestoreOnExit && m_dwSavedFVF != 0)
    {
        STATEMANAGER.SetFVF(m_dwSavedFVF);
    }
}

void ScopedD3DFVFGuard::Set(DWORD dwNewFVF)
{
    STATEMANAGER.SetFVF(dwNewFVF);
}

void ScopedD3DFVFGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DFVFGuard::ScopedD3DFVFGuard(ScopedD3DFVFGuard&& other) noexcept
    : m_dwSavedFVF(other.m_dwSavedFVF),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DFVFGuard& ScopedD3DFVFGuard::operator=(ScopedD3DFVFGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive && m_bRestoreOnExit && m_dwSavedFVF != 0)
        {
            STATEMANAGER.SetFVF(m_dwSavedFVF);
        }
        m_dwSavedFVF = other.m_dwSavedFVF;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DFFPGuard
// ============================================================================
ScopedD3DFFPGuard::ScopedD3DFFPGuard(DWORD dwTargetFVF, bool bRestoreOnExit)
    : m_bRestoreOnExit(bRestoreOnExit), m_bActive(true)
{
    STATEMANAGER.GetVertexDeclaration(&m_pSavedDecl);
    STATEMANAGER.GetVertexShader(&m_pSavedVS);
    STATEMANAGER.GetPixelShader(&m_pSavedPS);
    STATEMANAGER.GetFVF(&m_dwSavedFVF);

    EnsureCleanState();

    if (dwTargetFVF != 0)
    {
        STATEMANAGER.SetFVF(dwTargetFVF);
    }
}

void ScopedD3DFFPGuard::EnsureCleanState()
{
    STATEMANAGER.SetVertexShader(nullptr);
    STATEMANAGER.SetPixelShader(nullptr);
    STATEMANAGER.SetVertexDeclaration(nullptr);

    // Bezwzgledna gwarancja sprzetowa na urzadzeniu D3D9
    auto pDev = STATEMANAGER.GetDevice();
    if (pDev)
    {
        pDev->SetVertexDeclaration(nullptr);
        pDev->SetVertexShader(nullptr);
        pDev->SetPixelShader(nullptr);
    }
}

ScopedD3DFFPGuard::~ScopedD3DFFPGuard() noexcept
{
    if (m_bActive)
    {
        if (m_bRestoreOnExit)
        {
            STATEMANAGER.SetVertexDeclaration(m_pSavedDecl);
            STATEMANAGER.SetVertexShader(m_pSavedVS);
            STATEMANAGER.SetPixelShader(m_pSavedPS);
            if (m_dwSavedFVF != 0)
            {
                STATEMANAGER.SetFVF(m_dwSavedFVF);
            }
        }
        else
        {
            EnsureCleanState();
        }
    }
}

void ScopedD3DFFPGuard::SetFVF(DWORD dwTargetFVF)
{
    STATEMANAGER.SetFVF(dwTargetFVF);
}

void ScopedD3DFFPGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DFFPGuard::ScopedD3DFFPGuard(ScopedD3DFFPGuard&& other) noexcept
    : m_pSavedDecl(other.m_pSavedDecl),
      m_pSavedVS(other.m_pSavedVS),
      m_pSavedPS(other.m_pSavedPS),
      m_dwSavedFVF(other.m_dwSavedFVF),
      m_bRestoreOnExit(other.m_bRestoreOnExit),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DFFPGuard& ScopedD3DFFPGuard::operator=(ScopedD3DFFPGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            if (m_bRestoreOnExit)
            {
                STATEMANAGER.SetVertexDeclaration(m_pSavedDecl);
                STATEMANAGER.SetVertexShader(m_pSavedVS);
                STATEMANAGER.SetPixelShader(m_pSavedPS);
                if (m_dwSavedFVF != 0)
                    STATEMANAGER.SetFVF(m_dwSavedFVF);
            }
            else
            {
                EnsureCleanState();
            }
        }
        m_pSavedDecl = other.m_pSavedDecl;
        m_pSavedVS = other.m_pSavedVS;
        m_pSavedPS = other.m_pSavedPS;
        m_dwSavedFVF = other.m_dwSavedFVF;
        m_bRestoreOnExit = other.m_bRestoreOnExit;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DRenderStateGuard
// ============================================================================
ScopedD3DRenderStateGuard::ScopedD3DRenderStateGuard(D3DRENDERSTATETYPE state, DWORD dwNewValue)
    : m_bActive(true)
{
    AddAndSet(state, dwNewValue);
}

ScopedD3DRenderStateGuard::ScopedD3DRenderStateGuard(D3DRENDERSTATETYPE state)
    : m_bActive(true)
{
    Add(state);
}

ScopedD3DRenderStateGuard::ScopedD3DRenderStateGuard(std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>> states)
    : m_bActive(true)
{
    m_savedStates.reserve(states.size());
    for (const auto& entry : states)
    {
        AddAndSet(entry.first, entry.second);
    }
}

ScopedD3DRenderStateGuard::~ScopedD3DRenderStateGuard() noexcept
{
    if (m_bActive)
    {
        for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
        {
            STATEMANAGER.SetRenderState(it->first, it->second);
        }
    }
}

void ScopedD3DRenderStateGuard::AddAndSet(D3DRENDERSTATETYPE state, DWORD dwNewValue)
{
    DWORD dwOldVal = 0;
    STATEMANAGER.GetRenderState(state, &dwOldVal);
    m_savedStates.emplace_back(state, dwOldVal);
    STATEMANAGER.SetRenderState(state, dwNewValue);
}

void ScopedD3DRenderStateGuard::Add(D3DRENDERSTATETYPE state)
{
    DWORD dwOldVal = 0;
    STATEMANAGER.GetRenderState(state, &dwOldVal);
    m_savedStates.emplace_back(state, dwOldVal);
}

void ScopedD3DRenderStateGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DRenderStateGuard::ScopedD3DRenderStateGuard(ScopedD3DRenderStateGuard&& other) noexcept
    : m_savedStates(std::move(other.m_savedStates)),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DRenderStateGuard& ScopedD3DRenderStateGuard::operator=(ScopedD3DRenderStateGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
                STATEMANAGER.SetRenderState(it->first, it->second);
        }
        m_savedStates = std::move(other.m_savedStates);
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DTextureStageStateGuard
// ============================================================================
ScopedD3DTextureStageStateGuard::ScopedD3DTextureStageStateGuard(DWORD dwStage, D3DTEXTURESTAGESTATETYPE type, DWORD dwNewValue)
    : m_bActive(true)
{
    AddAndSet(dwStage, type, dwNewValue);
}

ScopedD3DTextureStageStateGuard::ScopedD3DTextureStageStateGuard(std::initializer_list<std::tuple<DWORD, D3DTEXTURESTAGESTATETYPE, DWORD>> states)
    : m_bActive(true)
{
    m_savedStates.reserve(states.size());
    for (const auto& entry : states)
    {
        AddAndSet(std::get<0>(entry), std::get<1>(entry), std::get<2>(entry));
    }
}

ScopedD3DTextureStageStateGuard::~ScopedD3DTextureStageStateGuard() noexcept
{
    if (m_bActive)
    {
        for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
        {
            STATEMANAGER.SetTextureStageState(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
        }
    }
}

void ScopedD3DTextureStageStateGuard::AddAndSet(DWORD dwStage, D3DTEXTURESTAGESTATETYPE type, DWORD dwNewValue)
{
    DWORD dwOldVal = 0;
    STATEMANAGER.GetTextureStageState(dwStage, type, &dwOldVal);
    m_savedStates.emplace_back(dwStage, type, dwOldVal);
    STATEMANAGER.SetTextureStageState(dwStage, type, dwNewValue);
}

void ScopedD3DTextureStageStateGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DTextureStageStateGuard::ScopedD3DTextureStageStateGuard(ScopedD3DTextureStageStateGuard&& other) noexcept
    : m_savedStates(std::move(other.m_savedStates)),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DTextureStageStateGuard& ScopedD3DTextureStageStateGuard::operator=(ScopedD3DTextureStageStateGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
                STATEMANAGER.SetTextureStageState(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
        }
        m_savedStates = std::move(other.m_savedStates);
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DSamplerStateGuard
// ============================================================================
ScopedD3DSamplerStateGuard::ScopedD3DSamplerStateGuard(DWORD dwStage, D3DSAMPLERSTATETYPE type, DWORD dwNewValue)
    : m_bActive(true)
{
    AddAndSet(dwStage, type, dwNewValue);
}

ScopedD3DSamplerStateGuard::ScopedD3DSamplerStateGuard(std::initializer_list<std::tuple<DWORD, D3DSAMPLERSTATETYPE, DWORD>> states)
    : m_bActive(true)
{
    m_savedStates.reserve(states.size());
    for (const auto& entry : states)
    {
        AddAndSet(std::get<0>(entry), std::get<1>(entry), std::get<2>(entry));
    }
}

ScopedD3DSamplerStateGuard::~ScopedD3DSamplerStateGuard() noexcept
{
    if (m_bActive)
    {
        for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
        {
            STATEMANAGER.SetSamplerState(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
        }
    }
}

void ScopedD3DSamplerStateGuard::AddAndSet(DWORD dwStage, D3DSAMPLERSTATETYPE type, DWORD dwNewValue)
{
    DWORD dwOldVal = 0;
    STATEMANAGER.GetSamplerState(dwStage, type, &dwOldVal);
    m_savedStates.emplace_back(dwStage, type, dwOldVal);
    STATEMANAGER.SetSamplerState(dwStage, type, dwNewValue);
}

void ScopedD3DSamplerStateGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DSamplerStateGuard::ScopedD3DSamplerStateGuard(ScopedD3DSamplerStateGuard&& other) noexcept
    : m_savedStates(std::move(other.m_savedStates)),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DSamplerStateGuard& ScopedD3DSamplerStateGuard::operator=(ScopedD3DSamplerStateGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
        {
            for (auto it = m_savedStates.rbegin(); it != m_savedStates.rend(); ++it)
                STATEMANAGER.SetSamplerState(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
        }
        m_savedStates = std::move(other.m_savedStates);
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DTextureGuard
// ============================================================================
ScopedD3DTextureGuard::ScopedD3DTextureGuard(DWORD dwStage, LPDIRECT3DBASETEXTURE9 pNewTexture)
    : m_dwStage(dwStage), m_bActive(true)
{
    STATEMANAGER.GetTexture(dwStage, &m_pSavedTexture);
    STATEMANAGER.SetTexture(dwStage, pNewTexture);
}

ScopedD3DTextureGuard::~ScopedD3DTextureGuard() noexcept
{
    if (m_bActive)
    {
        STATEMANAGER.SetTexture(m_dwStage, m_pSavedTexture);
    }
}

void ScopedD3DTextureGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DTextureGuard::ScopedD3DTextureGuard(ScopedD3DTextureGuard&& other) noexcept
    : m_dwStage(other.m_dwStage),
      m_pSavedTexture(other.m_pSavedTexture),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DTextureGuard& ScopedD3DTextureGuard::operator=(ScopedD3DTextureGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
            STATEMANAGER.SetTexture(m_dwStage, m_pSavedTexture);
        m_dwStage = other.m_dwStage;
        m_pSavedTexture = other.m_pSavedTexture;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}

// ============================================================================
// ScopedD3DTransformGuard
// ============================================================================
ScopedD3DTransformGuard::ScopedD3DTransformGuard(D3DTRANSFORMSTATETYPE type, const D3DXMATRIX* pNewMatrix)
    : m_type(type), m_bActive(true)
{
    STATEMANAGER.GetTransform(type, &m_savedMatrix);
    if (pNewMatrix)
    {
        STATEMANAGER.SetTransform(type, pNewMatrix);
    }
}

ScopedD3DTransformGuard::~ScopedD3DTransformGuard() noexcept
{
    if (m_bActive)
    {
        STATEMANAGER.SetTransform(m_type, &m_savedMatrix);
    }
}

void ScopedD3DTransformGuard::Dismiss() noexcept
{
    m_bActive = false;
}

ScopedD3DTransformGuard::ScopedD3DTransformGuard(ScopedD3DTransformGuard&& other) noexcept
    : m_type(other.m_type),
      m_savedMatrix(other.m_savedMatrix),
      m_bActive(other.m_bActive)
{
    other.m_bActive = false;
}

ScopedD3DTransformGuard& ScopedD3DTransformGuard::operator=(ScopedD3DTransformGuard&& other) noexcept
{
    if (this != &other)
    {
        if (m_bActive)
            STATEMANAGER.SetTransform(m_type, &m_savedMatrix);
        m_type = other.m_type;
        m_savedMatrix = other.m_savedMatrix;
        m_bActive = other.m_bActive;
        other.m_bActive = false;
    }
    return *this;
}
