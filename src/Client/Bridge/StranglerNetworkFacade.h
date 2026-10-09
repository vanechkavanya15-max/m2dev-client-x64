#pragma once

#include <cstdint>
#include <span>
#include <memory>
#include "EterBase/PacketResult.h"

namespace Client::Network {
    class ModernPacketDispatcher;
    class IPacketHandler;
}

namespace Client::Bridge {

/**
 * @brief Fasada migrujaca starszy kod sieciowy (PythonNetworkStream) 
 * na nowa architekture handlerow i dekoderow (ModernPacketDispatcher).
 * Realizuje wzorzec Strangler Fig - obudowuje nowa logike dla starszych wywolan.
 */
class StranglerNetworkFacade {
public:
    StranglerNetworkFacade();
    explicit StranglerNetworkFacade(Client::Network::ModernPacketDispatcher* dispatcher);
    ~StranglerNetworkFacade();

    StranglerNetworkFacade(const StranglerNetworkFacade&) = delete;
    StranglerNetworkFacade& operator=(const StranglerNetworkFacade&) = delete;
    StranglerNetworkFacade(StranglerNetworkFacade&&) noexcept;
    StranglerNetworkFacade& operator=(StranglerNetworkFacade&&) noexcept;

    static StranglerNetworkFacade& Instance() noexcept;

    /**
     * @brief Rejestruje nowy zmodernizowany handler pod dany opcode (16-bit / 8-bit).
     */
    void RegisterHandler(uint16_t opcode, Client::Network::IPacketHandler* handler);

    /**
     * @brief Odrejestrowuje handler dla danego opcode.
     */
    void UnregisterHandler(uint16_t opcode);

    [[nodiscard]] bool HasHandler(uint16_t opcode) const noexcept;

    /**
     * @brief Przekazuje obsluge pakietu (opcode, payload) do nowoczesnego dispatchera.
     * Uzywa mechaniki Result dla bezpiecznej obslugi bledow bez wyjatkow.
     */
    [[nodiscard]] EterBase::PacketResult<void> DispatchPacket(uint16_t opcode, std::span<const uint8_t> payload);

    void RegisterDefaultHandlers();

    // Dedykowana rejestracja handlerow domenowych
    void RegisterPartyHandlers();
    void RegisterGuildHandlers();
    void RegisterQuestDialogHandlers();
    void RegisterRefineExchangeHandlers();

    [[nodiscard]] Client::Network::ModernPacketDispatcher& GetDispatcher() noexcept;

private:
    std::unique_ptr<Client::Network::ModernPacketDispatcher> m_ownedDispatcher;
    Client::Network::ModernPacketDispatcher* m_dispatcher{nullptr};
};

} // namespace Client::Bridge
