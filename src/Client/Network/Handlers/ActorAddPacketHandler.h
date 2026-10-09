#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/PacketResult.h"
#include "../../../EterBase/StrongTypes.h"
#include "../ModernPacketDispatcher.h"
#include "../../World/ActorRegistry.h"
#include "../Protocol/Packets/Packet_ActorAdd.h"

namespace Client::Network::Handlers {

/**
 * @brief Obsluguje pakiet HEADER_GC_CHARACTER_ADD z serwera.
 * 
 * Implementuje IPacketHandler dla dodawania nowych aktorow (graczy, NPC, potworow)
 * do rejestru aktorow (ActorRegistry).
 */
class ActorAddPacketHandler : public IPacketHandler {
public:
    /**
     * @brief Konstruktor handlera.
     * @param registry Referencja do rejestru aktorow, do ktorego beda dodawane byty.
     */
    explicit ActorAddPacketHandler(Client::World::ActorRegistry& registry) noexcept;
    ~ActorAddPacketHandler() override = default;

    // Usuniecie mozliwosci kopiowania/przenoszenia
    ActorAddPacketHandler(const ActorAddPacketHandler&) = delete;
    ActorAddPacketHandler& operator=(const ActorAddPacketHandler&) = delete;
    ActorAddPacketHandler(ActorAddPacketHandler&&) = delete;
    ActorAddPacketHandler& operator=(ActorAddPacketHandler&&) = delete;

    /**
     * @brief Przetwarza nadchodzacy pakiet dodania aktora.
     * @param payload Bufor z danymi pakietu.
     * @return EterBase::PacketResult<void> Zwraca sukces lub blad parsowania/rejestracji.
     */
    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;

    /**
     * @brief Zwraca oczekiwany rozmiar pakietu.
     */
    [[nodiscard]] uint16_t GetExpectedSize() const override;

    /**
     * @brief Okresla, czy rozmiar pakietu jest dynamiczny.
     */
    [[nodiscard]] bool IsDynamicSize() const override;

private:
    Client::World::ActorRegistry& m_registry;
};

} // namespace Client::Network::Handlers
