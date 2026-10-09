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
    ~StranglerNetworkFacade();

    StranglerNetworkFacade(const StranglerNetworkFacade&) = delete;
    StranglerNetworkFacade& operator=(const StranglerNetworkFacade&) = delete;
    StranglerNetworkFacade(StranglerNetworkFacade&&) noexcept;
    StranglerNetworkFacade& operator=(StranglerNetworkFacade&&) noexcept;

    /**
     * @brief Rejestruje nowy zmodernizowany handler pod dany opcode.
     */
    void RegisterHandler(uint8_t opcode, Client::Network::IPacketHandler* handler);

    /**
     * @brief Odrejestrowuje handler dla danego opcode.
     */
    void UnregisterHandler(uint8_t opcode);

    /**
     * @brief Przekazuje obsluge pakietu (opcode, payload) do nowoczesnego dispatchera.
     * Uzywa mechaniki Result dla bezpiecznej obslugi bledow bez wyjatkow.
     */
    [[nodiscard]] EterBase::PacketResult<void> DispatchPacket(uint8_t opcode, std::span<const uint8_t> payload);

private:
    std::unique_ptr<Client::Network::ModernPacketDispatcher> m_dispatcher;
};

} // namespace Client::Bridge
