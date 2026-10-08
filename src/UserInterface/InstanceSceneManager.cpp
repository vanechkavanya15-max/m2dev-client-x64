#include "StdAfx.h"
#include "InstanceSceneManager.h"
#include "EterLib/Camera.h"
#include "EterLib/StateManager.h"

namespace UserInterface {

namespace {

struct LessCharacterInstancePtrRenderOrder
{
    D3DXVECTOR3 v3CameraPosition;

    bool operator() (CInstanceBase* pkLeft, CInstanceBase* pkRight) const
    {
        if (!pkLeft || !pkRight)
            return pkLeft < pkRight;

        D3DXVECTOR3 v3Left, v3Right;
        pkLeft->NEW_GetPixelPosition(&v3Left);
        pkRight->NEW_GetPixelPosition(&v3Right);

        v3Left.y *= -1;
        v3Right.y *= -1;

        D3DXVECTOR3 v3LeftDiff = v3Left - v3CameraPosition;
        D3DXVECTOR3 v3RightDiff = v3Right - v3CameraPosition;

        return D3DXVec3Dot(&v3LeftDiff, &v3LeftDiff) < D3DXVec3Dot(&v3RightDiff, &v3RightDiff);
    }
};

} // anonymous namespace

void InstanceSceneManager::AddAliveInstance(CInstanceBase* pkInst)
{
    if (!pkInst)
        return;

    auto it = std::find(m_kVct_pkInstAlive.begin(), m_kVct_pkInstAlive.end(), pkInst);
    if (it == m_kVct_pkInstAlive.end())
    {
        m_kVct_pkInstAlive.push_back(pkInst);
    }
}

void InstanceSceneManager::RemoveAliveInstance(CInstanceBase* pkInst)
{
    if (!pkInst)
        return;

    auto it = std::find(m_kVct_pkInstAlive.begin(), m_kVct_pkInstAlive.end(), pkInst);
    if (it != m_kVct_pkInstAlive.end())
    {
        m_kVct_pkInstAlive.erase(it);
    }
}

void InstanceSceneManager::AddDeadInstance(CInstanceBase* pkInst)
{
    if (!pkInst)
        return;

    m_kVct_pkInstDead.push_back(pkInst);
}

void InstanceSceneManager::RemoveDeadInstance(CInstanceBase* pkInst)
{
    if (!pkInst)
        return;

    auto it = std::find(m_kVct_pkInstDead.begin(), m_kVct_pkInstDead.end(), pkInst);
    if (it != m_kVct_pkInstDead.end())
    {
        m_kVct_pkInstDead.erase(it);
    }
}

void InstanceSceneManager::MoveToDead(CInstanceBase* pkInst)
{
    if (!pkInst)
        return;

    RemoveAliveInstance(pkInst);
    AddDeadInstance(pkInst);
}

void InstanceSceneManager::Clear()
{
    ClearAlive();
    ClearDead();
}

void InstanceSceneManager::ClearAlive()
{
    m_kVct_pkInstAlive.clear();
}

void InstanceSceneManager::ClearDead()
{
    for (auto* pkInst : m_kVct_pkInstDead)
    {
        if (pkInst)
        {
            CInstanceBase::Delete(pkInst);
        }
    }
    m_kVct_pkInstDead.clear();
}

void InstanceSceneManager::SortAliveInstances(const D3DXVECTOR3& cameraEye)
{
    LessCharacterInstancePtrRenderOrder sortFunc{ cameraEye };
    std::sort(m_kVct_pkInstAlive.begin(), m_kVct_pkInstAlive.end(), sortFunc);
}

void InstanceSceneManager::SortDeadInstances(const D3DXVECTOR3& cameraEye)
{
    LessCharacterInstancePtrRenderOrder sortFunc{ cameraEye };
    std::sort(m_kVct_pkInstDead.begin(), m_kVct_pkInstDead.end(), sortFunc);
}

void InstanceSceneManager::RenderSortedAliveActorList()
{
    CCamera* pCamera = CCameraManager::instance().GetCurrentCamera();
    if (!pCamera) [[unlikely]]
        return;

    SortAliveInstances(pCamera->GetEye());

    for (auto* pkInst : m_kVct_pkInstAlive)
    {
        if (pkInst)
        {
            pkInst->Render();
            pkInst->RenderTrace();
        }
    }
}

void InstanceSceneManager::RenderSortedDeadActorList()
{
    CCamera* pCamera = CCameraManager::instance().GetCurrentCamera();
    if (!pCamera) [[unlikely]]
        return;

    SortDeadInstances(pCamera->GetEye());

    for (auto* pkInst : m_kVct_pkInstDead)
    {
        if (pkInst)
        {
            pkInst->Render();
        }
    }
}

void InstanceSceneManager::Render()
{
    STATEMANAGER.SetTexture(0, NULL);
    STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);

    STATEMANAGER.SetTexture(1, NULL);
    STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RenderSortedAliveActorList();
    RenderSortedDeadActorList();
}

void InstanceSceneManager::RenderShadowMainInstance(CInstanceBase* pkInstMain)
{
    if (pkInstMain)
    {
        pkInstMain->RenderToShadowMap();
    }
}

void InstanceSceneManager::RenderShadowAllInstances()
{
    for (auto* pkInst : m_kVct_pkInstAlive)
    {
        if (pkInst)
        {
            pkInst->RenderToShadowMap();
        }
    }
}

void InstanceSceneManager::RenderCollision()
{
    for (auto* pkInst : m_kVct_pkInstAlive)
    {
        if (pkInst)
        {
            pkInst->RenderCollision();
        }
    }
}

void InstanceSceneManager::Deform()
{
    for (auto* pkInst : m_kVct_pkInstAlive)
    {
        if (pkInst)
        {
            pkInst->Deform();
        }
    }

    for (auto* pkInst : m_kVct_pkInstDead)
    {
        if (pkInst)
        {
            pkInst->Deform();
        }
    }
}

void InstanceSceneManager::UpdateDeleting()
{
    for (auto itor = m_kVct_pkInstDead.begin(); itor != m_kVct_pkInstDead.end(); )
    {
        CInstanceBase* pInstance = *itor;

        if (pInstance && pInstance->UpdateDeleting()) [[likely]]
        {
            ++itor;
        }
        else [[unlikely]]
        {
            if (pInstance)
            {
                CInstanceBase::Delete(pInstance);
            }
            itor = m_kVct_pkInstDead.erase(itor);
        }
    }
}

bool InstanceSceneManager::IsDead(DWORD dwVID) const
{
    for (CInstanceBase* pkInst : m_kVct_pkInstDead)
    {
        if (pkInst && pkInst->GetVirtualID() == dwVID)
            return true;
    }
    return false;
}

bool InstanceSceneManager::IsAlive(DWORD dwVID) const
{
    for (CInstanceBase* pkInst : m_kVct_pkInstAlive)
    {
        if (pkInst && pkInst->GetVirtualID() == dwVID)
            return true;
    }
    return false;
}

} // namespace UserInterface
