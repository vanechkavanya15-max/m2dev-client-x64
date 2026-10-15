#pragma once

// ============================================================================
// AUTOGEN PROTOCOL SPECIFICATION - Beavium (Custom Security Profile)
// Single Source of Truth: docs/AI_PROTOCOL_MAP.json
// Standard: C++23 | Zero Padding | static_assert verified
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace Protocol::Generated::Beavium
{
    // ========================================================================
    // Opcody CG (Client -> Game Server)
    // ========================================================================
    namespace CG
    {
        constexpr uint8_t LOGIN                    = 0x01; // Logowanie Beavium (53B)
        constexpr uint8_t ATTACK                   = 0x02; // Atak 8B z 16-bitowa sekwencja
        constexpr uint8_t CHAT                     = 0x03; // Dynamiczny czat
        constexpr uint8_t CHARACTER_CREATE         = 0x04; // Tworzenie postaci (42B)
        constexpr uint8_t CHARACTER_DESTROY        = 0x05; // Usuniecie postaci (10B)
        constexpr uint8_t CHAR_SELECT              = 0x06; // Wybor postaci ze slotu z tokenem (14B)
        constexpr uint8_t MOVE                     = 0x07; // Ruch 29B z mikrostopniami (* 1e6)
        constexpr uint8_t ENTERGAME                = 0x0A; // Wejscie do gry (1B)
        constexpr uint8_t ITEM_USE                 = 0x0C; // Uzycie przedmiotu (12B: cell, win, vnum, vid)
        constexpr uint8_t ITEM_MOVE                = 0x0D; // Przeniesienie w ekwipunku (8B)
        constexpr uint8_t ITEM_PICKUP              = 0x0E; // Podniesienie przedmiotu (5B: itemVid)
        constexpr uint8_t ON_CLICK                 = 0x14; // Klikniecie w cel (6B: targetVid, flag 0x00)
        constexpr uint8_t SCRIPT_ANSWER            = 0x17; // Odpowiedz na skrypt (2B)
        constexpr uint8_t WHISPER                  = 0x19; // Dynamiczny szept
        constexpr uint8_t SHOP                     = 0x1B; // Sklep (2..7B)
        constexpr uint8_t ATTACK_ALT               = 0x1D; // Alternatywny atak (9B)
        constexpr uint8_t EXCHANGE                 = 0x1F; // Handel (2..6B)
        constexpr uint8_t TARGET                   = 0x22; // Wybor celu (5B: targetVid)
        constexpr uint8_t SCRIPT_BUTTON            = 0x25; // Przycisk questu (5B)
        constexpr uint8_t USE_SKILL                = 0x34; // Umiejetnosc (9B)
        constexpr uint8_t QUEST_CONFIRM            = 0x35; // Potwierdzenie questu (6B)
        constexpr uint8_t SELECT_EMPIRE            = 0x35; // Wybor krolestwa (2B)
        constexpr uint8_t REFINE                   = 0x42; // Ulepszanie (6B)
        constexpr uint8_t TREASURE_LOOT            = 0x53; // Loot skarbca (11B)
        constexpr uint8_t TARGET_LOOT              = 0x54; // Loot celu (11B)
        constexpr uint8_t ITEM_USE_EXTEND          = 0x5D; // Rozszerzone uzycie przedmiotu (11B)
        constexpr uint8_t AUTH_LOGIN               = 0x67; // Autoryzacja z AES-CFB payloadem
        constexpr uint8_t LOGIN2                   = 0x68; // Logowanie z loginKey 8B i clientKey 16B (56B)
        constexpr uint8_t KEY_AGREEMENT            = 0xFA; // Wymiana kluczy X25519 (137B)
        constexpr uint8_t PONG                     = 0xFC; // Keepalive (1B)
        constexpr uint8_t HANDSHAKE_ACK            = 0xFF; // Handshake (13B)
    }

    // ========================================================================
    // Opcody GC (Game Server -> Client)
    // ========================================================================
    namespace GC
    {
        constexpr uint8_t CHARACTER_ADD            = 0x01;
        constexpr uint8_t CHARACTER_DEL            = 0x02;
        constexpr uint8_t CHARACTER_MOVE           = 0x03;
        constexpr uint8_t CHAT                     = 0x04;
        constexpr uint8_t SYNC_POSITION            = 0x05;
        constexpr uint8_t LOGIN_SUCCESS3           = 0x06;
        constexpr uint8_t LOGIN_FAILURE            = 0x07;
        constexpr uint8_t CHAR_CREATE_SUCCESS      = 0x08;
        constexpr uint8_t CHAR_CREATE_FAILURE      = 0x09;
        constexpr uint8_t CHAR_DELETE_SUCCESS      = 0x0A;
        constexpr uint8_t CHAR_DELETE_WRONG_PIN    = 0x0B;
        constexpr uint8_t ATTACK                   = 0x0C;
        constexpr uint8_t STUN                     = 0x0D;
        constexpr uint8_t DEAD                     = 0x0E;
        constexpr uint8_t MAIN_CHARACTER           = 0x0F;
        constexpr uint8_t CHARACTER_UPDATE         = 0x10;
        constexpr uint8_t ITEM_USE                 = 0x11;
        constexpr uint8_t ITEM_UPDATE              = 0x12;
        constexpr uint8_t ITEM_GROUND_ADD          = 0x13;
        constexpr uint8_t QUICKSLOT_ADD            = 0x15;
        constexpr uint8_t QUICKSLOT_DEL            = 0x16;
        constexpr uint8_t QUICKSLOT_SWAP           = 0x17;
        constexpr uint8_t ITEM_OWNERSHIP           = 0x18;
        constexpr uint8_t WHISPER                  = 0x19;
        constexpr uint8_t MOTION                   = 0x1A;
        constexpr uint8_t SHOP                     = 0x1B;
        constexpr uint8_t SHOP_SIGN                = 0x1C;
        constexpr uint8_t DUEL_START               = 0x1D;
        constexpr uint8_t PVP                      = 0x1E;
        constexpr uint8_t EXCHANGE                 = 0x1F;
        constexpr uint8_t CHARACTER_POINTS         = 0x20;
        constexpr uint8_t CHARACTER_POINT_CHANGE   = 0x21;
        constexpr uint8_t EXCHANGE2                = 0x22;
        constexpr uint8_t RANKING_DATA             = 0x23;
        constexpr uint8_t HANDSHAKE                = 0x24;
        constexpr uint8_t SCRIPT                   = 0x25;
        constexpr uint8_t QUEST_BUTTON             = 0x26;
        constexpr uint8_t PING                     = 0x2C;
        constexpr uint8_t QUEST_INFO               = 0x36;
        constexpr uint8_t WARP                     = 0x40;
        constexpr uint8_t DAMAGE_INFO              = 0x87;
        constexpr uint8_t VOICE_CHAT_DATA          = 0x90; // 32-bit dynamic voice payload
        constexpr uint8_t VOICE_CHAT_DATA2         = 0x91; // 32-bit dynamic voice payload
        constexpr uint8_t KEY_AGREEMENT            = 0xFA;
        constexpr uint8_t PHASE                    = 0xFD;
        constexpr uint8_t PONG                     = 0xFE;
        constexpr uint8_t HANDSHAKE_FIN            = 0xFF;
    }

    // ========================================================================
    // Struktury Pakietow (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    // --- Pakiety CG (Client -> Game) ---
    struct TPacketCGLoginBeavium
    {
        uint8_t header{ CG::LOGIN };
        uint8_t payload[52]{};
    };
    static_assert(sizeof(TPacketCGLoginBeavium) == 53, "TPacketCGLoginBeavium rozmiar musi wynosic 53 B");

    struct ProxyPacketCGAttack
    {
        uint8_t header{ CG::ATTACK };
        uint8_t attackType{};
        uint32_t dwVictimVID{};
        uint16_t wSeq{};
    };
    static_assert(sizeof(ProxyPacketCGAttack) == 8, "ProxyPacketCGAttack rozmiar musi wynosic 8 B");

    struct TPacketCGChatBeavium
    {
        uint8_t header{ CG::CHAT };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketCGChatBeavium) == 3, "TPacketCGChatBeavium bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketCGCharacterCreateBeavium
    {
        uint8_t header{ CG::CHARACTER_CREATE };
        uint8_t payload[41]{};
    };
    static_assert(sizeof(TPacketCGCharacterCreateBeavium) == 42, "TPacketCGCharacterCreateBeavium rozmiar musi wynosic 42 B");

    struct TPacketCGCharacterDestroyBeavium
    {
        uint8_t header{ CG::CHARACTER_DESTROY };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketCGCharacterDestroyBeavium) == 10, "TPacketCGCharacterDestroyBeavium rozmiar musi wynosic 10 B");

    struct TPacketCGCharSelectBeavium
    {
        uint8_t header{ CG::CHAR_SELECT };
        uint8_t payload[13]{};
    };
    static_assert(sizeof(TPacketCGCharSelectBeavium) == 14, "TPacketCGCharSelectBeavium rozmiar musi wynosic 14 B");

    struct TPacketCGMoveBeavium
    {
        uint8_t header{ CG::MOVE };
        uint8_t bFunc{};
        uint16_t wArg{};
        uint32_t dwRot{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
        uint32_t dwExtra{};
        uint8_t bFlag{};
        uint32_t dwReserved{};
    };
    static_assert(sizeof(TPacketCGMoveBeavium) == 29, "TPacketCGMoveBeavium rozmiar musi wynosic 29 B");

    struct TPacketCGEntergameBeavium
    {
        uint8_t header{ CG::ENTERGAME };
    };
    static_assert(sizeof(TPacketCGEntergameBeavium) == 1, "TPacketCGEntergameBeavium rozmiar musi wynosic 1 B");

    struct TPacketCGItemUseBeavium
    {
        uint8_t header{ CG::ITEM_USE };
        uint16_t cell{};
        uint8_t window{};
        uint32_t vnum{};
        uint32_t targetVid{};
    };
    static_assert(sizeof(TPacketCGItemUseBeavium) == 12, "TPacketCGItemUseBeavium rozmiar musi wynosic 12 B");

    struct TPacketCGItemMoveBeavium
    {
        uint8_t header{ CG::ITEM_MOVE };
        uint8_t payload[7]{};
    };
    static_assert(sizeof(TPacketCGItemMoveBeavium) == 8, "TPacketCGItemMoveBeavium rozmiar musi wynosic 8 B");

    struct TPacketCGItemPickupBeavium
    {
        uint8_t header{ CG::ITEM_PICKUP };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketCGItemPickupBeavium) == 5, "TPacketCGItemPickupBeavium rozmiar musi wynosic 5 B");

    struct TPacketCGOnClickBeavium
    {
        uint8_t header{ CG::ON_CLICK };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketCGOnClickBeavium) == 6, "TPacketCGOnClickBeavium rozmiar musi wynosic 6 B");

    struct TPacketCGScriptAnswerBeavium
    {
        uint8_t header{ CG::SCRIPT_ANSWER };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketCGScriptAnswerBeavium) == 2, "TPacketCGScriptAnswerBeavium rozmiar musi wynosic 2 B");

    struct TPacketCGWhisperBeavium
    {
        uint8_t header{ CG::WHISPER };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketCGWhisperBeavium) == 3, "TPacketCGWhisperBeavium bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketCGShopBeavium
    {
        uint8_t header{ CG::SHOP };
        uint8_t payload[6]{};
    };
    static_assert(sizeof(TPacketCGShopBeavium) == 7, "TPacketCGShopBeavium rozmiar musi wynosic 7 B");

    struct TPacketCGAttackAltBeavium
    {
        uint8_t header{ CG::ATTACK_ALT };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketCGAttackAltBeavium) == 9, "TPacketCGAttackAltBeavium rozmiar musi wynosic 9 B");

    struct TPacketCGExchangeBeavium
    {
        uint8_t header{ CG::EXCHANGE };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketCGExchangeBeavium) == 6, "TPacketCGExchangeBeavium rozmiar musi wynosic 6 B");

    struct TPacketCGTargetBeavium
    {
        uint8_t header{ CG::TARGET };
        uint32_t targetVid{};
    };
    static_assert(sizeof(TPacketCGTargetBeavium) == 5, "TPacketCGTargetBeavium rozmiar musi wynosic 5 B");

    struct TPacketCGScriptButtonBeavium
    {
        uint8_t header{ CG::SCRIPT_BUTTON };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketCGScriptButtonBeavium) == 5, "TPacketCGScriptButtonBeavium rozmiar musi wynosic 5 B");

    struct TPacketCGUseSkillBeavium
    {
        uint8_t header{ CG::USE_SKILL };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketCGUseSkillBeavium) == 9, "TPacketCGUseSkillBeavium rozmiar musi wynosic 9 B");

    struct TPacketCGQuestConfirmBeavium
    {
        uint8_t header{ CG::QUEST_CONFIRM };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketCGQuestConfirmBeavium) == 6, "TPacketCGQuestConfirmBeavium rozmiar musi wynosic 6 B");

    struct TPacketCGSelectEmpireBeavium
    {
        uint8_t header{ CG::SELECT_EMPIRE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketCGSelectEmpireBeavium) == 2, "TPacketCGSelectEmpireBeavium rozmiar musi wynosic 2 B");

    struct TPacketCGRefineBeavium
    {
        uint8_t header{ CG::REFINE };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketCGRefineBeavium) == 6, "TPacketCGRefineBeavium rozmiar musi wynosic 6 B");

    struct TPacketCGTreasureLootBeavium
    {
        uint8_t header{ CG::TREASURE_LOOT };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketCGTreasureLootBeavium) == 11, "TPacketCGTreasureLootBeavium rozmiar musi wynosic 11 B");

    struct TPacketCGTargetLootBeavium
    {
        uint8_t header{ CG::TARGET_LOOT };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketCGTargetLootBeavium) == 11, "TPacketCGTargetLootBeavium rozmiar musi wynosic 11 B");

    struct TPacketCGItemUseExtendBeavium
    {
        uint8_t header{ CG::ITEM_USE_EXTEND };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketCGItemUseExtendBeavium) == 11, "TPacketCGItemUseExtendBeavium rozmiar musi wynosic 11 B");

    struct TPacketCGAuthLoginBeavium
    {
        uint8_t header{ CG::AUTH_LOGIN };
        uint8_t payload[31]{};
    };
    static_assert(sizeof(TPacketCGAuthLoginBeavium) == 32, "TPacketCGAuthLoginBeavium rozmiar musi wynosic 32 B");

    struct TPacketCGLogin2Beavium
    {
        uint8_t header{ CG::LOGIN2 };
        uint8_t payload[55]{};
    };
    static_assert(sizeof(TPacketCGLogin2Beavium) == 56, "TPacketCGLogin2Beavium rozmiar musi wynosic 56 B");

    struct TPacketCGKeyAgreementBeavium
    {
        uint8_t header{ CG::KEY_AGREEMENT };
        uint8_t payload[136]{};
    };
    static_assert(sizeof(TPacketCGKeyAgreementBeavium) == 137, "TPacketCGKeyAgreementBeavium rozmiar musi wynosic 137 B");

    struct TPacketCGPongBeavium
    {
        uint8_t header{ CG::PONG };
    };
    static_assert(sizeof(TPacketCGPongBeavium) == 1, "TPacketCGPongBeavium rozmiar musi wynosic 1 B");

    struct TPacketCGHandshakeAckBeavium
    {
        uint8_t header{ CG::HANDSHAKE_ACK };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketCGHandshakeAckBeavium) == 13, "TPacketCGHandshakeAckBeavium rozmiar musi wynosic 13 B");

    // --- Pakiety GC (Game -> Client) ---
    struct TPacketGCCharacterAddBeavium
    {
        uint8_t header{ GC::CHARACTER_ADD };
        uint8_t payload[44]{};
    };
    static_assert(sizeof(TPacketGCCharacterAddBeavium) == 45, "TPacketGCCharacterAddBeavium rozmiar musi wynosic 45 B");

    struct TPacketGCCharacterDelBeavium
    {
        uint8_t header{ GC::CHARACTER_DEL };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCCharacterDelBeavium) == 6, "TPacketGCCharacterDelBeavium rozmiar musi wynosic 6 B");

    struct TPacketGCCharacterMoveBeavium
    {
        uint8_t header{ GC::CHARACTER_MOVE };
        uint8_t payload[30]{};
    };
    static_assert(sizeof(TPacketGCCharacterMoveBeavium) == 31, "TPacketGCCharacterMoveBeavium rozmiar musi wynosic 31 B");

    struct TPacketGCChatBeavium
    {
        uint8_t header{ GC::CHAT };
        uint8_t payload[13]{};
    };
    static_assert(sizeof(TPacketGCChatBeavium) == 14, "TPacketGCChatBeavium rozmiar musi wynosic 14 B");

    struct TPacketGCSyncPositionBeavium
    {
        uint8_t header{ GC::SYNC_POSITION };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCSyncPositionBeavium) == 3, "TPacketGCSyncPositionBeavium rozmiar musi wynosic 3 B");

    struct TPacketGCLoginSuccess3Beavium
    {
        uint8_t header{ GC::LOGIN_SUCCESS3 };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCLoginSuccess3Beavium) == 10, "TPacketGCLoginSuccess3Beavium rozmiar musi wynosic 10 B");

    struct TPacketGCLoginFailureBeavium
    {
        uint8_t header{ GC::LOGIN_FAILURE };
        uint8_t payload[90]{};
    };
    static_assert(sizeof(TPacketGCLoginFailureBeavium) == 91, "TPacketGCLoginFailureBeavium rozmiar musi wynosic 91 B");

    struct TPacketGCCharCreateSuccessBeavium
    {
        uint8_t header{ GC::CHAR_CREATE_SUCCESS };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCCharCreateSuccessBeavium) == 2, "TPacketGCCharCreateSuccessBeavium rozmiar musi wynosic 2 B");

    struct TPacketGCCharCreateFailureBeavium
    {
        uint8_t header{ GC::CHAR_CREATE_FAILURE };
    };
    static_assert(sizeof(TPacketGCCharCreateFailureBeavium) == 1, "TPacketGCCharCreateFailureBeavium rozmiar musi wynosic 1 B");

    struct TPacketGCCharDeleteSuccessBeavium
    {
        uint8_t header{ GC::CHAR_DELETE_SUCCESS };
    };
    static_assert(sizeof(TPacketGCCharDeleteSuccessBeavium) == 1, "TPacketGCCharDeleteSuccessBeavium rozmiar musi wynosic 1 B");

    struct TPacketGCCharDeleteWrongPinBeavium
    {
        uint8_t header{ GC::CHAR_DELETE_WRONG_PIN };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCCharDeleteWrongPinBeavium) == 5, "TPacketGCCharDeleteWrongPinBeavium rozmiar musi wynosic 5 B");

    struct TPacketGCAttackBeavium
    {
        uint8_t header{ GC::ATTACK };
        uint32_t dwVID{};
        uint32_t dwVictimVID{};
        uint8_t bType{};
        uint16_t wSeq{};
    };
    static_assert(sizeof(TPacketGCAttackBeavium) == 12, "TPacketGCAttackBeavium rozmiar musi wynosic 12 B");

    struct TPacketGCStunBeavium
    {
        uint8_t header{ GC::STUN };
        uint8_t payload[3072]{};
    };
    static_assert(sizeof(TPacketGCStunBeavium) == 3073, "TPacketGCStunBeavium rozmiar musi wynosic 3073 B");

    struct TPacketGCDeadBeavium
    {
        uint8_t header{ GC::DEAD };
        uint32_t dwVID{};
        uint8_t bReason{};
        uint8_t dwReserved[20]{};
    };
    static_assert(sizeof(TPacketGCDeadBeavium) == 26, "TPacketGCDeadBeavium rozmiar musi wynosic 26 B");

    struct TPacketGCMainCharacterBeavium
    {
        uint8_t header{ GC::MAIN_CHARACTER };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCMainCharacterBeavium) == 6, "TPacketGCMainCharacterBeavium rozmiar musi wynosic 6 B");

    struct TPacketGCCharacterUpdateBeavium
    {
        uint8_t header{ GC::CHARACTER_UPDATE };
        uint8_t payload[81]{};
    };
    static_assert(sizeof(TPacketGCCharacterUpdateBeavium) == 82, "TPacketGCCharacterUpdateBeavium rozmiar musi wynosic 82 B");

    struct TPacketGCItemUseBeavium
    {
        uint8_t header{ GC::ITEM_USE };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCItemUseBeavium) == 4, "TPacketGCItemUseBeavium rozmiar musi wynosic 4 B");

    struct TPacketGCItemUpdateBeavium
    {
        uint8_t header{ GC::ITEM_UPDATE };
        uint8_t payload[280]{};
    };
    static_assert(sizeof(TPacketGCItemUpdateBeavium) == 281, "TPacketGCItemUpdateBeavium rozmiar musi wynosic 281 B");

    struct TPacketGCItemGroundAddBeavium
    {
        uint8_t header{ GC::ITEM_GROUND_ADD };
        uint8_t payload[15]{};
    };
    static_assert(sizeof(TPacketGCItemGroundAddBeavium) == 16, "TPacketGCItemGroundAddBeavium rozmiar musi wynosic 16 B");

    struct TPacketGCQuickslotAddBeavium
    {
        uint8_t header{ GC::QUICKSLOT_ADD };
        uint8_t payload[267]{};
    };
    static_assert(sizeof(TPacketGCQuickslotAddBeavium) == 268, "TPacketGCQuickslotAddBeavium rozmiar musi wynosic 268 B");

    struct TPacketGCQuickslotDelBeavium
    {
        uint8_t header{ GC::QUICKSLOT_DEL };
        uint8_t payload[25]{};
    };
    static_assert(sizeof(TPacketGCQuickslotDelBeavium) == 26, "TPacketGCQuickslotDelBeavium rozmiar musi wynosic 26 B");

    struct TPacketGCQuickslotSwapBeavium
    {
        uint8_t header{ GC::QUICKSLOT_SWAP };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCQuickslotSwapBeavium) == 5, "TPacketGCQuickslotSwapBeavium rozmiar musi wynosic 5 B");

    struct TPacketGCItemOwnershipBeavium
    {
        uint8_t header{ GC::ITEM_OWNERSHIP };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCItemOwnershipBeavium) == 4, "TPacketGCItemOwnershipBeavium rozmiar musi wynosic 4 B");

    struct TPacketGCWhisperBeavium
    {
        uint8_t header{ GC::WHISPER };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCWhisperBeavium) == 2, "TPacketGCWhisperBeavium rozmiar musi wynosic 2 B");

    struct TPacketGCMotionBeavium
    {
        uint8_t header{ GC::MOTION };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCMotionBeavium) == 3, "TPacketGCMotionBeavium rozmiar musi wynosic 3 B");

    struct TPacketGCShopBeavium
    {
        uint8_t header{ GC::SHOP };
        uint8_t payload[37]{};
    };
    static_assert(sizeof(TPacketGCShopBeavium) == 38, "TPacketGCShopBeavium rozmiar musi wynosic 38 B");

    struct TPacketGCShopSignBeavium
    {
        uint8_t header{ GC::SHOP_SIGN };
        uint8_t payload[464]{};
    };
    static_assert(sizeof(TPacketGCShopSignBeavium) == 465, "TPacketGCShopSignBeavium rozmiar musi wynosic 465 B");

    struct TPacketGCDuelStartBeavium
    {
        uint8_t header{ GC::DUEL_START };
        uint8_t payload[36]{};
    };
    static_assert(sizeof(TPacketGCDuelStartBeavium) == 37, "TPacketGCDuelStartBeavium rozmiar musi wynosic 37 B");

    struct TPacketGCPvpBeavium
    {
        uint8_t header{ GC::PVP };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketGCPvpBeavium) == 11, "TPacketGCPvpBeavium rozmiar musi wynosic 11 B");

    struct TPacketGCExchangeBeavium
    {
        uint8_t header{ GC::EXCHANGE };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCExchangeBeavium) == 4, "TPacketGCExchangeBeavium rozmiar musi wynosic 4 B");

    struct TPacketGCCharacterPointsBeavium
    {
        uint8_t header{ GC::CHARACTER_POINTS };
        uint8_t payload[69]{};
    };
    static_assert(sizeof(TPacketGCCharacterPointsBeavium) == 70, "TPacketGCCharacterPointsBeavium rozmiar musi wynosic 70 B");

    struct TPacketGCCharacterPointChangeBeavium
    {
        uint8_t header{ GC::CHARACTER_POINT_CHANGE };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCCharacterPointChangeBeavium) == 3, "TPacketGCCharacterPointChangeBeavium rozmiar musi wynosic 3 B");

    struct TPacketGCExchange2Beavium
    {
        uint8_t header{ GC::EXCHANGE2 };
        uint8_t payload[276]{};
    };
    static_assert(sizeof(TPacketGCExchange2Beavium) == 277, "TPacketGCExchange2Beavium rozmiar musi wynosic 277 B");

    struct TPacketGCRankingDataBeavium
    {
        uint8_t header{ GC::RANKING_DATA };
        uint8_t payload[276]{};
    };
    static_assert(sizeof(TPacketGCRankingDataBeavium) == 277, "TPacketGCRankingDataBeavium rozmiar musi wynosic 277 B");

    struct TPacketGCHandshakeBeavium
    {
        uint8_t header{ GC::HANDSHAKE };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCHandshakeBeavium) == 6, "TPacketGCHandshakeBeavium rozmiar musi wynosic 6 B");

    struct TPacketGCScriptBeavium
    {
        uint8_t header{ GC::SCRIPT };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCScriptBeavium) == 3, "TPacketGCScriptBeavium rozmiar musi wynosic 3 B");

    struct TPacketGCQuestButtonBeavium
    {
        uint8_t header{ GC::QUEST_BUTTON };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCQuestButtonBeavium) == 6, "TPacketGCQuestButtonBeavium rozmiar musi wynosic 6 B");

    struct TPacketGCPingBeavium
    {
        uint8_t header{ GC::PING };
    };
    static_assert(sizeof(TPacketGCPingBeavium) == 1, "TPacketGCPingBeavium rozmiar musi wynosic 1 B");

    struct TPacketGCQuestInfoBeavium
    {
        uint8_t header{ GC::QUEST_INFO };
        uint8_t payload[44]{};
    };
    static_assert(sizeof(TPacketGCQuestInfoBeavium) == 45, "TPacketGCQuestInfoBeavium rozmiar musi wynosic 45 B");

    struct TPacketGCWarpBeavium
    {
        uint8_t header{ GC::WARP };
        uint8_t payload[14]{};
    };
    static_assert(sizeof(TPacketGCWarpBeavium) == 15, "TPacketGCWarpBeavium rozmiar musi wynosic 15 B");

    struct TPacketGCDamageInfoBeavium
    {
        uint8_t header{ GC::DAMAGE_INFO };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCDamageInfoBeavium) == 10, "TPacketGCDamageInfoBeavium rozmiar musi wynosic 10 B");

    struct TPacketGCVoiceChatDataBeavium
    {
        uint8_t header{ GC::VOICE_CHAT_DATA };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCVoiceChatDataBeavium) == 5, "TPacketGCVoiceChatDataBeavium rozmiar musi wynosic 5 B");

    struct TPacketGCVoiceChatData2Beavium
    {
        uint8_t header{ GC::VOICE_CHAT_DATA2 };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCVoiceChatData2Beavium) == 5, "TPacketGCVoiceChatData2Beavium rozmiar musi wynosic 5 B");

    struct TPacketGCKeyAgreementBeavium
    {
        uint8_t header{ GC::KEY_AGREEMENT };
        uint8_t payload[136]{};
    };
    static_assert(sizeof(TPacketGCKeyAgreementBeavium) == 137, "TPacketGCKeyAgreementBeavium rozmiar musi wynosic 137 B");

    struct TPacketGCPhaseBeavium
    {
        uint8_t header{ GC::PHASE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCPhaseBeavium) == 2, "TPacketGCPhaseBeavium rozmiar musi wynosic 2 B");

    struct TPacketGCPongBeavium
    {
        uint8_t header{ GC::PONG };
    };
    static_assert(sizeof(TPacketGCPongBeavium) == 1, "TPacketGCPongBeavium rozmiar musi wynosic 1 B");

    struct TPacketGCHandshakeFinBeavium
    {
        uint8_t header{ GC::HANDSHAKE_FIN };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketGCHandshakeFinBeavium) == 13, "TPacketGCHandshakeFinBeavium rozmiar musi wynosic 13 B");

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
#include "beavium_gc_table.inl"
        default:
            return false;
        }
    }

} // namespace Protocol::Generated::Beavium
