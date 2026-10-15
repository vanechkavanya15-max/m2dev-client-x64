#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cstdint>
#include <span>

namespace UserInterface::Network::Dispatchers
{
    struct BattleFieldStateEvent : public UserInterface::Core::IEvent
    {
        uint8_t state;
        explicit BattleFieldStateEvent(uint8_t s) : state(s) {}
    };

    class CombatDispatcher_BattleField
    {
    public:
        static EterBase::PacketResult<void> Process(std::span<const uint8_t> buffer)
        {
            if (buffer.empty())
            {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            uint8_t state = buffer[0];
            UserInterface::Core::EventBus::GetInstance().Publish(BattleFieldStateEvent(state));
            EterBase::ModernLogger::Info("CombatDispatcher_BattleField: BattleField state updated: {}", state);

            return {};
        }
    };
}
