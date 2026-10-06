#include "../../StdAfx.h"
#include "ItemDelHandler.h"
#include "../../AbstractPlayer.h"

namespace Network::Handlers
{

bool HandleItemDelPacket(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(ItemDelPacket))
    {
        return false;
    }

    const auto* packet = reinterpret_cast<const ItemDelPacket*>(buffer.data());

    TItemData emptyItemData{}; // Zastępuje memset w C++20

    auto& player = IAbstractPlayer::GetSingleton();
    player.SetItemData(packet->pos, emptyItemData);

    return true;
}

} // namespace Network::Handlers
