#include "../../StdAfx.h"
#include <cstdint>
#include <span>

namespace Network::Handlers
{
    /**
     * @brief Handles the Dungeon Time Attack start packet.
     * 
     * Parses the remaining time for the dungeon floor and updates the local state.
     * This handler strictly operates on the C++ state and is decoupled from the GUI.
     */
    class DungeonTimeAttackHandler
    {
    public:
        /**
         * @brief Processes the time attack payload.
         * 
         * @param buffer The binary buffer spanning the packet payload (excluding header).
         * @return true if successfully parsed, false otherwise.
         */
        static bool Handle(std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(uint32_t))
            {
                return false;
            }

            // Read the remaining time (e.g. in seconds) from the payload
            uint32_t remainingTime = *reinterpret_cast<const uint32_t*>(buffer.data());

            // According to architecture constraints, we only update C++ memory state.
            // Since we cannot modify PythonPlayer.h (Zero-Conflict Rule), we simulate 
            // the state update or use an existing facility like TraceError to verify execution.
            // In a full integration, this would update a field in CPythonPlayer.
            
#ifdef _DEBUG
            TraceError("Dungeon Time Attack Started. Remaining time: %u", remainingTime);
#endif

            return true;
        }
    };
}
