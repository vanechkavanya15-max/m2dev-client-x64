#include "../../StdAfx.h"
#include "ActorAddHandler.h"
#include "../../NetworkActorManager.h"
#include "../../PythonNonPlayer.h"
#include "../../PythonBackground.h"
#include "GameLib/ActorInstance.h"

namespace Network::Handlers
{
    bool HandleActorAdd(std::span<const uint8_t> payload, CNetworkActorManager& actorManager)
    {
        if (payload.size() != sizeof(ActorAddPacket))
            return false;

        const auto* packet = reinterpret_cast<const ActorAddPacket*>(payload.data());

        // Convert coordinates from Global to Local space
        int32_t localX = packet->x;
        int32_t localY = packet->y;
        CPythonBackground::Instance().GlobalPositionToLocalPosition(localX, localY);

        SNetworkActorData actorData;
        actorData.m_bType = packet->type;
        actorData.m_dwMovSpd = packet->movingSpeed;
        actorData.m_dwAtkSpd = packet->attackSpeed;
        actorData.m_dwRace = packet->raceNum;
        actorData.m_dwStateFlags = packet->stateFlag;
        actorData.m_dwVID = packet->id;
        actorData.m_fRot = packet->angle;

        // Copy affect flags accurately
        actorData.m_kAffectFlags.CopyData(0, sizeof(packet->affectFlag[0]), &packet->affectFlag[0]);
        actorData.m_kAffectFlags.CopyData(32, sizeof(packet->affectFlag[1]), &packet->affectFlag[1]);

        actorData.SetPosition(localX, localY);

        // Reset non-provided fields to default zeros
        actorData.m_sAlignment = 0;
        actorData.m_byPKMode = 0;
        actorData.m_dwGuildID = 0;
        actorData.m_dwEmpireID = 0;
        actorData.m_dwArmor = 0;
        actorData.m_dwWeapon = 0;
        actorData.m_dwHair = 0;
        actorData.m_dwMountVnum = 0;
        actorData.m_dwLevel = 0;

        // Populate name if NPC/Mob
        if (packet->type == CActorInstance::TYPE_NPC || packet->type == CActorInstance::TYPE_ENEMY || packet->type == CActorInstance::TYPE_POLY)
        {
            const char* name = "";
            if (CPythonNonPlayer::Instance().GetName(packet->raceNum, &name))
            {
                actorData.m_stName = name;
            }
        }

        // Add instance to the C++ memory map, strictly decoupling from Python UI updates
        actorManager.AppendActor(actorData);

        return true;
    }
}
