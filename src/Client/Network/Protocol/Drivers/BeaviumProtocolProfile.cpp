#include "BeaviumProtocolProfile.h"

namespace Network::Protocol::Drivers
{
    BeaviumProtocolProfile::BeaviumProtocolProfile()
    {
        // 1. Tabela remapowania naglowkow:
        // rawHeader (Beavium GC) -> unifiedHeader (Standardowy klient Metin2)
        // Dla standardowych opkodow zachowujemy 1:1, a dla przesunietych mapujemy:
        m_remapTable[0x0B] = 0x0B; // DEAD_ENTITY (w Beavium 5B)
        m_remapTable[0x0E] = 0x0E; // DEAD (w Beavium 26B)
        m_remapTable[0x0C] = 0x0C; // ATTACK (12B)
        m_remapTable[0x03] = 0x03; // CHARACTER_MOVE (31B)
        m_remapTable[0x10] = 0x12; // ITEM_SET (w Beavium 0x10 -> unified 0x12)
        m_remapTable[0x6C] = 0x6C; // TARGET_HP (13B)

        // 2. Tabela bezpiecznego odrzucania pakietow customowych (Discard Table)
        // Zapobiega desynchronizacji bufora TCP gdy serwer wysyla pakiety customowych systemow:
        m_discardTable[0x0D] = 3073; // HEADER_GC_GUILD_BEAVIUM
        m_discardTable[0x12] = 281;  // HEADER_GC_ITEM_UPDATE
        m_discardTable[0x1C] = 465;  // HEADER_GC_SHOP_SIGN
        m_discardTable[0x22] = 277;  // HEADER_GC_EXCHANGE_DATA
        m_discardTable[0x23] = 277;  // HEADER_GC_RANKING_DATA
        m_discardTable[0x34] = 3571; // HEADER_GC_PARTY_UPDATE
        m_discardTable[0x45] = 136;  // HEADER_GC_REFINE_INFORMATION (136B = 17 + 40 + 36 + 43)
        m_discardTable[0x46] = 9;    // HEADER_GC_TARGET_BOARD_REFRESH
        m_discardTable[0x47] = 9;    // HEADER_GC_TARGET_HP_REFRESH
        m_discardTable[0x48] = 9;    // HEADER_GC_TARGET_BUFF_REFRESH
        m_discardTable[0x4E] = 38;   // HEADER_GC_LOADING_PHASE_PROGRESS
        m_discardTable[0x55] = 69;   // HEADER_GC_RANDOM_REFINE_INFO
        m_discardTable[0x5D] = 281;  // HEADER_GC_MALL_OPEN
        m_discardTable[0x62] = 2;    // HEADER_GC_TELEPORT_PANEL
        // 0x65 (CHAR_ADDITIONAL_INFO) NIE jest discardowany - zawiera dane text taili i gildii
        m_discardTable[0x66] = 341;  // HEADER_GC_MAIN_CHARACTER4_BGM_VOL
        m_discardTable[0x67] = 25;   // HEADER_GC_TOURNAMENT_DATA
        m_discardTable[0x68] = 80;   // HEADER_GC_MOUNT_NAME
        m_discardTable[0x6B] = 6;    // HEADER_GC_RANKING_ENTRY (disasm 6B)
        m_discardTable[0x6D] = 6;    // HEADER_GC_LOADING_FINISH
        m_discardTable[0x70] = 51;   // HEADER_GC_COMMAND_51
        m_discardTable[0x71] = 101;  // HEADER_GC_COMMAND_101
        m_discardTable[0x72] = 257;  // HEADER_GC_COMMAND_257
        m_discardTable[0x73] = 513;  // HEADER_GC_COMMAND_513
        m_discardTable[0x75] = 1;    // HEADER_GC_SWITCHBOT_STOP
        m_discardTable[0x76] = 133;  // HEADER_GC_SWITCHBOT_STATE
        m_discardTable[0x79] = 281;  // HEADER_GC_PANAMA
        m_discardTable[0x7C] = 70;   // HEADER_GC_OVERHEAD_TITLE
        m_discardTable[0x7D] = 6;    // HEADER_GC_OFFLINE_SHOP_SEARCH
        m_discardTable[0x7E] = 47;   // HEADER_GC_FISHING_GAME_START
        m_discardTable[0x7F] = 13;   // HEADER_GC_FISHING_GAME_VERDICT
        m_discardTable[0x80] = 13;   // HEADER_GC_TARGET_STATE
        m_discardTable[0x83] = 6;    // HEADER_GC_TARGET_LOOT_CLOSE (disasm 6B)
        m_discardTable[0x84] = 33;   // HEADER_GC_CHAR_EFFECT_EXT
        // 0x8A (PET_INFO) jest pakietem dynamicznym i nie nalezy do stalej tabeli discard
        m_discardTable[0x8D] = 170;  // HEADER_GC_VOICE_CHAT
        m_discardTable[0x8E] = 1859; // HEADER_GC_VOICE_DATA_BATCH
        m_discardTable[0x8F] = 1859; // HEADER_GC_VOICE_USER_LIST
        m_discardTable[0x92] = 6635; // HEADER_GC_BLOCKED_PLAYERS_LIST
        m_discardTable[0x93] = 35;   // HEADER_GC_BLOCKED_PLAYER_MODIFY
        m_discardTable[0x94] = 31;   // HEADER_GC_AURA_SYSTEM
        m_discardTable[0x95] = 31;   // HEADER_GC_SASH_ACCE_SYSTEM
        m_discardTable[0x96] = 10;   // HEADER_GC_AUTH_TOKEN
        m_discardTable[0x97] = 8;    // HEADER_GC_CASTING_PROGRESS
        m_discardTable[0x9A] = 2;    // HEADER_GC_SECURITY_CHALLENGE (disasm 2B)
        m_discardTable[0xED] = 602;  // HEADER_GC_AUTH_RSA_KEY
        m_discardTable[0xFA] = 4;    // HEADER_GC_KEY_AGREEMENT_ACK
        m_discardTable[0xFB] = 137;  // HEADER_GC_KEY_AGREEMENT_OFFER
    }

    uint8_t BeaviumProtocolProfile::RemapHeader(uint8_t rawHeader) const
    {
        const auto it = m_remapTable.find(rawHeader);
        if (it != m_remapTable.end())
        {
            return it->second;
        }
        return rawHeader;
    }

    bool BeaviumProtocolProfile::IsDiscardHeader(uint8_t rawHeader) const
    {
        return m_discardTable.find(rawHeader) != m_discardTable.end();
    }

    int BeaviumProtocolProfile::GetDiscardSize(uint8_t rawHeader) const
    {
        const auto it = m_discardTable.find(rawHeader);
        if (it != m_discardTable.end())
        {
            return it->second;
        }
        return 0;
    }
}
