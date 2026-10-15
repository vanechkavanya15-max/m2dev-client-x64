#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <span>
#include <cstdint>

namespace Network::Dispatchers
{
#pragma pack(push, 1)
    /**
     * @brief Struktura pakietu informacji o wierzchowcu (GC_MOUNT).
     * 
     * Uzywa scislego wyrownania (1-byte alignment).
     */
    struct PacketMount
    {
        uint16_t header;
        uint16_t length;
        uint32_t vid;
        uint32_t mount_vid;
        uint8_t pos;
        uint32_t _x;
        uint32_t _y;
    };
#pragma pack(pop)

    /**
     * @brief Dyspozytor wejscia/zejscia z wierzchowca u gracza lub otoczenia.
     * @param buffer Surowy bufor bajtow ze strumienia sieciowego.
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub bledu.
     */
    EterBase::PacketResult<void> DispatchMountState(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketMount))
        {
            EterBase::ModernLogger::Error("DispatchMountState: Buffer too small. Expected {}, got {}", sizeof(PacketMount), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketMount*>(buffer.data());

        const EterBase::EntityId charId{packet->vid};
        const EterBase::ItemVnum mountVnum{packet->mount_vid};

        EterBase::ModernLogger::Info("DispatchMountState: VID: {}, MountVnum: {}, Pos: {}", charId.value(), mountVnum.value(), packet->pos);

        // Zgodnie ze standardem 2026, handler nie modyfikuje bezposrednio GUI/CInstanceBase.
        // Odczytuje dane i rozglasza zdarzenie do EventBusa.
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::MountStateChangedEvent(charId.value(), mountVnum.value(), packet->pos)
        );

        return {};
    }
}
