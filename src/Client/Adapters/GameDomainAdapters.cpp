#include "StdAfx.h"
#include "GameDomainAdapters.h"
#include "Client/MCP/AgentMCPBridge.h"
#include "Client/Bridge/BotApi_FFI.h"
#include "UserInterface/PythonPlayer.h"
#include "UserInterface/PythonCharacterManager.h"
#include "UserInterface/PythonItem.h"
#include "UserInterface/PythonNetworkStream.h"
#include "GameLib/ItemManager.h"

namespace Client::Adapters
{
    // ========================================================================
    // PlayerDomainAdapter
    // ========================================================================
    uint32_t PlayerDomainAdapter::GetPlayerVID() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetMainCharacterIndex() : 0;
    }

    std::string_view PlayerDomainAdapter::GetPlayerName() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return "";
        const char* szName = p->GetName();
        return szName ? szName : "";
    }

    int PlayerDomainAdapter::GetPlayerLevel() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus(POINT_LEVEL) : 0;
    }

    int64_t PlayerDomainAdapter::GetCurrentHP() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus64(POINT_HP) : 0;
    }

    int64_t PlayerDomainAdapter::GetMaxHP() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus64(POINT_MAX_HP) : 0;
    }

    int64_t PlayerDomainAdapter::GetCurrentSP() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus64(POINT_SP) : 0;
    }

    int64_t PlayerDomainAdapter::GetMaxSP() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus64(POINT_MAX_SP) : 0;
    }

    int64_t PlayerDomainAdapter::GetYang() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetGold() : 0;
    }

    int64_t PlayerDomainAdapter::GetExp() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetEXP() : 0;
    }

    Client::Contracts::PlayerCoordinates PlayerDomainAdapter::GetCoordinates() const noexcept
    {
        Client::Contracts::PlayerCoordinates coords{};
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return coords;

        TPixelPosition kPos;
        p->NEW_GetMainActorPosition(&kPos);
        coords.x = kPos.x;
        coords.y = kPos.y;
        coords.z = kPos.z;

        CInstanceBase* pkMain = p->NEW_GetMainActorPtr();
        if (pkMain)
        {
            coords.rotation = pkMain->GetRotation();
        }
        return coords;
    }

    uint32_t PlayerDomainAdapter::GetTargetVID() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetTargetVID() : 0;
    }

    bool PlayerDomainAdapter::IsInCombat() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return false;

        CInstanceBase* pkMain = p->NEW_GetMainActorPtr();
        if (!pkMain)
            return false;
        return pkMain->IsAttacking() || p->GetTargetVID() != 0;
    }

    Client::Contracts::PlayerStatusSnapshot PlayerDomainAdapter::CaptureSnapshot() const
    {
        Client::Contracts::PlayerStatusSnapshot snapshot{};
        snapshot.vid = GetPlayerVID();
        snapshot.name = std::string(GetPlayerName());
        snapshot.level = GetPlayerLevel();
        snapshot.hp = GetCurrentHP();
        snapshot.maxHp = GetMaxHP();
        snapshot.sp = GetCurrentSP();
        snapshot.maxSp = GetMaxSP();
        snapshot.yang = GetYang();
        snapshot.exp = GetExp();
        snapshot.coords = GetCoordinates();
        snapshot.targetVid = GetTargetVID();
        snapshot.isCombatActive = IsInCombat();
        snapshot.mapName = "";
        return snapshot;
    }

    int64_t PlayerDomainAdapter::GetStatus(int statusType) const
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetStatus64(statusType) : 0;
    }

    bool PlayerDomainAdapter::RequestMove(float x, float y)
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return false;
        TPixelPosition targetPos{x, y, 0.0f};
        return p->NEW_MoveToDestPixelPositionDirection(targetPos);
    }

    // ========================================================================
    // InventoryDomainAdapter
    // ========================================================================
    std::optional<Client::Contracts::ItemDataSnapshot> InventoryDomainAdapter::GetItem(const Client::Contracts::ItemCellAddress& pos) const
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return std::nullopt;

        TItemPos cellPos(pos.windowType, pos.cellIndex);
        if (!cellPos.IsValidCell())
            return std::nullopt;

        const TItemData* pData = p->GetItemData(cellPos);
        if (!pData || pData->vnum == 0)
            return std::nullopt;

        Client::Contracts::ItemDataSnapshot item{};
        item.pos = pos;
        item.vnum = pData->vnum;
        item.count = pData->count;
        item.isEquipped = (pos.windowType == EQUIPMENT) || (pos.windowType == INVENTORY && cellPos.IsEquipCell());

        for (size_t i = 0; i < 6 && i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
        {
            item.sockets[i] = static_cast<uint32_t>(pData->alSockets[i]);
        }
        for (size_t i = 0; i < 7 && i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
        {
            item.attrTypes[i] = pData->aAttr[i].bType;
            item.attrValues[i] = pData->aAttr[i].sValue;
        }

        CItemData* pItemData = nullptr;
        if (CItemManager::InstancePtr() && CItemManager::Instance().GetItemDataPointer(pData->vnum, &pItemData) && pItemData)
        {
            item.localizedName = pItemData->GetName();
        }

        return item;
    }

    std::vector<Client::Contracts::ItemDataSnapshot> InventoryDomainAdapter::GetAllItems(uint8_t windowType) const
    {
        std::vector<Client::Contracts::ItemDataSnapshot> items;
        if (!CPythonPlayer::InstancePtr())
            return items;

        uint16_t maxSlots = 180;
        if (windowType == EQUIPMENT)
            maxSlots = 32;
        else if (windowType == BELT_INVENTORY)
            maxSlots = 16;
        else if (windowType == DRAGON_SOUL_INVENTORY)
            maxSlots = 192;

        for (uint16_t i = 0; i < maxSlots; ++i)
        {
            auto itemOpt = GetItem({windowType, i});
            if (itemOpt.has_value())
            {
                items.push_back(std::move(*itemOpt));
            }
        }
        return items;
    }

    bool InventoryDomainAdapter::IsCellEmpty(const Client::Contracts::ItemCellAddress& pos) const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return true;

        TItemPos cellPos(pos.windowType, pos.cellIndex);
        return p->IsItem(cellPos) == 0;
    }

    size_t InventoryDomainAdapter::GetFreeSlotCount(uint8_t windowType) const noexcept
    {
        if (!CPythonPlayer::InstancePtr())
            return 0;

        size_t freeSlots = 0;
        uint16_t maxSlots = (windowType == INVENTORY) ? 180 : 32;
        for (uint16_t i = 0; i < maxSlots; ++i)
        {
            if (IsCellEmpty({windowType, i}))
                freeSlots++;
        }
        return freeSlots;
    }

    bool InventoryDomainAdapter::RequestUseItem(const Client::Contracts::ItemCellAddress& pos)
    {
        TItemPos cellPos(pos.windowType, pos.cellIndex);
        if (!cellPos.IsValidCell())
            return false;

        auto* pNetStream = CPythonNetworkStream::InstancePtr();
        if (!pNetStream)
            return false;

        return pNetStream->SendItemUsePacket(cellPos);
    }

    bool InventoryDomainAdapter::RequestMoveItem(const Client::Contracts::ItemCellAddress& src, const Client::Contracts::ItemCellAddress& dst, uint32_t count)
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (!p)
            return false;

        TItemPos srcPos(src.windowType, src.cellIndex);
        TItemPos dstPos(dst.windowType, dst.cellIndex);
        return p->SendItemMovePacket(srcPos, dstPos, static_cast<BYTE>(count));
    }

    bool InventoryDomainAdapter::RequestDropItem(const Client::Contracts::ItemCellAddress& pos, uint32_t count)
    {
        TItemPos cellPos(pos.windowType, pos.cellIndex);
        auto* pNetStream = CPythonNetworkStream::InstancePtr();
        if (!pNetStream)
            return false;

        return pNetStream->SendItemDropPacketNew(cellPos, 0, count);
    }

    // ========================================================================
    // CombatDomainAdapter
    // ========================================================================
    Client::Contracts::CombatVerdict CombatDomainAdapter::EvaluateAttack(uint32_t targetVid) const
    {
        Client::Contracts::CombatVerdict verdict{};
        if (targetVid == 0)
        {
            verdict.canAttack = false;
            verdict.reason = "INVALID_TARGET_VID";
            return verdict;
        }

        auto* p = CPythonPlayer::InstancePtr();
        auto* pChrMgr = CPythonCharacterManager::InstancePtr();
        if (!p || !pChrMgr)
        {
            verdict.canAttack = false;
            verdict.reason = "NOT_IN_GAME";
            return verdict;
        }

        if (!p->CanAttackInstance(targetVid))
        {
            verdict.canAttack = false;
            verdict.reason = "TARGET_UNATTACKABLE";
            return verdict;
        }

        CInstanceBase* pkMain = p->NEW_GetMainActorPtr();
        CInstanceBase* pkTarget = pChrMgr->GetInstancePtr(targetVid);
        if (!pkMain || !pkTarget)
        {
            verdict.canAttack = false;
            verdict.reason = "ACTOR_NOT_FOUND";
            return verdict;
        }

        verdict.distanceToTarget = pkMain->NEW_GetDistanceFromDestInstance(*pkTarget);
        verdict.canAttack = true;
        verdict.remainingCooldown = 0.0f;
        verdict.hitChancePercent = 100;
        verdict.reason = "OK";
        return verdict;
    }

    float CombatDomainAdapter::GetAttackCooldown() const noexcept
    {
        return 0.0f;
    }

    uint32_t CombatDomainAdapter::GetLastTargetVID() const noexcept
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->GetTargetVID() : 0;
    }

    bool CombatDomainAdapter::ExecuteAttack(uint32_t targetVid)
    {
        auto* p = CPythonPlayer::InstancePtr();
        return p ? p->AttackTarget(targetVid) : false;
    }

    void CombatDomainAdapter::UpdateCombatState(float deltaTime)
    {
        auto* p = CPythonPlayer::InstancePtr();
        if (p)
        {
            p->UpdateAttackState(deltaTime);
        }
    }

    // ========================================================================
    // WorldQueryAdapter
    // ========================================================================
    std::vector<Client::Contracts::EntitySnapshot> WorldQueryAdapter::GetEntitiesInRadius(float radius) const
    {
        std::vector<Client::Contracts::EntitySnapshot> entities;
        auto* p = CPythonPlayer::InstancePtr();
        auto* pChrMgr = CPythonCharacterManager::InstancePtr();
        if (!p || !pChrMgr)
            return entities;

        CInstanceBase* pkMain = p->NEW_GetMainActorPtr();
        if (!pkMain)
            return entities;

        for (auto it = pChrMgr->CharacterInstanceBegin(); it != pChrMgr->CharacterInstanceEnd(); ++it)
        {
            CInstanceBase* pkInst = *it;
            if (!pkInst || pkInst == pkMain)
                continue;

            float dist = pkMain->NEW_GetDistanceFromDestInstance(*pkInst);
            if (dist <= radius)
            {
                Client::Contracts::EntitySnapshot snap{};
                snap.vid = pkInst->GetVirtualID();
                snap.vnum = pkInst->GetRace();
                snap.name = pkInst->GetNameString();
                snap.distance = dist;
                snap.isDead = pkInst->IsDead();
                snap.isHostile = pkInst->IsEnemy();

                TPixelPosition pos;
                pkInst->NEW_GetPixelPosition(&pos);
                snap.x = pos.x;
                snap.y = pos.y;
                snap.z = pos.z;

                if (pkInst->IsPC())
                    snap.type = Client::Contracts::EntityType::Player;
                else if (pkInst->IsStone())
                    snap.type = Client::Contracts::EntityType::Stone;
                else if (pkInst->IsEnemy())
                    snap.type = Client::Contracts::EntityType::Monster;
                else if (pkInst->IsNPC())
                    snap.type = Client::Contracts::EntityType::NPC;
                else if (pkInst->IsDoor())
                    snap.type = Client::Contracts::EntityType::Door;
                else
                    snap.type = Client::Contracts::EntityType::Unknown;

                entities.push_back(std::move(snap));
            }
        }
        return entities;
    }

    std::optional<Client::Contracts::EntitySnapshot> WorldQueryAdapter::GetEntityByVID(uint32_t vid) const
    {
        auto* pChrMgr = CPythonCharacterManager::InstancePtr();
        if (!pChrMgr)
            return std::nullopt;

        CInstanceBase* pkInst = pChrMgr->GetInstancePtr(vid);
        if (!pkInst)
            return std::nullopt;

        auto* p = CPythonPlayer::InstancePtr();
        CInstanceBase* pkMain = p ? p->NEW_GetMainActorPtr() : nullptr;

        Client::Contracts::EntitySnapshot snap{};
        snap.vid = pkInst->GetVirtualID();
        snap.vnum = pkInst->GetRace();
        snap.name = pkInst->GetNameString();
        snap.distance = pkMain ? pkMain->NEW_GetDistanceFromDestInstance(*pkInst) : 0.0f;
        snap.isDead = pkInst->IsDead();
        snap.isHostile = pkInst->IsEnemy();

        TPixelPosition pos;
        pkInst->NEW_GetPixelPosition(&pos);
        snap.x = pos.x;
        snap.y = pos.y;
        snap.z = pos.z;

        if (pkInst->IsPC())
            snap.type = Client::Contracts::EntityType::Player;
        else if (pkInst->IsStone())
            snap.type = Client::Contracts::EntityType::Stone;
        else if (pkInst->IsEnemy())
            snap.type = Client::Contracts::EntityType::Monster;
        else if (pkInst->IsNPC())
            snap.type = Client::Contracts::EntityType::NPC;
        else if (pkInst->IsDoor())
            snap.type = Client::Contracts::EntityType::Door;
        else
            snap.type = Client::Contracts::EntityType::Unknown;

        return snap;
    }

    std::optional<Client::Contracts::EntitySnapshot> WorldQueryAdapter::GetNearestHostile() const
    {
        auto entities = GetEntitiesInRadius(5000.0f);
        std::optional<Client::Contracts::EntitySnapshot> nearest = std::nullopt;
        float minDist = 999999.0f;

        for (const auto& entity : entities)
        {
            if (entity.isHostile && !entity.isDead && entity.distance < minDist)
            {
                minDist = entity.distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    std::optional<Client::Contracts::EntitySnapshot> WorldQueryAdapter::GetNearestItemGround() const
    {
        return std::nullopt;
    }

    int WorldQueryAdapter::ForEachEntityInRadius(float radius, Client::Contracts::EntityVisitor visitor, void* userData) const
    {
        if (!visitor)
            return 0;

        auto* p = CPythonPlayer::InstancePtr();
        auto* pChrMgr = CPythonCharacterManager::InstancePtr();
        if (!p || !pChrMgr)
            return 0;

        CInstanceBase* pkMain = p->NEW_GetMainActorPtr();
        if (!pkMain)
            return 0;

        int count = 0;
        for (auto it = pChrMgr->CharacterInstanceBegin(); it != pChrMgr->CharacterInstanceEnd(); ++it)
        {
            CInstanceBase* pkInst = *it;
            if (!pkInst || pkInst == pkMain)
                continue;

            float dist = pkMain->NEW_GetDistanceFromDestInstance(*pkInst);
            if (dist <= radius)
            {
                Client::Contracts::EntitySnapshot snap{};
                snap.vid = pkInst->GetVirtualID();
                snap.vnum = pkInst->GetRace();
                snap.name = pkInst->GetNameString();
                snap.distance = dist;
                snap.isDead = pkInst->IsDead();
                snap.isHostile = pkInst->IsEnemy();

                TPixelPosition pos;
                pkInst->NEW_GetPixelPosition(&pos);
                snap.x = pos.x;
                snap.y = pos.y;
                snap.z = pos.z;

                if (pkInst->IsPC())
                    snap.type = Client::Contracts::EntityType::Player;
                else if (pkInst->IsStone())
                    snap.type = Client::Contracts::EntityType::Stone;
                else if (pkInst->IsNPC())
                    snap.type = Client::Contracts::EntityType::NPC;
                else if (pkInst->IsDoor())
                    snap.type = Client::Contracts::EntityType::Door;
                else
                    snap.type = Client::Contracts::EntityType::Monster;

                visitor(snap, userData);
                ++count;
            }
        }
        return count;
    }

    void InitializeMCPAdapters()
    {
        static PlayerDomainAdapter s_playerAdapter;
        static InventoryDomainAdapter s_inventoryAdapter;
        static CombatDomainAdapter s_combatAdapter;
        static WorldQueryAdapter s_worldAdapter;

        Client::MCP::AgentMCPBridge::Instance().Initialize(
            &s_playerAdapter,
            &s_inventoryAdapter,
            &s_combatAdapter,
            &s_worldAdapter
        );

        // Rejestracja w natywnym mostku C-ABI BotApi_FFI
        Bot_InitializeContracts(
            &s_playerAdapter,
            &s_worldAdapter,
            &s_combatAdapter,
            &s_inventoryAdapter
        );
    }
}
