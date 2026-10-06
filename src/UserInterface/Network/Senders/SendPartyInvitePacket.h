#pragma once

#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"

namespace Network::Senders
{
    /**
     * @brief Wysyla zaproszenie do grupy.
     * 
     * Tworzy i transmituje pakiet TPacketCGPartyInvite, korzystajac
     * ze scislych typow domenowych C++23.
     * 
     * @param targetId Silnie typowany identyfikator jednostki docelowej.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub kodem bledu.
     */
    EterBase::PacketResult<void> SendPartyInvitePacket(EterBase::EntityId targetId);
}
