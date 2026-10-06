#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cstdint>
#include <span>

namespace UserInterface::Network::Dispatchers {

/**
 * @brief Dispatcher obslugujacy sieciowy pakiet smierci encji (TPacketGCDead).
 */
class CombatDispatcher_Dead {
public:
    /**
     * @brief Przetwarza pakiet powiadamiajacy o smierci aktora.
     * 
     * @param buffer Bufor z danymi sieciowymi z pakietu.
     * @return EterBase::PacketResult<void> informujacy o sukcesie lub bledzie.
     */
    static EterBase::PacketResult<void> Process(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCDead)) {
            EterBase::ModernLogger::Error("CombatDispatcher_Dead: Za krotki bufor, wymagane {}, otrzymano {}", sizeof(TPacketGCDead), buffer.size());
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCDead*>(buffer.data());
        EterBase::EntityId targetId(packet->vid);

        EterBase::ModernLogger::Debug("CombatDispatcher_Dead: Aktor {} zgloszony jako martwy", packet->vid);

        // Powiadomienie innych systemow o smierci (np. GUI, CharacterManager)
        Core::EventBus::GetInstance().Publish(Core::ActorDeadEvent(targetId.value()));

        return {};
    }
};

} // namespace UserInterface::Network::Dispatchers
