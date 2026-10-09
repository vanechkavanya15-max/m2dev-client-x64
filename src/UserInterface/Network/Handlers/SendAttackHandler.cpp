#include "StdAfx.h"
#include "SendAttackHandler.h"
#include "../../../EterLib/NetStream.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Domain/CombatCommands.h"

bool SendAttackHandler::SendAttack(uint32_t targetId, uint32_t attackMotion, uint16_t sequence, CNetworkStream* networkStream)
{
    if (!networkStream)
        return false;

    auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
    if (!pDriver)
    {
        Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
        pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
    }

    if (!pDriver)
        return false;

    Network::Domain::AttackCommand cmd{
        .targetVid = targetId,
        .attackerVid = 0,
        .attackType = 0,
        .sequence = sequence,
        .attackMotion = attackMotion
    };

    auto encodedResult = pDriver->EncodeAttack(cmd);
    if (!encodedResult.has_value())
        return false;

    const auto& buffer = encodedResult.value();
    return networkStream->Send(static_cast<int>(buffer.size()), buffer.data());
}
