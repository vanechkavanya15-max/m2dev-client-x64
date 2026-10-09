#include "StranglerNetworkFacade.h"
#include "../Network/ModernPacketDispatcher.h"

namespace Client::Bridge {

StranglerNetworkFacade::StranglerNetworkFacade() 
    : m_dispatcher(std::make_unique<Client::Network::ModernPacketDispatcher>()) 
{
}

StranglerNetworkFacade::~StranglerNetworkFacade() = default;
StranglerNetworkFacade::StranglerNetworkFacade(StranglerNetworkFacade&&) noexcept = default;
StranglerNetworkFacade& StranglerNetworkFacade::operator=(StranglerNetworkFacade&&) noexcept = default;

void StranglerNetworkFacade::RegisterHandler(uint8_t opcode, Client::Network::IPacketHandler* handler) {
    if (m_dispatcher) {
        m_dispatcher->RegisterHandler(opcode, handler);
    }
}

void StranglerNetworkFacade::UnregisterHandler(uint8_t opcode) {
    if (m_dispatcher) {
        m_dispatcher->UnregisterHandler(opcode);
    }
}

EterBase::PacketResult<void> StranglerNetworkFacade::DispatchPacket(uint8_t opcode, std::span<const uint8_t> payload) {
    if (!m_dispatcher) {
        return std::unexpected(EterBase::PacketError::SessionClosed);
    }
    return m_dispatcher->Dispatch(opcode, payload);
}

} // namespace Client::Bridge
