#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu dodania czlonka do grupy (TPacketGCPartyAdd).
     */
    struct PacketPartyAdd
    {
        uint16_t header;
        uint16_t length;
        uint32_t pid;
        char name[24 + 1]; // CHARACTER_NAME_MAX_LEN = 24
        uint8_t role; // Odpowiada wymaganiu (EntityId, nazwa gracza, rola)
    };
#pragma pack(pop)

    /**
     * @brief Zdarzenie dodania czlonka do grupy wysylane na EventBus.
     */
    struct PartyMemberAddEvent : public UserInterface::Core::IEvent
    {
        uint32_t pid;
        std::string name;
        uint8_t role;

        PartyMemberAddEvent(uint32_t pid, std::string_view name, uint8_t role) : pid(pid), name(name), role(role) {}
    };

    /**
     * @brief Przetwarza pakiet dodania czlonka do grupy, uzywajac std::expected.
     * @param buffer Bufor bajtow pakietu (std::span).
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub porazki.
     */
    EterBase::PacketResult<void> ProcessPartyAdd(std::span<const uint8_t> buffer);

    /**
     * @brief Przetwarza pakiet dodania czlonka do grupy i kieruje dane do logiki. (Kompatybilnosc z python network stream)
     * @param buffer Bufor bajtow pakietu (std::span).
     * @return true jesli przetworzono poprawnie, false w przeciwnym razie.
     */
    bool HandlePartyAdd(std::span<const uint8_t> buffer);
}
