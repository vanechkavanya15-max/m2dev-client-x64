#pragma once

// ============================================================================
// AUTOGEN PROTOCOL SPECIFICATION - MartySama (Ymir Mainline Standard)
// Single Source of Truth: docs/AI_PROTOCOL_MAP.json
// Standard: C++23 | Zero Padding | static_assert verified
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace Protocol::Generated::MartySama
{
    // ========================================================================
    // Opcody CG (Client -> Game Server)
    // ========================================================================
    namespace CG
    {
        constexpr uint8_t LOGIN                    = 0x01; // Logowanie standardowe (49B)
        constexpr uint8_t ATTACK                   = 0x02; // Atak 8B: opcode 2, bType, dwVictimVID, CRC
        constexpr uint8_t CHAT                     = 0x03; // Dynamiczny czat
        constexpr uint8_t CHARACTER_CREATE         = 0x04; // Tworzenie postaci (34B)
        constexpr uint8_t CHARACTER_DELETE         = 0x05; // Usuniecie postaci kodem PIN (10B)
        constexpr uint8_t CHARACTER_SELECT         = 0x06; // Wybor postaci ze slotu (2B)
        constexpr uint8_t MOVE                     = 0x07; // Ruch 16B: opcode 7, bFunc, bArg, bRot, lX, lY, dwTime
        constexpr uint8_t ENTERGAME                = 0x0A; // Wejscie do gry (1B)
        constexpr uint8_t ITEM_USE                 = 0x0B; // Uzycie przedmiotu 4B: opcode 11, window, cell
        constexpr uint8_t ITEM_DROP                = 0x0C; // Wyrzucenie przedmiotu (9B)
        constexpr uint8_t ITEM_MOVE                = 0x0D; // Przeniesienie w ekwipunku (9B)
        constexpr uint8_t ITEM_PICKUP              = 0x0F; // Podniesienie przedmiotu (5B)
        constexpr uint8_t TARGET                   = 0x3D; // Wybor celu 5B: opcode 61, targetVid
        constexpr uint8_t EMPIRE                   = 0x5A; // Wybor krolestwa (2B)
        constexpr uint8_t LOGIN2                   = 0x6D; // Logowanie do gry (52B)
        constexpr uint8_t PONG                     = 0xFE; // Odpowiedz keepalive (1B)
        constexpr uint8_t HANDSHAKE                = 0xFF; // Handshake (13B)
    }

    // ========================================================================
    // Opcody GC (Game Server -> Client)
    // ========================================================================
    namespace GC
    {
        constexpr uint8_t CHARACTER_ADD            = 0x01;
        constexpr uint8_t CHARACTER_DEL            = 0x02;
        constexpr uint8_t MOVE                     = 0x03;
        constexpr uint8_t CHAT                     = 0x04;
        constexpr uint8_t SYNC_POSITION            = 0x05;
        constexpr uint8_t LOGIN_SUCCESS            = 0x06;
        constexpr uint8_t LOGIN_FAILURE            = 0x07;
        constexpr uint8_t CHARACTER_CREATE_SUCCESS = 0x08;
        constexpr uint8_t CHARACTER_CREATE_FAILURE = 0x09;
        constexpr uint8_t CHARACTER_DELETE_SUCCESS = 0x0A;
        constexpr uint8_t CHARACTER_DELETE_WRONG_SOCIAL_ID = 0x0B;
        constexpr uint8_t ATTACK                   = 0x0C;
        constexpr uint8_t STUN                     = 0x0D;
        constexpr uint8_t DEAD                     = 0x0E;
        constexpr uint8_t MAIN_CHARACTER           = 0x0F;
        constexpr uint8_t CHARACTER_POINTS         = 0x10;
        constexpr uint8_t CHARACTER_POINT_CHANGE   = 0x11;
        constexpr uint8_t CHANGE_SPEED             = 0x12;
        constexpr uint8_t CHARACTER_UPDATE         = 0x13;
        constexpr uint8_t ITEM_DEL                 = 0x14;
        constexpr uint8_t ITEM_SET                 = 0x15;
        constexpr uint8_t ITEM_USE                 = 0x16;
        constexpr uint8_t ITEM_DROP                = 0x17;
        constexpr uint8_t CHARACTER_UPDATE_NEW     = 0x18;
        constexpr uint8_t ITEM_UPDATE              = 0x19;
        constexpr uint8_t ITEM_GROUND_ADD          = 0x1A;
        constexpr uint8_t ITEM_GROUND_DEL          = 0x1B;
        constexpr uint8_t QUICKSLOT_ADD            = 0x1C;
        constexpr uint8_t QUICKSLOT_DEL            = 0x1D;
        constexpr uint8_t QUICKSLOT_SWAP           = 0x1E;
        constexpr uint8_t ITEM_OWNERSHIP           = 0x1F;
        constexpr uint8_t WHISPER                  = 0x22;
        constexpr uint8_t MOTION                   = 0x24;
        constexpr uint8_t PARTS                    = 0x25;
        constexpr uint8_t SHOP                     = 0x26;
        constexpr uint8_t SHOP_SIGN                = 0x27;
        constexpr uint8_t DUEL_START               = 0x28;
        constexpr uint8_t PVP                      = 0x29;
        constexpr uint8_t EXCHANGE                 = 0x2A;
        constexpr uint8_t CHARACTER_POSITION       = 0x2B;
        constexpr uint8_t PING                     = 0x2C;
        constexpr uint8_t SCRIPT                   = 0x2D;
        constexpr uint8_t QUEST_CONFIRM            = 0x2E;
        constexpr uint8_t MOUNT                    = 0x3D;
        constexpr uint8_t OWNERSHIP                = 0x3E;
        constexpr uint8_t TARGET                   = 0x3F;
        constexpr uint8_t COMMAND                  = 0x40;
        constexpr uint8_t WARP                     = 0x41;
        constexpr uint8_t ADD_FLY_TARGETING        = 0x45;
        constexpr uint8_t CREATE_FLY               = 0x46;
        constexpr uint8_t FLY_TARGETING            = 0x47;
        constexpr uint8_t SKILL_LEVEL              = 0x4C;
        constexpr uint8_t QUEST_INFO               = 0x51;
        constexpr uint8_t SAFEBOX_SET              = 0x55;
        constexpr uint8_t SAFEBOX_DEL              = 0x56;
        constexpr uint8_t SAFEBOX_WRONG_PASSWORD   = 0x57;
        constexpr uint8_t SAFEBOX_SIZE             = 0x58;
        constexpr uint8_t MAIN_CHARACTER2          = 0x71;
        constexpr uint8_t CHANNEL                  = 0x79;
        constexpr uint8_t AFFECT_ADD               = 0x7E;
        constexpr uint8_t AFFECT_REMOVE            = 0x7F;
        constexpr uint8_t DAMAGE_INFO              = 0x87;
        constexpr uint8_t PANAMA_PACK              = 0x97;
        constexpr uint8_t RESPOND_CHANNELSTATUS    = 0xD2;
        constexpr uint8_t TIME_SYNC                = 0xFC;
        constexpr uint8_t PHASE                    = 0xFD;
        constexpr uint8_t BINDUDP                  = 0xFE;
        constexpr uint8_t HANDSHAKE                = 0xFF;
    }

    // ========================================================================
    // Struktury Pakietow (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    // --- Pakiety CG (Client -> Game) ---
    struct TPacketCGLoginMartySama
    {
        uint8_t header{ CG::LOGIN };
        uint8_t payload[48]{};
    };
    static_assert(sizeof(TPacketCGLoginMartySama) == 49, "TPacketCGLoginMartySama rozmiar musi wynosic 49 B");

    struct TPacketCGAttackMarty
    {
        uint8_t header{ CG::ATTACK };
        uint8_t bType{};
        uint32_t dwVictimVID{};
        uint8_t bCRCMagicCubeProcPiece{};
        uint8_t bCRCMagicCubeFilePiece{};
    };
    static_assert(sizeof(TPacketCGAttackMarty) == 8, "TPacketCGAttackMarty rozmiar musi wynosic 8 B");

    struct TPacketCGChatMartySama
    {
        uint8_t header{ CG::CHAT };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketCGChatMartySama) == 3, "TPacketCGChatMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketCGCharacterCreateMartySama
    {
        uint8_t header{ CG::CHARACTER_CREATE };
        uint8_t payload[33]{};
    };
    static_assert(sizeof(TPacketCGCharacterCreateMartySama) == 34, "TPacketCGCharacterCreateMartySama rozmiar musi wynosic 34 B");

    struct TPacketCGCharacterDeleteMartySama
    {
        uint8_t header{ CG::CHARACTER_DELETE };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketCGCharacterDeleteMartySama) == 10, "TPacketCGCharacterDeleteMartySama rozmiar musi wynosic 10 B");

    struct TPacketCGCharacterSelectMartySama
    {
        uint8_t header{ CG::CHARACTER_SELECT };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketCGCharacterSelectMartySama) == 2, "TPacketCGCharacterSelectMartySama rozmiar musi wynosic 2 B");

    struct TPacketCGMoveMarty
    {
        uint8_t header{ CG::MOVE };
        uint8_t bFunc{};
        uint8_t bArg{};
        uint8_t bRot{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
    };
    static_assert(sizeof(TPacketCGMoveMarty) == 16, "TPacketCGMoveMarty rozmiar musi wynosic 16 B");

    struct TPacketCGEntergameMartySama
    {
        uint8_t header{ CG::ENTERGAME };
    };
    static_assert(sizeof(TPacketCGEntergameMartySama) == 1, "TPacketCGEntergameMartySama rozmiar musi wynosic 1 B");

    struct TPacketCGItemUseMarty
    {
        uint8_t header{ CG::ITEM_USE };
        uint8_t window_type{};
        uint16_t cell{};
    };
    static_assert(sizeof(TPacketCGItemUseMarty) == 4, "TPacketCGItemUseMarty rozmiar musi wynosic 4 B");

    struct TPacketCGItemDropMartySama
    {
        uint8_t header{ CG::ITEM_DROP };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketCGItemDropMartySama) == 9, "TPacketCGItemDropMartySama rozmiar musi wynosic 9 B");

    struct TPacketCGItemMoveMartySama
    {
        uint8_t header{ CG::ITEM_MOVE };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketCGItemMoveMartySama) == 9, "TPacketCGItemMoveMartySama rozmiar musi wynosic 9 B");

    struct TPacketCGItemPickupMartySama
    {
        uint8_t header{ CG::ITEM_PICKUP };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketCGItemPickupMartySama) == 5, "TPacketCGItemPickupMartySama rozmiar musi wynosic 5 B");

    struct TPacketCGTargetMarty
    {
        uint8_t header{ CG::TARGET };
        uint32_t targetVid{};
    };
    static_assert(sizeof(TPacketCGTargetMarty) == 5, "TPacketCGTargetMarty rozmiar musi wynosic 5 B");

    struct TPacketCGEmpireMartySama
    {
        uint8_t header{ CG::EMPIRE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketCGEmpireMartySama) == 2, "TPacketCGEmpireMartySama rozmiar musi wynosic 2 B");

    struct TPacketCGLogin2MartySama
    {
        uint8_t header{ CG::LOGIN2 };
        uint8_t payload[51]{};
    };
    static_assert(sizeof(TPacketCGLogin2MartySama) == 52, "TPacketCGLogin2MartySama rozmiar musi wynosic 52 B");

    struct TPacketCGPongMartySama
    {
        uint8_t header{ CG::PONG };
    };
    static_assert(sizeof(TPacketCGPongMartySama) == 1, "TPacketCGPongMartySama rozmiar musi wynosic 1 B");

    struct TPacketCGHandshakeMartySama
    {
        uint8_t header{ CG::HANDSHAKE };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketCGHandshakeMartySama) == 13, "TPacketCGHandshakeMartySama rozmiar musi wynosic 13 B");

    // --- Pakiety GC (Game -> Client) ---
    struct TPacketGCCharacterAddMartySama
    {
        uint8_t header{ GC::CHARACTER_ADD };
        uint8_t payload[38]{};
    };
    static_assert(sizeof(TPacketGCCharacterAddMartySama) == 39, "TPacketGCCharacterAddMartySama rozmiar musi wynosic 39 B");

    struct TPacketGCCharacterDelMartySama
    {
        uint8_t header{ GC::CHARACTER_DEL };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCCharacterDelMartySama) == 5, "TPacketGCCharacterDelMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCMoveMarty
    {
        uint8_t header{ GC::MOVE };
        uint8_t bFunc{};
        uint8_t bArg{};
        uint8_t bRot{};
        uint32_t dwVID{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
        uint32_t dwDuration{};
    };
    static_assert(sizeof(TPacketGCMoveMarty) == 24, "TPacketGCMoveMarty rozmiar musi wynosic 24 B");

    struct TPacketGCChatMartySama
    {
        uint8_t header{ GC::CHAT };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCChatMartySama) == 3, "TPacketGCChatMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCSyncPositionMartySama
    {
        uint8_t header{ GC::SYNC_POSITION };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCSyncPositionMartySama) == 9, "TPacketGCSyncPositionMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCLoginSuccessMartySama
    {
        uint8_t header{ GC::LOGIN_SUCCESS };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCLoginSuccessMartySama) == 10, "TPacketGCLoginSuccessMartySama rozmiar musi wynosic 10 B");

    struct TPacketGCLoginFailureMartySama
    {
        uint8_t header{ GC::LOGIN_FAILURE };
        uint8_t payload[90]{};
    };
    static_assert(sizeof(TPacketGCLoginFailureMartySama) == 91, "TPacketGCLoginFailureMartySama rozmiar musi wynosic 91 B");

    struct TPacketGCCharacterCreateSuccessMartySama
    {
        uint8_t header{ GC::CHARACTER_CREATE_SUCCESS };
        uint8_t payload[42]{};
    };
    static_assert(sizeof(TPacketGCCharacterCreateSuccessMartySama) == 43, "TPacketGCCharacterCreateSuccessMartySama rozmiar musi wynosic 43 B");

    struct TPacketGCCharacterCreateFailureMartySama
    {
        uint8_t header{ GC::CHARACTER_CREATE_FAILURE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCCharacterCreateFailureMartySama) == 2, "TPacketGCCharacterCreateFailureMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCCharacterDeleteSuccessMartySama
    {
        uint8_t header{ GC::CHARACTER_DELETE_SUCCESS };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCCharacterDeleteSuccessMartySama) == 2, "TPacketGCCharacterDeleteSuccessMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCCharacterDeleteWrongSocialIdMartySama
    {
        uint8_t header{ GC::CHARACTER_DELETE_WRONG_SOCIAL_ID };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCCharacterDeleteWrongSocialIdMartySama) == 2, "TPacketGCCharacterDeleteWrongSocialIdMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCAttackMarty
    {
        uint8_t header{ GC::ATTACK };
        uint32_t dwVID{};
        uint32_t dwVictimVID{};
        uint8_t bType{};
    };
    static_assert(sizeof(TPacketGCAttackMarty) == 10, "TPacketGCAttackMarty rozmiar musi wynosic 10 B");

    struct TPacketGCStunMartySama
    {
        uint8_t header{ GC::STUN };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCStunMartySama) == 5, "TPacketGCStunMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCDeadMarty
    {
        uint8_t header{ GC::DEAD };
        uint32_t dwVID{};
    };
    static_assert(sizeof(TPacketGCDeadMarty) == 5, "TPacketGCDeadMarty rozmiar musi wynosic 5 B");

    struct TPacketGCMainCharacterMartySama
    {
        uint8_t header{ GC::MAIN_CHARACTER };
        uint8_t payload[6]{};
    };
    static_assert(sizeof(TPacketGCMainCharacterMartySama) == 7, "TPacketGCMainCharacterMartySama rozmiar musi wynosic 7 B");

    struct TPacketGCCharacterPointsMartySama
    {
        uint8_t header{ GC::CHARACTER_POINTS };
        uint8_t payload[64]{};
    };
    static_assert(sizeof(TPacketGCCharacterPointsMartySama) == 65, "TPacketGCCharacterPointsMartySama rozmiar musi wynosic 65 B");

    struct TPacketGCCharacterPointChangeMartySama
    {
        uint8_t header{ GC::CHARACTER_POINT_CHANGE };
        uint8_t payload[13]{};
    };
    static_assert(sizeof(TPacketGCCharacterPointChangeMartySama) == 14, "TPacketGCCharacterPointChangeMartySama rozmiar musi wynosic 14 B");

    struct TPacketGCChangeSpeedMartySama
    {
        uint8_t header{ GC::CHANGE_SPEED };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCChangeSpeedMartySama) == 6, "TPacketGCChangeSpeedMartySama rozmiar musi wynosic 6 B");

    struct TPacketGCCharacterUpdateMartySama
    {
        uint8_t header{ GC::CHARACTER_UPDATE };
        uint8_t payload[34]{};
    };
    static_assert(sizeof(TPacketGCCharacterUpdateMartySama) == 35, "TPacketGCCharacterUpdateMartySama rozmiar musi wynosic 35 B");

    struct TPacketGCItemDelMartySama
    {
        uint8_t header{ GC::ITEM_DEL };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCItemDelMartySama) == 4, "TPacketGCItemDelMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCItemSetMartySama
    {
        uint8_t header{ GC::ITEM_SET };
        uint8_t payload[50]{};
    };
    static_assert(sizeof(TPacketGCItemSetMartySama) == 51, "TPacketGCItemSetMartySama rozmiar musi wynosic 51 B");

    struct TPacketGCItemUseMartySama
    {
        uint8_t header{ GC::ITEM_USE };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCItemUseMartySama) == 4, "TPacketGCItemUseMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCItemDropMartySama
    {
        uint8_t header{ GC::ITEM_DROP };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCItemDropMartySama) == 9, "TPacketGCItemDropMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCCharacterUpdateNewMartySama
    {
        uint8_t header{ GC::CHARACTER_UPDATE_NEW };
        uint8_t payload[41]{};
    };
    static_assert(sizeof(TPacketGCCharacterUpdateNewMartySama) == 42, "TPacketGCCharacterUpdateNewMartySama rozmiar musi wynosic 42 B");

    struct TPacketGCItemUpdateMartySama
    {
        uint8_t header{ GC::ITEM_UPDATE };
        uint8_t payload[50]{};
    };
    static_assert(sizeof(TPacketGCItemUpdateMartySama) == 51, "TPacketGCItemUpdateMartySama rozmiar musi wynosic 51 B");

    struct TPacketGCItemGroundAddMartySama
    {
        uint8_t header{ GC::ITEM_GROUND_ADD };
        uint8_t payload[18]{};
    };
    static_assert(sizeof(TPacketGCItemGroundAddMartySama) == 19, "TPacketGCItemGroundAddMartySama rozmiar musi wynosic 19 B");

    struct TPacketGCItemGroundDelMartySama
    {
        uint8_t header{ GC::ITEM_GROUND_DEL };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCItemGroundDelMartySama) == 5, "TPacketGCItemGroundDelMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCQuickslotAddMartySama
    {
        uint8_t header{ GC::QUICKSLOT_ADD };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCQuickslotAddMartySama) == 4, "TPacketGCQuickslotAddMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCQuickslotDelMartySama
    {
        uint8_t header{ GC::QUICKSLOT_DEL };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCQuickslotDelMartySama) == 3, "TPacketGCQuickslotDelMartySama rozmiar musi wynosic 3 B");

    struct TPacketGCQuickslotSwapMartySama
    {
        uint8_t header{ GC::QUICKSLOT_SWAP };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCQuickslotSwapMartySama) == 4, "TPacketGCQuickslotSwapMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCItemOwnershipMartySama
    {
        uint8_t header{ GC::ITEM_OWNERSHIP };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCItemOwnershipMartySama) == 9, "TPacketGCItemOwnershipMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCWhisperMartySama
    {
        uint8_t header{ GC::WHISPER };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCWhisperMartySama) == 3, "TPacketGCWhisperMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCMotionMartySama
    {
        uint8_t header{ GC::MOTION };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCMotionMartySama) == 10, "TPacketGCMotionMartySama rozmiar musi wynosic 10 B");

    struct TPacketGCPartsMartySama
    {
        uint8_t header{ GC::PARTS };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketGCPartsMartySama) == 11, "TPacketGCPartsMartySama rozmiar musi wynosic 11 B");

    struct TPacketGCShopMartySama
    {
        uint8_t header{ GC::SHOP };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCShopMartySama) == 3, "TPacketGCShopMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCShopSignMartySama
    {
        uint8_t header{ GC::SHOP_SIGN };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCShopSignMartySama) == 3, "TPacketGCShopSignMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCDuelStartMartySama
    {
        uint8_t header{ GC::DUEL_START };
        uint8_t payload[36]{};
    };
    static_assert(sizeof(TPacketGCDuelStartMartySama) == 37, "TPacketGCDuelStartMartySama rozmiar musi wynosic 37 B");

    struct TPacketGCPvpMartySama
    {
        uint8_t header{ GC::PVP };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketGCPvpMartySama) == 11, "TPacketGCPvpMartySama rozmiar musi wynosic 11 B");

    struct TPacketGCExchangeMartySama
    {
        uint8_t header{ GC::EXCHANGE };
        uint8_t payload[276]{};
    };
    static_assert(sizeof(TPacketGCExchangeMartySama) == 277, "TPacketGCExchangeMartySama rozmiar musi wynosic 277 B");

    struct TPacketGCCharacterPositionMartySama
    {
        uint8_t header{ GC::CHARACTER_POSITION };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCCharacterPositionMartySama) == 9, "TPacketGCCharacterPositionMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCPingMartySama
    {
        uint8_t header{ GC::PING };
    };
    static_assert(sizeof(TPacketGCPingMartySama) == 1, "TPacketGCPingMartySama rozmiar musi wynosic 1 B");

    struct TPacketGCScriptMartySama
    {
        uint8_t header{ GC::SCRIPT };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCScriptMartySama) == 3, "TPacketGCScriptMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCQuestConfirmMartySama
    {
        uint8_t header{ GC::QUEST_CONFIRM };
        uint8_t payload[72]{};
    };
    static_assert(sizeof(TPacketGCQuestConfirmMartySama) == 73, "TPacketGCQuestConfirmMartySama rozmiar musi wynosic 73 B");

    struct TPacketGCMountMartySama
    {
        uint8_t header{ GC::MOUNT };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCMountMartySama) == 5, "TPacketGCMountMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCOwnershipMartySama
    {
        uint8_t header{ GC::OWNERSHIP };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCOwnershipMartySama) == 5, "TPacketGCOwnershipMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCTargetMarty
    {
        uint8_t header{ GC::TARGET };
        uint32_t dwVID{};
        uint8_t bHPPercent{};
    };
    static_assert(sizeof(TPacketGCTargetMarty) == 6, "TPacketGCTargetMarty rozmiar musi wynosic 6 B");

    struct TPacketGCCommandMartySama
    {
        uint8_t header{ GC::COMMAND };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCCommandMartySama) == 3, "TPacketGCCommandMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCWarpMartySama
    {
        uint8_t header{ GC::WARP };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCWarpMartySama) == 9, "TPacketGCWarpMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCAddFlyTargetingMartySama
    {
        uint8_t header{ GC::ADD_FLY_TARGETING };
        uint8_t payload[31]{};
    };
    static_assert(sizeof(TPacketGCAddFlyTargetingMartySama) == 32, "TPacketGCAddFlyTargetingMartySama rozmiar musi wynosic 32 B");

    struct TPacketGCCreateFlyMartySama
    {
        uint8_t header{ GC::CREATE_FLY };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCCreateFlyMartySama) == 4, "TPacketGCCreateFlyMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCFlyTargetingMartySama
    {
        uint8_t header{ GC::FLY_TARGETING };
        uint8_t payload[19]{};
    };
    static_assert(sizeof(TPacketGCFlyTargetingMartySama) == 20, "TPacketGCFlyTargetingMartySama rozmiar musi wynosic 20 B");

    struct TPacketGCSkillLevelMartySama
    {
        uint8_t header{ GC::SKILL_LEVEL };
        uint8_t payload[16]{};
    };
    static_assert(sizeof(TPacketGCSkillLevelMartySama) == 17, "TPacketGCSkillLevelMartySama rozmiar musi wynosic 17 B");

    struct TPacketGCQuestInfoMartySama
    {
        uint8_t header{ GC::QUEST_INFO };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCQuestInfoMartySama) == 3, "TPacketGCQuestInfoMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCSafeboxSetMartySama
    {
        uint8_t header{ GC::SAFEBOX_SET };
        uint8_t payload[50]{};
    };
    static_assert(sizeof(TPacketGCSafeboxSetMartySama) == 51, "TPacketGCSafeboxSetMartySama rozmiar musi wynosic 51 B");

    struct TPacketGCSafeboxDelMartySama
    {
        uint8_t header{ GC::SAFEBOX_DEL };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCSafeboxDelMartySama) == 4, "TPacketGCSafeboxDelMartySama rozmiar musi wynosic 4 B");

    struct TPacketGCSafeboxWrongPasswordMartySama
    {
        uint8_t header{ GC::SAFEBOX_WRONG_PASSWORD };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCSafeboxWrongPasswordMartySama) == 2, "TPacketGCSafeboxWrongPasswordMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCSafeboxSizeMartySama
    {
        uint8_t header{ GC::SAFEBOX_SIZE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCSafeboxSizeMartySama) == 2, "TPacketGCSafeboxSizeMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCMainCharacter2MartySama
    {
        uint8_t header{ GC::MAIN_CHARACTER2 };
        uint8_t payload[6]{};
    };
    static_assert(sizeof(TPacketGCMainCharacter2MartySama) == 7, "TPacketGCMainCharacter2MartySama rozmiar musi wynosic 7 B");

    struct TPacketGCChannelMartySama
    {
        uint8_t header{ GC::CHANNEL };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCChannelMartySama) == 2, "TPacketGCChannelMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCAffectAddMartySama
    {
        uint8_t header{ GC::AFFECT_ADD };
        uint8_t payload[11]{};
    };
    static_assert(sizeof(TPacketGCAffectAddMartySama) == 12, "TPacketGCAffectAddMartySama rozmiar musi wynosic 12 B");

    struct TPacketGCAffectRemoveMartySama
    {
        uint8_t header{ GC::AFFECT_REMOVE };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCAffectRemoveMartySama) == 5, "TPacketGCAffectRemoveMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCDamageInfoMarty
    {
        uint8_t header{ GC::DAMAGE_INFO };
        uint32_t dwVID{};
        uint8_t flag{};
        int32_t damage{};
    };
    static_assert(sizeof(TPacketGCDamageInfoMarty) == 10, "TPacketGCDamageInfoMarty rozmiar musi wynosic 10 B");

    struct TPacketGCPanamaPackMartySama
    {
        uint8_t header{ GC::PANAMA_PACK };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCPanamaPackMartySama) == 3, "TPacketGCPanamaPackMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCRespondChannelstatusMartySama
    {
        uint8_t header{ GC::RESPOND_CHANNELSTATUS };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketGCRespondChannelstatusMartySama) == 3, "TPacketGCRespondChannelstatusMartySama bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketGCTimeSyncMartySama
    {
        uint8_t header{ GC::TIME_SYNC };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCTimeSyncMartySama) == 5, "TPacketGCTimeSyncMartySama rozmiar musi wynosic 5 B");

    struct TPacketGCPhaseMartySama
    {
        uint8_t header{ GC::PHASE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCPhaseMartySama) == 2, "TPacketGCPhaseMartySama rozmiar musi wynosic 2 B");

    struct TPacketGCBindudpMartySama
    {
        uint8_t header{ GC::BINDUDP };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCBindudpMartySama) == 9, "TPacketGCBindudpMartySama rozmiar musi wynosic 9 B");

    struct TPacketGCHandshakeMartySama
    {
        uint8_t header{ GC::HANDSHAKE };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketGCHandshakeMartySama) == 13, "TPacketGCHandshakeMartySama rozmiar musi wynosic 13 B");

#pragma pack(pop)

    // ========================================================================
    // Tabela Informacji o Pakietach GC
    // ========================================================================
    struct GCInfo
    {
        uint32_t size{0};
        bool isDynamic{false};
    };

    inline bool GetGCInfo(uint8_t header, GCInfo& info)
    {
        switch (header)
        {
#include "martysama_gc_table.inl"
        default:
            return false;
        }
    }

} // namespace Protocol::Generated::MartySama
