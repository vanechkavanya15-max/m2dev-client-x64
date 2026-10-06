#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../PacketDispatcher.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <span>
#include <cstdint>

namespace UserInterface::Core {
    /**
     * @brief Zdarzenie rozglaszane gdy pakiet w strumieniu Game Phase nie zostanie sparsowany.
     */
    struct PacketProcessingErrorEvent : public IEvent {
        uint16_t opcode;
        EterBase::PacketError error;

        PacketProcessingErrorEvent(uint16_t op, EterBase::PacketError err) 
            : opcode(op), error(err) {}
    };
}

namespace Network::Dispatchers {

    /**
     * @brief NetworkStreamPhaseGameBridge
     *
     * Klasa odpowiedzialna za mostkowanie z przestarzalego mechanizmu (wielkiego switch-case
     * w CPythonNetworkStreamPhaseGame) do nowej architektury PacketDispatcher w standardzie C++23.
     */
    class NetworkStreamPhaseGameBridge {
    public:
        /**
         * @brief Przekazuje pakiet do nowego dyspozytora uzywajac typowania w stylu std::span.
         * 
         * @param opcode 16-bitowy kod polecenia pakietu.
         * @param payload Skompresowane badz odszyfrowane cialo pakietu, z wycietym naglowkiem (zaleznie od architektury strumienia).
         * @return EterBase::PacketResult<void> z opcjonalnym bledem obslugi.
         */
        static EterBase::PacketResult<void> RouteGamePacket(uint16_t opcode, std::span<const uint8_t> payload) {
            EterBase::ModernLogger::Trace("NetworkStreamPhaseGameBridge: Routing packet opcode 0x{:04X}, size: {} bytes", opcode, payload.size());
            
            auto& dispatcher = Network::PacketDispatcher::Instance();

            if (!dispatcher.HasHandler(opcode)) {
                EterBase::ModernLogger::Warning("NetworkStreamPhaseGameBridge: Unhandled packet opcode 0x{:04X}", opcode);
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
            }

            auto result = dispatcher.DispatchModern(opcode, payload);
            if (!result.has_value()) {
                EterBase::ModernLogger::Error("NetworkStreamPhaseGameBridge: Failed to dispatch packet opcode 0x{:04X}. Error: {}", 
                    opcode, EterBase::ToString(result.error()));
                
                // Emisja Bledu poprzez szyne zdarzen GUI (EventBus)
                UserInterface::Core::EventBus::GetInstance().Publish(
                    UserInterface::Core::PacketProcessingErrorEvent(opcode, result.error())
                );
            }
            
            return result;
        }
    };

}
