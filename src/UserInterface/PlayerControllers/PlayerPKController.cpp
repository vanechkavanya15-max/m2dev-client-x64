#include "../StdAfx.h"
#include "PlayerPKController.h"
#include "Client/Network/Protocol/Protocol.h"

#include "../InstanceBase.h"

namespace UserInterface::PlayerControllers
{
    PlayerPKController::PlayerPKController(
        Contracts::IActorProvider* pActorProvider,
        Contracts::INetworkService* pNetworkService)
        : m_pActorProvider(pActorProvider)
        , m_pNetworkService(pNetworkService)
    {
    }

    void PlayerPKController::RememberChallengeInstance(DWORD dwVID)
    {
        m_RevengeInstanceSet.erase(dwVID);
        m_ChallengeInstanceSet.insert(dwVID);
    }

    void PlayerPKController::RememberRevengeInstance(DWORD dwVID)
    {
        m_ChallengeInstanceSet.erase(dwVID);
        m_RevengeInstanceSet.insert(dwVID);
    }

    void PlayerPKController::RememberCantFightInstance(DWORD dwVID)
    {
        m_CantFightInstanceSet.insert(dwVID);
    }

    void PlayerPKController::ForgetInstance(DWORD dwVID)
    {
        m_ChallengeInstanceSet.erase(dwVID);
        m_RevengeInstanceSet.erase(dwVID);
        m_CantFightInstanceSet.erase(dwVID);
    }

    void PlayerPKController::Clear()
    {
        m_ChallengeInstanceSet.clear();
        m_RevengeInstanceSet.clear();
        m_CantFightInstanceSet.clear();
    }

    bool PlayerPKController::IsChallengeInstance(DWORD dwVID) const
    {
        return m_ChallengeInstanceSet.find(dwVID) != m_ChallengeInstanceSet.end();
    }

    bool PlayerPKController::IsRevengeInstance(DWORD dwVID) const
    {
        return m_RevengeInstanceSet.find(dwVID) != m_RevengeInstanceSet.end();
    }

    bool PlayerPKController::IsCantFightInstance(DWORD dwVID) const
    {
        return m_CantFightInstanceSet.find(dwVID) != m_CantFightInstanceSet.end();
    }

    bool PlayerPKController::CanAttack(CInstanceBase* pkInstMain, CInstanceBase* pkInstVictim) const
    {
        if (!pkInstMain || !pkInstVictim)
            return false;

        if (pkInstMain->GetVirtualID() == pkInstVictim->GetVirtualID())
            return false;

        if (IsCantFightInstance(pkInstVictim->GetVirtualID()))
            return false;

        if (pkInstMain->IsStone())
        {
            if (pkInstVictim->IsPC())
                return true;
        }
        else if (pkInstMain->IsPC())
        {
            if (pkInstVictim->IsStone())
                return true;

            if (pkInstVictim->IsPC())
            {
                if (pkInstMain->GetDuelMode())
                {
                    switch (pkInstMain->GetDuelMode())
                    {
                        case CInstanceBase::DUEL_CANNOTATTACK:
                            return false;

                        case CInstanceBase::DUEL_START:
                            if (pkInstMain->IsPVPInstance(*pkInstVictim))
                                return true;
                            else
                                return false;

                        default:
                            break;
                    }
                }

                if (PK_MODE_GUILD == pkInstMain->GetPKMode())
                {
                    if (pkInstMain->GetGuildID() == pkInstVictim->GetGuildID())
                        return false;
                }

                if (pkInstVictim->IsKiller())
                {
                    bool isParty = m_pActorProvider ? m_pActorProvider->IsSamePartyMember(pkInstMain->GetVirtualID(), pkInstVictim->GetVirtualID()) : false;
                    if (!isParty)
                        return true;
                }

                if (PK_MODE_PROTECT != pkInstMain->GetPKMode())
                {
                    if (PK_MODE_FREE == pkInstMain->GetPKMode())
                    {
                        if (PK_MODE_PROTECT != pkInstVictim->GetPKMode())
                        {
                            bool isParty = m_pActorProvider ? m_pActorProvider->IsSamePartyMember(pkInstMain->GetVirtualID(), pkInstVictim->GetVirtualID()) : false;
                            if (!isParty)
                                return true;
                        }
                    }

                    if (PK_MODE_GUILD == pkInstMain->GetPKMode())
                    {
                        if (PK_MODE_PROTECT != pkInstVictim->GetPKMode())
                        {
                            bool isParty = m_pActorProvider ? m_pActorProvider->IsSamePartyMember(pkInstMain->GetVirtualID(), pkInstVictim->GetVirtualID()) : false;
                            if (!isParty)
                            {
                                if (pkInstMain->GetGuildID() != pkInstVictim->GetGuildID())
                                    return true;
                            }
                        }
                    }
                }

                if (pkInstMain->IsSameEmpire(*pkInstVictim))
                {
                    if (IsChallengeInstance(pkInstVictim->GetVirtualID()) ||
                        IsRevengeInstance(pkInstVictim->GetVirtualID()) ||
                        pkInstMain->IsPVPInstance(*pkInstVictim))
                    {
                        return true;
                    }

                    if (PK_MODE_REVENGE == pkInstMain->GetPKMode())
                    {
                        bool isParty = m_pActorProvider ? m_pActorProvider->IsSamePartyMember(pkInstMain->GetVirtualID(), pkInstVictim->GetVirtualID()) : false;
                        if (!isParty)
                        {
                            if ((pkInstMain->GetGuildID() == 0 || pkInstMain->GetGuildID() != pkInstVictim->GetGuildID()) &&
                                pkInstMain->IsConflictAlignmentInstance(*pkInstVictim) &&
                                pkInstVictim->GetAlignment() < 0)
                            {
                                return true;
                            }
                        }
                    }
                }
                else
                {
                    return true;
                }
            }

            if (pkInstVictim->IsEnemy())
                return true;

            if (pkInstVictim->IsWoodenDoor())
                return true;
        }
        else if (pkInstMain->IsEnemy())
        {
            if (pkInstVictim->IsPC() || pkInstVictim->IsBuilding())
                return true;
        }
        else if (pkInstMain->IsPoly())
        {
            if (pkInstVictim->IsPC() || pkInstVictim->IsEnemy())
                return true;
        }

        return false;
    }
}
