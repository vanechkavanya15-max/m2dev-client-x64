#include "../../StdAfx.h"
#include "QuickSlotSwapHandler.h"
#include "../../AbstractPlayer.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers
{
    /**
     * @brief Przetwarza zamiane quickslotow zwracajac PacketResult C++23.
     * @param buffer Bufor bajtow pakietu.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> ProcessQuickSlotSwap(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketQuickSlotSwap))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketQuickSlotSwap*>(buffer.data());

        IAbstractPlayer& player = IAbstractPlayer::GetSingleton();
        player.MoveQuickSlot(packet->pos, packet->change_pos);

        UserInterface::Core::EventBus::GetInstance().Publish(QuickSlotSwapEvent{packet->pos, packet->change_pos});

        return {};
    }

    /**
     * @brief Kompatybilny z C++ wrapper obslugujacy zamiane quickslotow.
     * @param buffer Bufor bajtow pakietu.
     * @return true w przypadku poprawnego zdekodowania pakietu i zmiany w pamieci.
     */
    bool HandleQuickSlotSwapPacket(std::span<const uint8_t> buffer)
    {
        auto result = ProcessQuickSlotSwap(buffer);
        if (!result)
        {
            EterBase::ModernLogger::Error("Failed to process QuickSlotSwap packet: {}", result.error());
            return false;
        }

        return true;
    }
}
