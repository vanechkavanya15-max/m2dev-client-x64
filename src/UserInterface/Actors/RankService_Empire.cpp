#include "../StdAfx.h"
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <expected>

#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../Core/EventBus.h"

namespace UserInterface::Actors::RankTitleService
{
    /**
     * @brief Zdarzenie przypisania krolestwa dla calej sesji klienta.
     * Wykorzystywane przez systemy UI do odswiezenia wizualnych elementow (np. tla, mapy).
     */
    struct ClientEmpireAssignedEvent : public UserInterface::Core::IEvent
    {
        uint8_t empireId;
        std::string empireName;
        uint32_t colorArgb;

        ClientEmpireAssignedEvent(uint8_t id, std::string name, uint32_t color)
            : empireId(id), empireName(std::move(name)), colorArgb(color) {}
    };

    /**
     * @brief Zdarzenie przypisania krolestwa dla konkretnego aktora (VID).
     * Odpala aktualizacje koloru nazwy postaci nad jej glowa.
     */
    struct ActorEmpireAssignedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;
        uint8_t empireId;
        std::string empireName;
        uint32_t colorArgb;

        ActorEmpireAssignedEvent(EterBase::EntityId id, uint8_t empId, std::string name, uint32_t color)
            : entityId(id), empireId(empId), empireName(std::move(name)), colorArgb(color) {}
    };

    /**
     * @brief Zwraca nazwe krolestwa i jego domyslny kolor (ARGB) na podstawie ID.
     * @param empireId Identyfikator krolestwa (1: Shinsoo, 2: Chunjo, 3: Jinno)
     * @return std::pair<std::string, uint32_t> Nazwa i kolor
     */
    static auto GetEmpireDetails(uint8_t empireId) -> std::pair<std::string, uint32_t>
    {
        switch (empireId)
        {
            case 1: return {"Shinsoo", 0xFFFF0000}; // Czerwony
            case 2: return {"Chunjo",  0xFFFFFF00}; // Zolty
            case 3: return {"Jinno",   0xFF0000FF}; // Niebieski
            default: return {"None",   0xFFFFFFFF}; // Bialy
        }
    }

    /**
     * @brief Przetwarza pakiet TPacketGCEmpire okreslajacy krolestwo klienta.
     * @param buffer Bufor bajtow zawierajacy pakiet.
     * @return EterBase::PacketResult<void> Sukces lub blad formatu pakietu.
     */
    EterBase::PacketResult<void> ProcessClientEmpirePacket(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCEmpire))
        {
            EterBase::ModernLogger::Error("RankTitleService: Buffer underflow (oczekiwano {}, otrzymano {})", sizeof(TPacketGCEmpire), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCEmpire*>(buffer.data());
        auto [empireName, color] = GetEmpireDetails(packet->bEmpire);

        EterBase::ModernLogger::Info("RankTitleService: Przetworzono pakiet krolestwa. Klient przypisany do: {} (ID: {}) z kolorem: {:08X}", empireName, packet->bEmpire, color);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ClientEmpireAssignedEvent{packet->bEmpire, empireName, color}
        );

        return {};
    }

    /**
     * @brief Przypisuje kolor krolestwa dla konkretnego aktora w swiecie gry.
     * @param entityId Silny typ reprezentujacy ID aktora (VID).
     * @param empireId ID krolestwa do przypisania.
     * @return std::expected<void, EterBase::EntityError> Sukces lub kod bledu domenowego.
     */
    std::expected<void, EterBase::EntityError> AssignActorEmpireColor(EterBase::EntityId entityId, uint8_t empireId)
    {
        if (!entityId)
        {
            EterBase::ModernLogger::Warning("RankTitleService: Proba przypisania krolestwa do niewaznego aktora.");
            return EterBase::MakeError(EterBase::EntityError::NotFound);
        }

        auto [empireName, color] = GetEmpireDetails(empireId);

        EterBase::ModernLogger::Debug("RankTitleService: Aktorowi {} przypisano krolestwo {} (kolor: {:08X})", entityId.value(), empireName, color);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ActorEmpireAssignedEvent{entityId, empireId, empireName, color}
        );

        return {};
    }
}
