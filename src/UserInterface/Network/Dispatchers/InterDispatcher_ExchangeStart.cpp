#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"
#include "../../PythonExchange.h"
#include <span>

namespace InterNetDispatch
{
    struct ExchangeStartEventAdapter : public UserInterface::Core::IEvent
    {
        uint32_t partnerVid;
        explicit ExchangeStartEventAdapter(uint32_t vid) : partnerVid(vid) {}
    };

    EterBase::PacketResult<void> DispatchExchangeStart(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCExchange))
        {
            EterBase::ModernLogger::Error("DispatchExchangeStart: Zbyt krotki bufor.");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());
        CPythonExchange::Instance().Start();

        UserInterface::Core::EventBus::GetInstance().Publish(ExchangeStartEventAdapter(packet->arg1));
        EterBase::ModernLogger::Info("DispatchExchangeStart: Exchange started with VID {}", packet->arg1);

        return {};
    }
}
