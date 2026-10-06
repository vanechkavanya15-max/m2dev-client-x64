#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <span>
#include <cstdint>
#include <unordered_map>

namespace Network::Dispatchers
{
    /**
     * @brief Zdarzenie rozglaszane, gdy pakiet Item zostal poprawnie przerutowany i przetworzony.
     */
    struct ItemPacketRoutedEvent : public UserInterface::Core::IEvent
    {
        uint16_t opcode;

        explicit ItemPacketRoutedEvent(uint16_t opcode) : opcode(opcode) {}
    };

    /**
     * @brief Sygnatura handlera pakietow Item.
     */
    using ItemPacketHandlerFn = EterBase::PacketResult<void> (*)(std::span<const uint8_t>);

    /**
     * @brief Nowoczesny router (dispatcher) domenowy dla pakietow Item.
     * Zapobiega monolitycznym switchom. Implementacja Single Responsibility.
     */
    class NetFacadeItemRouter
    {
    public:
        static NetFacadeItemRouter& Instance()
        {
            static NetFacadeItemRouter instance;
            return instance;
        }

        void RegisterHandler(uint16_t opcode, ItemPacketHandlerFn handler)
        {
            m_handlers[opcode] = handler;
        }

        EterBase::PacketResult<void> Route(uint16_t opcode, std::span<const uint8_t> payload)
        {
            auto it = m_handlers.find(opcode);
            if (it == m_handlers.end() || !it->second)
            {
                EterBase::ModernLogger::Warning("NetFacadeItemRouter: Unhandled opcode {:#06x}", opcode);
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }

            auto result = it->second(payload);
            if (result.has_value())
            {
                EterBase::ModernLogger::Debug("NetFacadeItemRouter: Successfully routed opcode {:#06x}", opcode);
                UserInterface::Core::EventBus::GetInstance().Publish(ItemPacketRoutedEvent(opcode));
            }
            else
            {
                EterBase::ModernLogger::Error("NetFacadeItemRouter: Error routing opcode {:#06x}: {}", opcode, EterBase::ToString(result.error()));
            }

            return result;
        }

        void RegisterDefaultRoutes()
        {
            // Placeholder: Handlers would be registered here
            // e.g., RegisterHandler(ITEM_SET, Network::Handlers::ProcessItemSet);
        }

    private:
        NetFacadeItemRouter() = default;
        ~NetFacadeItemRouter() = default;

        NetFacadeItemRouter(const NetFacadeItemRouter&) = delete;
        NetFacadeItemRouter& operator=(const NetFacadeItemRouter&) = delete;

        std::unordered_map<uint16_t, ItemPacketHandlerFn> m_handlers;
    };
}
