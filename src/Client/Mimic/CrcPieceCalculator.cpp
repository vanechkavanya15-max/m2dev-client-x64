#include "StdAfx.h"
#include "CrcPieceCalculator.h"

namespace Client::Mimic {

    CrcPieceCalculator::CrcPieceCalculator(uint32_t processCrc, uint32_t fileCrc) noexcept
        : m_magicCube{}, m_index{0}
    {
        m_magicCube[0] = static_cast<uint8_t>(processCrc & 0x000000FF);
        m_magicCube[1] = static_cast<uint8_t>(fileCrc & 0x000000FF);
        m_magicCube[2] = static_cast<uint8_t>((processCrc & 0x0000FF00) >> 8);
        m_magicCube[3] = static_cast<uint8_t>((fileCrc & 0x0000FF00) >> 8);
        m_magicCube[4] = static_cast<uint8_t>((processCrc & 0x00FF0000) >> 16);
        m_magicCube[5] = static_cast<uint8_t>((fileCrc & 0x00FF0000) >> 16);
        m_magicCube[6] = static_cast<uint8_t>((processCrc & 0xFF000000) >> 24);
        m_magicCube[7] = static_cast<uint8_t>((fileCrc & 0xFF000000) >> 24);
    }

    [[nodiscard]] uint8_t CrcPieceCalculator::GetNextPiece() noexcept
    {
        uint8_t piece = static_cast<uint8_t>(m_magicCube[m_index] ^ XOR_TABLE[m_index]);
        
        m_index++;
        if ((m_index & 7) == 0)
        {
            m_index = 0;
        }

        return piece;
    }

} // namespace Client::Mimic
