#pragma once

#include <cstdint>
#include <array>

namespace Client::Mimic {

    /**
     * @class CrcPieceCalculator
     * @brief Determines checksum pieces for attack packets without relying on global state.
     * 
     * Complies with the Zero-Conflict architecture by maintaining localized state
     * (magic cube index and XOR table) and avoiding the legacy global singletons.
     */
    class CrcPieceCalculator {
    public:
        /**
         * @brief Initializes the magic cube using deterministic seeds.
         * @param processCrc The initial process memory CRC seed.
         * @param fileCrc The initial file CRC seed.
         */
        CrcPieceCalculator(uint32_t processCrc, uint32_t fileCrc) noexcept;

        /**
         * @brief Gets the next CRC piece and advances the rotation index.
         * @return The calculated byte piece.
         */
        [[nodiscard]] uint8_t GetNextPiece() noexcept;

    private:
        std::array<uint8_t, 8> m_magicCube;
        uint8_t m_index;

        static constexpr std::array<uint8_t, 8> XOR_TABLE = { 
            102, 30, 188, 44, 39, 201, 43, 5 
        };
    };

} // namespace Client::Mimic
