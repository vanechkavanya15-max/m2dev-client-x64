#include "../../StdAfx.h"
#include "ShopSignHandler.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/LogModern.h"
#include <cstring>
#include <string_view>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ShopSignHandler::Process(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCShopSign))
        {
            EterBase::ModernLogger::Error("ShopSignHandler: Buffer underflow. Expected {} bytes, got {}", 
                sizeof(TPacketGCShopSign), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCShopSign*>(buffer.data());
        const EterBase::EntityId targetId(packet->dwVID);

        // Safely extract the string view from the packed packet
        std::string_view signView(packet->szSign, strnlen(packet->szSign, sizeof(packet->szSign)));

        auto& player = CPythonPlayer::Instance();

        if (signView.empty())
        {
            // Empty sign means the shop is being closed
            if (player.IsMainCharacterIndex(targetId.value()))
            {
                player.ClosePrivateShop();
            }
        }
        else
        {
            // Non-empty sign means a shop has appeared
            if (player.IsMainCharacterIndex(targetId.value()))
            {
                player.OpenPrivateShop();
            }
        }

        // Broadcast the event to the EventBus instead of directly calling Python functions
        UserInterface::Core::EventBus::GetInstance().Publish(ShopSignEvent(targetId, signView));

        return {};
    }
}
