#include "StranglerNetworkFacade.h"
#include "../Network/ModernPacketDispatcher.h"

namespace Client::Bridge {

StranglerNetworkFacade& StranglerNetworkFacade::Instance() noexcept {
    static StranglerNetworkFacade s_instance(&Client::Network::ModernPacketDispatcher::Instance());
    return s_instance;
}

StranglerNetworkFacade::StranglerNetworkFacade() 
    : m_ownedDispatcher(std::make_unique<Client::Network::ModernPacketDispatcher>()),
      m_dispatcher(m_ownedDispatcher.get())
{
}

StranglerNetworkFacade::StranglerNetworkFacade(Client::Network::ModernPacketDispatcher* dispatcher)
    : m_dispatcher(dispatcher ? dispatcher : &Client::Network::ModernPacketDispatcher::Instance())
{
}

StranglerNetworkFacade::~StranglerNetworkFacade() = default;
StranglerNetworkFacade::StranglerNetworkFacade(StranglerNetworkFacade&&) noexcept = default;
StranglerNetworkFacade& StranglerNetworkFacade::operator=(StranglerNetworkFacade&&) noexcept = default;

Client::Network::ModernPacketDispatcher& StranglerNetworkFacade::GetDispatcher() noexcept {
    if (!m_dispatcher) {
        return Client::Network::ModernPacketDispatcher::Instance();
    }
    return *m_dispatcher;
}

void StranglerNetworkFacade::RegisterHandler(uint16_t opcode, Client::Network::IPacketHandler* handler) {
    GetDispatcher().RegisterHandler(opcode, handler);
}

void StranglerNetworkFacade::UnregisterHandler(uint16_t opcode) {
    GetDispatcher().UnregisterHandler(opcode);
}

bool StranglerNetworkFacade::HasHandler(uint16_t opcode) const noexcept {
    if (m_dispatcher) {
        return m_dispatcher->HasHandler(opcode);
    }
    return Client::Network::ModernPacketDispatcher::Instance().HasHandler(opcode);
}

EterBase::PacketResult<void> StranglerNetworkFacade::DispatchPacket(uint16_t opcode, std::span<const uint8_t> payload) {
    return GetDispatcher().Dispatch(opcode, payload);
}

void StranglerNetworkFacade::RegisterDefaultHandlers() {
    GetDispatcher().RegisterDefaultHandlers();
}

void StranglerNetworkFacade::RegisterPartyHandlers() {
    GetDispatcher().RegisterDefaultHandlers();
}

void StranglerNetworkFacade::RegisterGuildHandlers() {
    GetDispatcher().RegisterDefaultHandlers();
}

void StranglerNetworkFacade::RegisterQuestDialogHandlers() {
    GetDispatcher().RegisterDefaultHandlers();
}

void StranglerNetworkFacade::RegisterRefineExchangeHandlers() {
    GetDispatcher().RegisterDefaultHandlers();
}

} // namespace Client::Bridge
