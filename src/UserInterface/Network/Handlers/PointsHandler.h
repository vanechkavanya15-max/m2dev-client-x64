#pragma once

#include <cstdint>
#include <array>
#include <vector>

#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"

/**
 * @brief Event triggered when the player's full points status is updated.
 * Used to signal decoupled GUI elements to refresh the points bars and stats window.
 */
struct PlayerPointsUpdateEvent : public UserInterface::Core::IEvent {
    // Empty payload because the GUI (Python) should fetch directly 
    // from the singleton CPythonPlayer cache when notified.
    // This maintains symmetry with the old PyCallClassMemberFunc("RefreshStatus") behavior
    // without passing redundant massive structs.
};

/**
 * @brief Handler for full points packet updates from the server.
 * Ensures strict C++23 standard decoupling by preventing direct Python function calls.
 */
class PointsHandler {
public:
    /**
     * @brief Processes the GC_POINTS packet.
     * @param packet The network packet containing all player points (HP, MP, EXP, Stamina, Gold).
     * @return PacketResult<void> Returns None on success, or a PacketError on failure.
     */
    static EterBase::PacketResult<void> HandlePoints(const TPacketGCPoints& packet);
};
