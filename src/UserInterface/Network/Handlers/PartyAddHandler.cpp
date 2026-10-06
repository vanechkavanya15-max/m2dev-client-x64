#include "StdAfx.h"
#include "PartyAddHandler.h"
#include "../../PythonPlayer.h"
#include <cstring>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessPartyAdd(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketPartyAdd))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketPartyAdd*>(buffer.data());
        EterBase::EntityId id{packet->pid};

        // Zabezpieczenie przed brakiem null-terminatora
        size_t nameLen = strnlen(packet->name, sizeof(packet->name));
        std::string safeName(packet->name, nameLen);

        // Aktualizacja stanu logiki po stronie C++ (dodanie gracza do grupy)
        CPythonPlayer::Instance().AppendPartyMember(id.value(), safeName.c_str());

        // Rozeslanie zdarzenia do modulu UI w Pythonie poprzez EventBus
        UserInterface::Core::EventBus::Instance().Publish(PartyMemberAddEvent{id.value(), safeName, packet->role});

        return {};
    }

    bool HandlePartyAdd(std::span<const uint8_t> buffer)
    {
        return ProcessPartyAdd(buffer).has_value();
    }
}
