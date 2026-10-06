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
    struct ExchangeAcceptEventAdapter : public UserInterface::Core::IEvent
    {
        bool isInitiator;
        bool isAccepted;

        ExchangeAcceptEventAdapter(bool isInitiator, bool isAccepted)
            : isInitiator(isInitiator), isAccepted(isAccepted)
        {
        }
    };

    EterBase::PacketResult<void> DispatchExchangeAccept(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCExchange))
        {
            EterBase::ModernLogger::Error("DispatchExchangeAccept: Otrzymano zbyt krotki bufor pakietu ({} < {}).",
                buffer.size(), sizeof(TPacketGCExchange));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());

        bool isMe = (packet->is_me != 0);
        bool isAccepted = (packet->arg1 != 0);

        EterBase::ModernLogger::Info("DispatchExchangeAccept: Exchange accept state changed: is_me={}, accepted={}", 
            packet->is_me, packet->arg1);

        auto& exchange = CPythonExchange::Instance();

        if (isMe)
        {
            exchange.SetAcceptToSelf(packet->arg1);
        }
        else
        {
            exchange.SetAcceptToTarget(packet->arg1);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(
            ExchangeAcceptEventAdapter(isMe, isAccepted)
        );

        return {};
    }
}
