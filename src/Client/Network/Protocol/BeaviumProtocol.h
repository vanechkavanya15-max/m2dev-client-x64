#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace Beavium
{
    // ========================================================================
    // Stale Kryptograficzne Beavium
    // ========================================================================
    static const uint8_t AesKey[32] = {
        0x1a, 0x6a, 0x77, 0x57, 0x40, 0x24, 0x20, 0xc8,
        0x95, 0xf2, 0x94, 0xcd, 0x20, 0x20, 0xfc, 0xdc,
        0x70, 0x2e, 0x67, 0x5d, 0x89, 0x15, 0x29, 0x07,
        0x3d, 0x3d, 0x26, 0x51, 0xe6, 0x4a, 0x36, 0x0b
    };

    static const uint8_t AesIv[16] = {
        0xba, 0x07, 0x6e, 0x7a, 0x53, 0x8d, 0x65, 0x0b,
        0x3d, 0x91, 0x4a, 0xe9, 0x5b, 0x77, 0x2a, 0x02
    };

    // Domyslny klucz klienta
    static const uint8_t DefaultClientKey[16] = {
        0xa1, 0x91, 0x62, 0x53, 0x17, 0xcd, 0x3a, 0xad,
        0x76, 0x1b, 0xc7, 0x0d, 0x1e, 0xde, 0x27, 0x2c
    };

    // ========================================================================
    // Opcody CG (Client -> Game Server)
    // ========================================================================
    namespace CG
    {
        constexpr uint8_t LOGIN            = 0x01; // 53B
        constexpr uint8_t ATTACK           = 0x02; // 8B (type, victimVid, seq)
        constexpr uint8_t CHAT             = 0x03; // Dyn
        constexpr uint8_t CHARACTER_CREATE = 0x04; // 42B
        constexpr uint8_t CHARACTER_DESTROY= 0x05; // 10B
        constexpr uint8_t CHAR_SELECT      = 0x06; // 14B (slot, charId, token)
        constexpr uint8_t MOVE             = 0x07; // 24B (bFunc, wArg, dwRot 1e6, lX, lY, dwTime, dwExtra)
        constexpr uint8_t ENTERGAME        = 0x0A; // 1B
        constexpr uint8_t ITEM_USE         = 0x0C; // 12B (cell, win, vnum, vid)
        constexpr uint8_t ITEM_MOVE        = 0x0D; // 8B
        constexpr uint8_t ITEM_PICKUP      = 0x0E; // 5B (itemVid)
        constexpr uint8_t ON_CLICK         = 0x14; // 6B (targetVid, flag 0x00)
        constexpr uint8_t SCRIPT_ANSWER    = 0x17; // 2B (answer)
        constexpr uint8_t WHISPER          = 0x19; // Dyn
        constexpr uint8_t SHOP             = 0x1B; // 2..7B (sub: 0=END, 1=BUY, 2=SELL)
        constexpr uint8_t ATTACK_ALT       = 0x1D; // 9B (victimVid, mot)
        constexpr uint8_t EXCHANGE         = 0x1F; // 2..6B (sub: 1=START, 6=ACCEPT, 7=EXIT)
        constexpr uint8_t TARGET           = 0x22; // 5B (targetVid)
        constexpr uint8_t SCRIPT_BUTTON    = 0x25; // 5B (questIndex)
        constexpr uint8_t USE_SKILL        = 0x34; // 9B (skillVnum, targetVid)
        constexpr uint8_t QUEST_CONFIRM    = 0x35; // 6B (answer, requestPid)
        constexpr uint8_t SELECT_EMPIRE    = 0x35; // 2B (empireId)
        constexpr uint8_t REFINE           = 0x42; // 6B (pos, type, f1, f2)
        constexpr uint8_t TREASURE_LOOT    = 0x53; // 11B
        constexpr uint8_t TARGET_LOOT      = 0x54; // 11B
        constexpr uint8_t ITEM_USE_EXTEND  = 0x5D; // 11B
        constexpr uint8_t AUTH_LOGIN       = 0x67; // 32B + AES-CFB B64 payload
        constexpr uint8_t LOGIN2           = 0x68; // 56B (name 30B, loginKey 8B, clientKey 16B)
        constexpr uint8_t KEY_AGREEMENT    = 0xFA; // 137B (X25519)
        constexpr uint8_t PONG             = 0xFC; // 1B
        constexpr uint8_t HANDSHAKE_ACK    = 0xFF; // 13B (token, sig1, sig2)
    }

    // ========================================================================
    // Opcody GC (Game Server -> Client)
    // ========================================================================
    namespace GC
    {
        constexpr uint8_t CHARACTER_ADD       = 0x01; // 45B
        constexpr uint8_t CHARACTER_DEL       = 0x02; // 6B
        constexpr uint8_t CHARACTER_MOVE      = 0x03; // 31B
        constexpr uint8_t CHAT                = 0x04; // Dyn
        constexpr uint8_t SYNC_POSITION       = 0x05; // Dyn
        constexpr uint8_t LOGIN_SUCCESS3      = 0x06; // 10B
        constexpr uint8_t LOGIN_FAILURE       = 0x07; // 91B
        constexpr uint8_t CHAR_CREATE_SUCCESS = 0x08; // 2B
        constexpr uint8_t CHAR_CREATE_FAILURE = 0x09; // 1B
        constexpr uint8_t CHAR_DELETE_SUCCESS = 0x0A; // 1B
        constexpr uint8_t CHAR_DELETE_WRONG_PIN = 0x0B; // 5B
        constexpr uint8_t ATTACK              = 0x0C; // 12B
        constexpr uint8_t DEAD                = 0x0E; // 26B
        constexpr uint8_t MAIN_CHARACTER      = 0x0F; // Dyn
        constexpr uint8_t CHARACTER_UPDATE    = 0x10; // 82B
        constexpr uint8_t ITEM_USE            = 0x11; // 4B
        constexpr uint8_t ITEM_UPDATE         = 0x12; // 281B
        constexpr uint8_t ITEM_GROUND_ADD     = 0x13; // 16B
        constexpr uint8_t QUICKSLOT_ADD       = 0x15; // 268B
        constexpr uint8_t QUICKSLOT_DEL       = 0x16; // 26B
        constexpr uint8_t QUICKSLOT_SWAP      = 0x17; // 5B
        constexpr uint8_t ITEM_OWNERSHIP      = 0x18; // 4B
        constexpr uint8_t WHISPER             = 0x19; // 2B
        constexpr uint8_t MOTION              = 0x1A; // 3B
        constexpr uint8_t SHOP                = 0x1B; // 38B
        constexpr uint8_t CHAR_LIST           = 0x1C; // 465B
        constexpr uint8_t DUEL_START          = 0x1D; // 37B
        constexpr uint8_t PVP                 = 0x1E; // 11B
        constexpr uint8_t EXCHANGE            = 0x1F; // Dyn
        constexpr uint8_t CHARACTER_POINTS    = 0x20; // 70B
        constexpr uint8_t POINT_CHANGE        = 0x21; // Dyn
        constexpr uint8_t CHANGE_SPEED        = 0x22; // 10B
        constexpr uint8_t HANDSHAKE           = 0x24; // 6B
        constexpr uint8_t SCRIPT              = 0x25; // Dyn
        constexpr uint8_t HANDSHAKE_OK        = 0x26; // Dyn
        constexpr uint8_t OWNERSHIP           = 0x27; // 74B
        constexpr uint8_t FLY_TARGETING       = 0x2C; // 22B
        constexpr uint8_t ADD_FLY_TARGETING   = 0x2D; // 32B
        constexpr uint8_t SKILL_LEVEL         = 0x2E; // 17B
        constexpr uint8_t SKILL_COOLTIME_END  = 0x2F; // 10B
        constexpr uint8_t MESSENGER           = 0x30; // 17B
        constexpr uint8_t GUILD               = 0x31; // 2B
        constexpr uint8_t PARTY_INVITE        = 0x32; // Dyn
        constexpr uint8_t PARTY_ADD           = 0x33; // Dyn
        constexpr uint8_t PARTY_UPDATE        = 0x34; // 3571B
        constexpr uint8_t PARTY_REMOVE        = 0x35; // 5B
        constexpr uint8_t QUEST_INFO          = 0x36; // 45B
        constexpr uint8_t REQUEST_MAKE_GUILD  = 0x37; // 29B
        constexpr uint8_t PARTY_PARAMETER     = 0x38; // 6B
        constexpr uint8_t SAFEBOX_SET         = 0x39; // 106B
        constexpr uint8_t SAFEBOX_SIZE        = 0x3C; // 5B
        constexpr uint8_t WALK_MODE           = 0x40; // 2B
        constexpr uint8_t CHANGE_SKILL_GROUP  = 0x41; // 8B
        constexpr uint8_t REFINE_INFORMATION  = 0x42; // 2B
        constexpr uint8_t SPECIAL_EFFECT      = 0x43; // 9B
        constexpr uint8_t NPC_POSITION        = 0x44; // 9B
        constexpr uint8_t VIEW_EQUIP          = 0x50; // 6B
        constexpr uint8_t AFFECT_ADD          = 0x57; // 2B
        constexpr uint8_t AFFECT_REMOVE       = 0x58; // 17B
        constexpr uint8_t MAIN_CHARACTER_BGM  = 0x66; // 341B
        constexpr uint8_t CHAR_ADDITIONAL_INFO = 0x65; // 129B
        constexpr uint8_t TARGET_HP           = 0x6C; // 13B
        constexpr uint8_t DRAGON_SOUL_REFINE  = 0x6E; // 37B
        constexpr uint8_t AUTH_SUCCESS        = 0x78; // Dyn
        constexpr uint8_t AUTH_KEY            = 0x96; // 10B (loginKey64)
        constexpr uint8_t KEY_AGREEMENT_ACK   = 0xFA; // 4B
        constexpr uint8_t KEY_AGREEMENT_OFFER = 0xFB; // 137B
        constexpr uint8_t KEY_AGREEMENT_INIT  = 0xFC; // 5B
        constexpr uint8_t PHASE               = 0xFD; // 2B (0xFD, phase)
        constexpr uint8_t TIME_SYNC           = 0xFF; // 13B (token, srvTime, delta)
    }

    // ========================================================================
    // Struktury Ramkowe Beavium x64 (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    struct TPacketCGMoveBeavium
    {
        uint8_t  header;    // 0x07
        uint8_t  bFunc;     // 0=WAIT, 1=MOVE, 2=ROTATE, 3=COMBO
        uint16_t wArg;      // combo index (14..17)
        uint32_t dwRot;     // rotation * 1,000,000.0f
        int32_t  lX;        // Global X
        int32_t  lY;        // Global Y
        uint32_t dwTime;    // Client tick
        uint32_t dwExtra;   // 0
    };

    struct TPacketCGAttackBeavium
    {
        uint8_t  header;        // 0x02
        uint8_t  attackType;    // 0x00
        uint32_t dwVictimVID;   // Target VID
        uint16_t wSeq;          // CRC / sequence
    };
    static_assert(sizeof(TPacketCGAttackBeavium) == 8, "TPacketCGAttackBeavium must be 8 bytes");

    struct TPacketCGAttackAltBeavium
    {
        uint8_t  header;        // 0x1D
        uint32_t dwVictimVID;   // Target VID
        uint32_t uMotAttack;    // motion/combo index
    };
    static_assert(sizeof(TPacketCGAttackAltBeavium) == 9, "TPacketCGAttackAltBeavium must be 9 bytes");

    struct TPacketCGHandshakeAckBeavium
    {
        uint8_t  header;    // 0xFF
        uint32_t token;
        uint32_t sig1;
        uint32_t sig2;
    };

    struct TPacketCGLogin2Beavium
    {
        uint8_t  header;        // 0x68
        char     username[30];
        uint8_t  nullByte;      // 0x00
        uint64_t loginKey;      // 8B uint64
        uint8_t  clientKey[16];
    };

    struct TPacketCGCharSelectBeavium
    {
        uint8_t  header;        // 0x06
        uint8_t  slot;
        uint32_t charId;
        uint64_t securityToken;
    };

    struct TPacketCGEnterGameBeavium
    {
        uint8_t header; // 0x0A
    };

    struct TPacketCGItemUseBeavium
    {
        uint8_t  header;    // 0x0C
        uint16_t cell;
        uint8_t  window;
        uint32_t vnum;
        uint32_t targetVid;
    };
    static_assert(sizeof(TPacketCGItemUseBeavium) == 12, "TPacketCGItemUseBeavium must be 12 bytes");

    struct TPacketCGItemUseStandardBeavium
    {
        uint8_t  header;    // 0x11
        uint8_t  window;
        uint16_t cell;
    };
    static_assert(sizeof(TPacketCGItemUseStandardBeavium) == 4, "TPacketCGItemUseStandardBeavium must be 4 bytes");

    struct TPacketCGItemPickupBeavium
    {
        uint8_t  header;    // 0x0E
        uint32_t itemVid;
    };

    struct TPacketCGOnClickBeavium
    {
        uint8_t  header;    // 0x14
        uint32_t targetVid;
        uint8_t  flag;      // 0x00
    };

    struct TPacketCGScriptAnswerBeavium
    {
        uint8_t header; // 0x17
        uint8_t answer;
    };

    struct TPacketCGScriptButtonBeavium
    {
        uint8_t  header;    // 0x25
        uint32_t questIndex;
    };

    struct TPacketCGTargetBeavium
    {
        uint8_t  header;    // 0x22
        uint32_t targetVid;
    };

    struct TPacketCGUseSkillBeavium
    {
        uint8_t  header;    // 0x34
        uint32_t skillVnum;
        uint32_t targetVid;
    };

    struct TPacketCGQuestConfirmBeavium
    {
        uint8_t  header;    // 0x35
        uint8_t  answer;
        uint32_t requestPid;
    };

    struct TPacketCGRefineBeavium
    {
        uint8_t  header;    // 0x42
        uint16_t itemPos;
        uint8_t  refineType;
        uint8_t  flag1;
        uint8_t  flag2;
    };

    struct TPacketCGCharacterCreateBeavium
    {
        uint8_t  header;       // 0x04
        uint8_t  slot;         // 0..4
        char     name[33];     // char[33]
        uint16_t job;          // ushort LE
        uint8_t  shape;        // 1B
        uint8_t  str;          // 1B
        uint8_t  dex;          // 1B
        uint8_t  intelligence; // 1B
        uint8_t  con;          // 1B
    };

    struct TPacketCGCharacterDestroyBeavium
    {
        uint8_t header;        // 0x05
        uint8_t slot;          // 0..4
        char    privateCode[8];// char[8]
    };

    struct TPacketCGShopBuyBeavium
    {
        uint8_t  header;    // 0x1B
        uint8_t  subHeader; // 0x01 (BUY)
        uint8_t  count;     // count (1B)
        uint16_t pos;       // pos (2B LE)
        uint16_t tab;       // tab/slot (2B LE)
    };

    struct TPacketCGShopSellBeavium
    {
        uint8_t  header;    // 0x1B
        uint8_t  subHeader; // 0x02 (SELL)
        uint8_t  windowType;// 1B
        uint16_t cell;      // 2B LE
    };

    struct TPacketCGShopEndBeavium
    {
        uint8_t header;    // 0x1B
        uint8_t subHeader; // 0x00 (END)
    };

    struct TPacketCGSelectEmpireBeavium
    {
        uint8_t header;   // 0x35
        uint8_t empireId; // 1B (1=Shinsoo, 2=Chunjo, 3=Jinno)
    };

    struct TPacketCGPongBeavium
    {
        uint8_t header; // 0xFC
    };

    // Pakiety GC
    struct TPacketGCPhaseBeavium
    {
        uint8_t header; // 0xFD
        uint8_t phase;  // 0=CLOSE, 1=HANDSHAKE, 2=LOGIN, 3=SELECT, 4=LOADING, 5=GAME
    };

    struct TPacketGCHandshakeBeavium
    {
        uint8_t  header;    // 0xFF
        uint32_t token;
        uint32_t srvTime;
        uint32_t delta;
    };

    struct TPacketGCCharacterAddBeavium
    {
        uint8_t  header;        // 0x01
        uint32_t dwVID;         // 4B LE
        float    fAngle;        // 4B
        int32_t  lX;            // 4B LE
        int32_t  lY;            // 4B LE
        int32_t  lZ;            // 4B LE
        uint8_t  bType;         // 1B
        uint16_t wRaceNum;      // 2B LE
        uint8_t  bMovingSpeed;  // 1B
        uint8_t  bAttackSpeed;  // 1B
        uint8_t  bStateFlag;    // 1B
        uint32_t dwAffectFlag[2]; // 8B
        uint8_t  reserved[10];  // 10B
    };
    static_assert(sizeof(TPacketGCCharacterAddBeavium) == 45, "TPacketGCCharacterAddBeavium must be 45 bytes");

    struct TPacketGCCharacterDelBeavium
    {
        uint8_t  header;    // 0x02
        uint32_t dwVID;     // 4B LE
        uint8_t  bReserved; // 1B
    };
    static_assert(sizeof(TPacketGCCharacterDelBeavium) == 6, "TPacketGCCharacterDelBeavium must be 6 bytes");

    struct TPacketGCCharacterMoveBeavium
    {
        uint8_t  header;        // 0x03
        uint8_t  bFunc;         // 1B
        uint8_t  bArg;          // 1B
        uint32_t dwVID;         // 4B LE
        uint32_t dwRot;         // 4B LE
        int32_t  lX;            // 4B LE
        int32_t  lY;            // 4B LE
        uint32_t dwTime;        // 4B LE
        uint32_t dwDuration;    // 4B LE
        uint32_t dwReserved;    // 4B LE
    };
    static_assert(sizeof(TPacketGCCharacterMoveBeavium) == 31, "TPacketGCCharacterMoveBeavium must be 31 bytes");

    struct TPacketGCChatBeavium
    {
        uint8_t  header;      // 0x04
        uint16_t size;        // 2B LE
        uint8_t  type;        // 1B
        uint32_t id;          // 4B LE (dwVID)
        uint8_t  bEmpire;     // 1B
        uint8_t  reserved[5]; // 5B
    };
    static_assert(sizeof(TPacketGCChatBeavium) == 14, "TPacketGCChatBeavium must be 14 bytes");

    struct TPacketGCAttackBeavium
    {
        uint8_t  header;      // 0x0C
        uint32_t dwVID;       // 4B LE
        uint32_t dwVictimVID; // 4B LE
        uint8_t  bType;       // 1B
        uint16_t wMotionArg;  // 2B LE
    };
    static_assert(sizeof(TPacketGCAttackBeavium) == 12, "TPacketGCAttackBeavium must be 12 bytes");

    struct TPacketGCDeadBeavium
    {
        uint8_t  header;    // 0x0E
        uint32_t dwVID;
        uint8_t  bReserved[21];
    };
    static_assert(sizeof(TPacketGCDeadBeavium) == 26, "TPacketGCDeadBeavium must be 26 bytes");

    struct TPacketGCItemSetBeavium
    {
        uint8_t  header;        // 0x10
        uint8_t  window_type;   // 1B
        uint16_t Cell;          // 2B LE
        uint32_t vnum;          // 4B LE
        uint32_t count;         // 4B LE
        uint32_t flags;         // 4B LE
        uint32_t anti_flags;    // 4B LE
        int32_t  alSockets[6];  // 24B
        int16_t  aAttr[7][2];   // 28B
        uint8_t  reserved[10];  // 10B
    };
    static_assert(sizeof(TPacketGCItemSetBeavium) == 82, "TPacketGCItemSetBeavium must be 82 bytes");

    struct TPacketGCItemUpdateBeavium
    {
        uint8_t  header;        // 0x12
        uint8_t  window_type;   // 1B
        uint16_t Cell;          // 2B LE
        uint32_t count;         // 4B LE
        int32_t  alSockets[6];  // 24B
        int16_t  aAttr[7][2];   // 28B
        uint8_t  extendedData[221]; // 221B
    };
    static_assert(sizeof(TPacketGCItemUpdateBeavium) == 281, "TPacketGCItemUpdateBeavium must be 281 bytes");

    struct TPacketGCItemGroundAddBeavium
    {
        uint8_t  header; // 0x13
        int32_t  lX;     // 4B LE
        int32_t  lY;     // 4B LE
        int32_t  lZ;     // 4B LE
        uint32_t dwVID;  // 4B LE
    };
    static_assert(sizeof(TPacketGCItemGroundAddBeavium) == 17, "TPacketGCItemGroundAddBeavium must be 17 bytes");

    struct TPacketGCScriptBeavium
    {
        uint8_t  header;   // 0x25
        uint16_t size;     // 2B LE
        uint8_t  skin;     // 1B
        uint16_t src_size; // 2B LE
    };
    static_assert(sizeof(TPacketGCScriptBeavium) == 6, "TPacketGCScriptBeavium must be 6 bytes");

    struct TPacketGCCharAdditionalInfoBeavium
    {
        uint8_t  header;            // 0x65
        uint32_t dwVID;             // 4B LE
        char     szName[24];        // 24B
        uint8_t  bEmpire;           // 1B
        uint32_t dwGuildID;         // 4B LE
        int16_t  sAlignment;        // 2B LE
        uint8_t  bPKMode;           // 1B
        uint32_t dwMountVnum;       // 4B LE
        uint8_t  extendedInfo[88];  // 88B
    };
    static_assert(sizeof(TPacketGCCharAdditionalInfoBeavium) == 129, "TPacketGCCharAdditionalInfoBeavium must be 129 bytes");

    struct TPacketGCTargetHPBeavium
    {
        uint8_t  header;        // 0x6C
        uint32_t dwTargetVID;   // 4B LE
        int64_t  llCurrentHP;   // 8B LE
    };
    static_assert(sizeof(TPacketGCTargetHPBeavium) == 13, "TPacketGCTargetHPBeavium must be 13 bytes");

    struct TPacketGCCharCreateSuccessBeavium
    {
        uint8_t header; // 0x08
        uint8_t slot;   // 1B
    };

    struct TPacketGCCharCreateFailureBeavium
    {
        uint8_t header; // 0x09
    };

    struct TPacketGCCharDeleteSuccessBeavium
    {
        uint8_t header; // 0x0A
    };

    struct TPacketGCCharDeleteWrongPinBeavium
    {
        uint8_t  header; // 0x0B
        uint32_t data;   // 4B
    };

#pragma pack(pop)

    // ========================================================================
    // Tabela Rozmiarow Pakietow GC w Beavium (146 zarejestrowanych pakietow)
    // ========================================================================
    struct BeaviumGCInfo
    {
        uint32_t size;
        bool isDynamic;
    };

    inline bool GetGCInfo(uint8_t header, BeaviumGCInfo& info)
    {
        switch (header)
        {
#include "beavium_gc_table.inl"
        default:
            return false;
        }
    }

    // ========================================================================
    // Algorytmy Matematyczne i Haszujace Beavium
    // ========================================================================
    inline uint32_t RotateLeft32(uint32_t val, int count)
    {
        return (val << count) | (val >> (32 - count));
    }

    inline uint32_t CalculateMurmur3(const uint8_t* data, size_t length, uint32_t seed)
    {
        size_t ptr = 0;
        uint32_t eax;
        uint32_t rcx;

        if (length >= 16)
        {
            uint32_t r10 = seed + 0x24234428;
            uint32_t r11 = seed - 0x7A143589;
            rcx = seed + 0x61C8864F;
            uint32_t r8 = seed;
            size_t limit = length - 15;

            while (ptr < limit)
            {
                uint32_t v0 = *(const uint32_t*)(data + ptr);
                uint32_t v1 = *(const uint32_t*)(data + ptr + 4);
                uint32_t v2 = *(const uint32_t*)(data + ptr + 8);
                uint32_t v3 = *(const uint32_t*)(data + ptr + 12);

                uint32_t eax0 = v0 * 0x7A143589;
                r10 -= eax0;

                uint32_t eax1 = v1 * 0x7A143589;
                r10 = RotateLeft32(r10, 13) * 0x9E3779B1;
                r11 -= eax1;

                uint32_t eax2 = v2 * 0x7A143589;
                r11 = RotateLeft32(r11, 13) * 0x9E3779B1;
                r8 -= eax2;

                uint32_t eax3 = v3 * 0x7A143589;
                r8 = RotateLeft32(r8, 13);
                ptr += 16;
                r8 *= 0x9E3779B1;
                rcx = RotateLeft32(rcx - eax3, 13) * 0x9E3779B1;
            }

            rcx = RotateLeft32(rcx, 18);
            r8 = RotateLeft32(r8, 12);
            rcx += r8;
            r11 = RotateLeft32(r11, 7);
            rcx += r11;
            r10 = RotateLeft32(r10, 1);
            rcx += r10;
        }
        else
        {
            rcx = seed + 0x165667B1;
        }

        size_t remLen = length & 0x0F;
        eax = (uint32_t)length + rcx;

        if ((length & 0x0C) != 0)
        {
            while (remLen >= 4)
            {
                uint32_t v = *(const uint32_t*)(data + ptr);
                uint32_t c = v * 0x3D4D51C3;
                remLen -= 4;
                ptr += 4;
                eax = RotateLeft32(eax - c, 17) * 0x27D4EB2F;
            }
        }

        while (remLen > 0)
        {
            uint8_t b = data[ptr++];
            uint32_t c = (uint32_t)b * 0x165667B1 + eax;
            c = RotateLeft32(c, 11);
            eax = c * 0x9E3779B1;
            remLen--;
        }

        uint32_t ecxFin = (eax >> 15) ^ eax;
        eax = ecxFin * 0x85EBCA77;
        ecxFin = (eax >> 13) ^ eax;
        uint32_t edxFin = ecxFin * 0xC2B2AE3D;
        eax = (edxFin >> 16) ^ edxFin;
        return eax;
    }

    inline void CalculateHandshakeSignatures(uint32_t token, uint32_t srvTime, uint32_t& sig1, uint32_t& sig2)
    {
        sig1 = srvTime ^ 0x2710BB96;
        sig2 = ((token * 0x9E3779B1) ^ 0x58D2FC1E);
    }

    inline uint32_t EncodeRotationMicrodegrees(float degrees)
    {
        float norm = fmodf(degrees, 360.0f);
        if (norm < 0.0f) norm += 360.0f;
        return (uint32_t)(norm * 1000000.0f + 0.5f);
    }

} // namespace Beavium

