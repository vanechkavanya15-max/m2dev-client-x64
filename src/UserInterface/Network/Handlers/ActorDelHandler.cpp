#include "../../StdAfx.h"
#include "ActorDelHandler.h"
#include "../../NetworkActorManager.h"
#include "../../Packet.h"

namespace Network::Handlers
{
    bool HandleActorDel(std::span<const uint8_t> payload, CNetworkActorManager& actorManager)
    {
        if (payload.size() < sizeof(TPacketGCCharacterDelete))
            return false;

        const auto* packet = reinterpret_cast<const TPacketGCCharacterDelete*>(payload.data());
        actorManager.RemoveActor(packet->dwVID);
        return true;
    }
}
