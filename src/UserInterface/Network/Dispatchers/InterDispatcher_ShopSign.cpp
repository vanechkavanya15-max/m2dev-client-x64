#include "../../StdAfx.h"
#include "../PacketDispatcher.h"
#include "../../Packet.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../Core/ShopEvents.h"
#include <cstring>
#include <string>

namespace Network
{
    EterBase::PacketResult<void> HandleShopSign(std::span<const uint8_t> payload)
    {
        if (payload.size_bytes() < sizeof(TPacketGCShopSign))
        {
            EterBase::ModernLogger::Error("Buffer underflow in HandleShopSign. Expected {} bytes, got {}.", 
                                           sizeof(TPacketGCShopSign), payload.size_bytes());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCShopSign packet;
        std::copy_n(payload.data(), sizeof(TPacketGCShopSign), reinterpret_cast<uint8_t*>(&packet));
        
        std::string_view sign(packet.szSign, strnlen(packet.szSign, sizeof(packet.szSign)));

        if (sign.empty())
        {
            EterBase::ModernLogger::Info("Private shop disappear for VID {}", packet.dwVID);
            
            UserInterface::Core::PrivateShopDisappearEvent ev;
            ev.vid = packet.dwVID;
            UserInterface::Core::EventBus::Instance().Publish(ev);
        }
        else
        {
            EterBase::ModernLogger::Info("Private shop appear for VID {}, sign: {}", packet.dwVID, sign);
            
            UserInterface::Core::PrivateShopAppearEvent ev;
            ev.vid = packet.dwVID;
            ev.sign = std::string(sign);
            UserInterface::Core::EventBus::Instance().Publish(ev);
        }

        return {};
    }
}
