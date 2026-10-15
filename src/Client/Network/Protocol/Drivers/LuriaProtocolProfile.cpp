#include "LuriaProtocolProfile.h"

namespace Network::Protocol::Drivers
{
    LuriaProtocolProfile::LuriaProtocolProfile()
    {
        // 1. Tabela remapowania naglowkow Lurii do standardowych kodow klienta:
        m_remapTable[0x01] = 0x01; // CHARACTER_ADD
        m_remapTable[0x02] = 0x02; // CHARACTER_DEL
        m_remapTable[0x03] = 0x03; // CHARACTER_MOVE
        m_remapTable[0x0C] = 0x0C; // ATTACK
        m_remapTable[0x0E] = 0x0E; // DEAD
        m_remapTable[0x14] = 0x12; // ITEM_SET (Luria 0x14 -> unified 0x12)
        m_remapTable[0x6C] = 0x6C; // TARGET_HP
        m_remapTable[0x87] = 0x10; // DAMAGE_INFO (Luria 0x87 -> unified 0x10)

        // 2. Tabela bezpiecznego odrzucania customowych/rozszerzonych pakietow serwera Luria:
        m_discardTable[0xF5] = 513;  // HEADER_GC_LURIA_F5
        m_discardTable[0xD8] = 9;    // HEADER_GC_LURIA_D8
        m_discardTable[0x1C] = 4;    // HEADER_GC_LURIA_1C (ping/time sync)
        m_discardTable[0x11] = 17;   // POINT_CHANGE (gdy trafia do PhaseGamePacketDispatcher zamiast CPythonNetworkStream)
        m_discardTable[0x10] = 1021; // POINTS snapshot
        m_discardTable[0x4C] = 1531; // SKILL_LEVEL_MATRIX
        m_discardTable[0x15] = 64;   // QUICKSLOTS
        m_discardTable[0x16] = 16;   // QUICKSLOT_MOVE
        m_discardTable[0x26] = 4;    // SCRIPT_PACKET
    }

    uint8_t LuriaProtocolProfile::RemapHeader(uint8_t rawHeader) const
    {
        const auto it = m_remapTable.find(rawHeader);
        if (it != m_remapTable.end())
        {
            return it->second;
        }
        return rawHeader;
    }

    bool LuriaProtocolProfile::IsDiscardHeader(uint8_t rawHeader) const
    {
        return m_discardTable.find(rawHeader) != m_discardTable.end();
    }

    int LuriaProtocolProfile::GetDiscardSize(uint8_t rawHeader) const
    {
        const auto it = m_discardTable.find(rawHeader);
        if (it != m_discardTable.end())
        {
            return it->second;
        }
        return 0;
    }
}
