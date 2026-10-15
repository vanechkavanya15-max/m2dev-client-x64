#include "../../StdAfx.h"
#include <span>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Packet.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include "../../InstanceBase.h"

namespace CombatNetDispatch {

/**
 * @brief Event triggered when a combat point (HP, MP, SP, etc.) is updated from the server.
 */
struct CombatPointUpdateEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    uint8_t pointType;
    int32_t amount;
    int32_t value;

    CombatPointUpdateEvent(EterBase::EntityId id, uint8_t type, int32_t amt, int32_t val)
        : entityId(id), pointType(type), amount(amt), value(val) {}
};

/**
 * @brief Dispatcher for combat point updates (HP/MP/EXP/SP).
 * Validates payload, updates entity state, and emits an event for decoupled GUI updates.
 *
 * @param payload Binary span containing a TPacketGCPointChange struct.
 * @return PacketResult<void> Returns success or a specific packet error.
 */
EterBase::PacketResult<void> DispatchPointsUpdate(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCPointChange)) {
        EterBase::ModernLogger::Error("CombatNetDispatch: Buffer underflow in DispatchPointsUpdate (size: {}, expected: {})",
                                      payload.size(), sizeof(TPacketGCPointChange));
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCPointChange*>(payload.data());
    EterBase::EntityId entityId{packet->dwVID};

    EterBase::ModernLogger::Debug("CombatNetDispatch: Point update for VID {}, Type {}, Amount {}, Value {}",
                                  entityId.get(), packet->Type, packet->amount, packet->value);

    auto& charManager = CPythonCharacterManager::Instance();
    charManager.ShowPointEffect(packet->Type, entityId.get());

    auto* mainInstance = charManager.GetMainActorPtr();
    if (mainInstance && entityId.get() == mainInstance->GetVirtualID()) {
        auto& player = CPythonPlayer::Instance();
        player.SetStatus(packet->Type, packet->value);

        if (packet->Type == POINT_ENERGY && packet->value == 0) {
            player.SetStatus(POINT_ENERGY_END_TIME, 0);
        }
    } else if (packet->Type == POINT_LEVEL) {
        auto* targetInstance = charManager.GetInstancePtr(entityId.get());
        if (targetInstance) {
            targetInstance->SetLevel(packet->value);
            targetInstance->UpdateTextTailLevel(packet->value);
        }
    }

    CombatPointUpdateEvent event{entityId, packet->Type, packet->amount, packet->value};
    UserInterface::Core::EventBus::GetInstance().Publish(event);

    return {};
}

} // namespace CombatNetDispatch
