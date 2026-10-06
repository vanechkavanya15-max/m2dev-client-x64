#include "StdAfx.h"
#include "ItemGroundDelHandler.h"
#include "../../PythonItem.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessItemGroundDel(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketItemGroundDel))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketItemGroundDel*>(buffer.data());
        const EterBase::EntityId itemVid(packet->itemVid);

        // Usuniecie przedmiotu ze swiata gry
        CPythonItem::Instance().DeleteItem(itemVid.value());

        return {};
    }

    bool HandleItemGroundDel(std::span<const uint8_t> buffer)
    {
        return ProcessItemGroundDel(buffer).has_value();
    }
}
