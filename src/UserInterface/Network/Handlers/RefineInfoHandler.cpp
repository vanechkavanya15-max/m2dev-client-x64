#include "../../StdAfx.h"
/**
 * @file RefineInfoHandler.cpp
 * @brief Handles network packets related to item refinement operations.
 * 
 * This file implements the handler for incoming refine information packets from the server.
 * It strictly maintains C++20 standards, uses modern types (std::span, std::string_view),
 * and completely decouples from the Python GUI by only updating the internal C++ state.
 */

#include <cstdint>
#include <vector>
#include <span>
#include <optional>
#include <cstring>

namespace Network::Handlers
{
    /**
     * @brief Maximum number of materials that can be required for a single refinement.
     */
    constexpr uint8_t MAX_REFINE_MATERIALS = 5;

    /**
     * @brief Represents a single material required for item refinement.
     */
    struct Material
    {
        uint32_t vnum;   ///< Virtual number of the material item
        int32_t count;   ///< Required amount of this material
    };

    /**
     * @brief Holds the current state of a refine operation, including costs, probabilities, and required materials.
     */
    struct RefineState
    {
        uint8_t position;                   ///< Position in inventory or refine window
        uint8_t type;                       ///< Type of refinement (e.g., normal, scroll, etc.)
        uint32_t resultVnum;                ///< Virtual number of the resulting item upon success
        int32_t cost;                       ///< Yang cost for the refinement
        int32_t probability;                ///< Probability of success (0-100)
        std::vector<Material> materials;    ///< List of required materials
    };

#pragma pack(push, 1)
    /**
     * @brief Raw binary structure for a single material in the network packet.
     */
    struct RawMaterial
    {
        uint32_t vnum;
        int32_t count;
    };

    /**
     * @brief Raw binary structure for the refine table data in the network packet.
     */
    struct RawRefineTable
    {
        uint32_t srcVnum;
        uint32_t resultVnum;
        uint8_t materialCount;
        int32_t cost;
        int32_t prob;
        RawMaterial materials[MAX_REFINE_MATERIALS];
    };

    /**
     * @brief Raw binary structure for the GC Refine Information network packet.
     */
    struct PacketGCRefineInformation
    {
        uint16_t header;
        uint16_t length;
        uint8_t type;
        uint8_t pos;
        RawRefineTable refineTable;
    };
#pragma pack(pop)

    /**
     * @brief Handler class responsible for parsing Refine Information packets and updating local C++ state.
     * 
     * This handler strictly decouples from the Python GUI, adhering to C++20 standards.
     * It safely processes binary payloads using std::span.
     */
    class RefineInfoHandler
    {
    public:
        /**
         * @brief Gets the singleton instance of the handler.
         * 
         * @return RefineInfoHandler& Reference to the handler instance.
         */
        static RefineInfoHandler& GetInstance()
        {
            static RefineInfoHandler instance;
            return instance;
        }

        /**
         * @brief Parses the refine information packet and updates the local state.
         * 
         * This handles both standard and new refine info packets since their internal layouts
         * are structurally identical.
         * 
         * @param buffer Binary buffer containing the packet data.
         * @return bool True if successfully parsed and state updated, false otherwise.
         */
        bool HandleRefineInformation(std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(PacketGCRefineInformation))
            {
                return false;
            }

            PacketGCRefineInformation packet;
            std::memcpy(&packet, buffer.data(), sizeof(PacketGCRefineInformation));

            RefineState newState{};
            newState.position = packet.pos;
            newState.type = packet.type;
            newState.resultVnum = packet.refineTable.resultVnum;
            newState.cost = packet.refineTable.cost;
            newState.probability = packet.refineTable.prob;

            uint8_t matCount = packet.refineTable.materialCount;
            if (matCount > MAX_REFINE_MATERIALS)
            {
                matCount = MAX_REFINE_MATERIALS; // Cap at max to prevent out of bounds logic errors
            }

            newState.materials.reserve(matCount);
            for (uint8_t i = 0; i < matCount; ++i)
            {
                newState.materials.push_back({
                    packet.refineTable.materials[i].vnum,
                    packet.refineTable.materials[i].count
                });
            }

            state_ = std::move(newState);
            return true;
        }

        /**
         * @brief Retrieves the current refine state.
         * 
         * @return const std::optional<RefineState>& The current refine state, if any.
         */
        const std::optional<RefineState>& GetState() const
        {
            return state_;
        }

        /**
         * @brief Clears the current refine state.
         */
        void ClearState()
        {
            state_.reset();
        }

    private:
        RefineInfoHandler() = default;
        ~RefineInfoHandler() = default;
        RefineInfoHandler(const RefineInfoHandler&) = delete;
        RefineInfoHandler& operator=(const RefineInfoHandler&) = delete;

        std::optional<RefineState> state_;
    };
} // namespace Network::Handlers
