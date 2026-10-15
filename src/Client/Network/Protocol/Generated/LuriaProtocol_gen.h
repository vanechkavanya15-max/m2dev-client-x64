#pragma once

// ============================================================================
// AUTOGEN PROTOCOL SPECIFICATION - Luria Online (UPM Profil Luria)
// Single Source of Truth: docs/AI_PROTOCOL_MAP.json
// Standard: C++23 | Zero Padding | static_assert verified
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace Protocol::Generated::Luria
{
    // ========================================================================
    // Opcody CG (Client -> Game Server)
    // ========================================================================
    namespace CG
    {
        constexpr uint8_t LOGIN                    = 0x01; // Logowanie standardowe (49B)
        constexpr uint8_t ATTACK                   = 0x02; // Atak 8B z zabezpieczeniem rotujacej sumy CRC_XOR_TABLE = { 102, 30, 188, 44, 39, 201, 43, 5 }
        constexpr uint8_t MOVE                     = 0x03; // Ruch 19B z precyzyjna rotacja 4B float fRot
        constexpr uint8_t CHAT                     = 0x04; // Dynamiczny czat
        constexpr uint8_t CHARACTER_CREATE         = 0x04; // Tworzenie postaci (34B: slot, nazwa 25B, job 2B, shape, con, int, str, dex)
        constexpr uint8_t CHARACTER_DELETE         = 0x05; // Usuniecie postaci kodem PIN (10B: slot, szPrivateCode 8B)
        constexpr uint8_t CHARACTER_SELECT         = 0x06; // Wybor postaci ze slotu (2B: header, index)
        constexpr uint8_t ENTERGAME                = 0x0A; // Wejscie do gry (1B)
        constexpr uint8_t ITEM_USE                 = 0x0B; // Uzycie przedmiotu (4B: window_type, cell 2B)
        constexpr uint8_t ITEM_DROP                = 0x0C; // Wyrzucenie przedmiotu / zlota (8B: window, cell, count 4B)
        constexpr uint8_t ITEM_MOVE                = 0x0D; // Przeniesienie w ekwipunku (9B: srcWin, srcCell, dstWin, dstCell, count 2B)
        constexpr uint8_t ITEM_PICKUP              = 0x0F; // Podniesienie przedmiotu (5B: header, dwVID 4B)
        constexpr uint8_t ON_CLICK                 = 0x14; // Klikniecie w cel (6B: vid 4B, flag 1B)
        constexpr uint8_t TARGET                   = 0x3D; // Wybor celu (5B: header, targetVid 4B)
        constexpr uint8_t EMPIRE                   = 0x5A; // Wybor krolestwa (2B: header, empire 1B)
        constexpr uint8_t LOGIN2                   = 0x6D; // Wejscie do serwera gry z dwLoginKey (52B)
        constexpr uint8_t AUTH_LOGIN               = 0x6F; // Pelna autoryzacja z PIN, tokenem i SHA-256 HWID (168B)
        constexpr uint8_t CLIENT_VERSION           = 0xF1; // Weryfikacja wersji klienta (67B)
        constexpr uint8_t KEY_AGREEMENT            = 0xFB; // Wymiana kluczy DH2 MODP 1024-bit (261B)
        constexpr uint8_t PONG                     = 0xFE; // Keepalive odpowiedz na Ping (1B)
        constexpr uint8_t HANDSHAKE                = 0xFF; // Synchronizacja czasu zegarow (13B)
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
        constexpr uint8_t WHISPER                  = 0x05;
        constexpr uint8_t LOGIN_SUCCESS3           = 0x06;
        constexpr uint8_t LOGIN_FAILURE            = 0x07;
        constexpr uint8_t GC_08                    = 0x08;
        constexpr uint8_t GC_09                    = 0x09;
        constexpr uint8_t GC_0A                    = 0x0A;
        constexpr uint8_t GC_0B                    = 0x0B;
        constexpr uint8_t ATTACK                   = 0x0C;
        constexpr uint8_t GC_0D                    = 0x0D;
        constexpr uint8_t DEAD                     = 0x0E;
        constexpr uint8_t MAIN_CHARACTER           = 0x0F;
        constexpr uint8_t CHARACTER_UPDATE         = 0x10;
        constexpr uint8_t POINT_CHANGE             = 0x11;
        constexpr uint8_t GC_12                    = 0x12;
        constexpr uint8_t GC_13                    = 0x13;
        constexpr uint8_t GC_14                    = 0x14;
        constexpr uint8_t GC_15                    = 0x15;
        constexpr uint8_t GC_16                    = 0x16;
        constexpr uint8_t GC_19                    = 0x19;
        constexpr uint8_t GC_1A                    = 0x1A;
        constexpr uint8_t GC_1B                    = 0x1B;
        constexpr uint8_t GC_1C                    = 0x1C;
        constexpr uint8_t GC_1D                    = 0x1D;
        constexpr uint8_t GC_1E                    = 0x1E;
        constexpr uint8_t GC_1F                    = 0x1F;
        constexpr uint8_t LOGIN_SUCCESS            = 0x20;
        constexpr uint8_t GC_22                    = 0x22;
        constexpr uint8_t GC_24                    = 0x24;
        constexpr uint8_t SCRIPT                   = 0x26;
        constexpr uint8_t GC_27                    = 0x27;
        constexpr uint8_t GC_28                    = 0x28;
        constexpr uint8_t TARGET                   = 0x29;
        constexpr uint8_t GC_2A                    = 0x2A;
        constexpr uint8_t GC_2B                    = 0x2B;
        constexpr uint8_t PING                     = 0x2C;
        constexpr uint8_t GC_2D                    = 0x2D;
        constexpr uint8_t GC_2E                    = 0x2E;
        constexpr uint8_t GC_32                    = 0x32;
        constexpr uint8_t GC_39                    = 0x39;
        constexpr uint8_t GC_3B                    = 0x3B;
        constexpr uint8_t GC_3C                    = 0x3C;
        constexpr uint8_t GC_3D                    = 0x3D;
        constexpr uint8_t GC_3E                    = 0x3E;
        constexpr uint8_t GC_3F                    = 0x3F;
        constexpr uint8_t WARP                     = 0x41;
        constexpr uint8_t GC_45                    = 0x45;
        constexpr uint8_t GC_46                    = 0x46;
        constexpr uint8_t GC_47                    = 0x47;
        constexpr uint8_t GC_48                    = 0x48;
        constexpr uint8_t GC_49                    = 0x49;
        constexpr uint8_t GC_4A                    = 0x4A;
        constexpr uint8_t GC_4B                    = 0x4B;
        constexpr uint8_t GC_4C                    = 0x4C;
        constexpr uint8_t GC_4D                    = 0x4D;
        constexpr uint8_t GC_4E                    = 0x4E;
        constexpr uint8_t GC_4F                    = 0x4F;
        constexpr uint8_t GC_50                    = 0x50;
        constexpr uint8_t EMPIRE                   = 0x5A;
        constexpr uint8_t TARGET_HP                = 0x6C;
        constexpr uint8_t DAMAGE_INFO              = 0x87;
        constexpr uint8_t AUTH_SUCCESS             = 0x96;
        constexpr uint8_t DYNAMIC_32               = 0xBE; // Dynamiczny pakiet z 32-bitowa dlugoscia na offsetcie 1
        constexpr uint8_t KEY_AGREEMENT_ACK        = 0xFA;
        constexpr uint8_t KEY_AGREEMENT            = 0xFB;
        constexpr uint8_t PHASE                    = 0xFD;
        constexpr uint8_t PONG                     = 0xFE;
        constexpr uint8_t HANDSHAKE                = 0xFF;
    }

    // ========================================================================
    // Struktury Pakietow (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    // --- Pakiety CG (Client -> Game) ---
    struct TPacketCGLoginLuria
    {
        uint8_t header{ CG::LOGIN };
        uint8_t payload[48]{};
    };
    static_assert(sizeof(TPacketCGLoginLuria) == 49, "TPacketCGLoginLuria rozmiar musi wynosic 49 B");

    struct TPacketCGAttackLuria
    {
        uint8_t header{ CG::ATTACK };
        uint8_t bType{};
        uint32_t dwVictimVID{};
        uint8_t bCRCMagicCubeProcPiece{};
        uint8_t bCRCMagicCubeFilePiece{};
    };
    static_assert(sizeof(TPacketCGAttackLuria) == 8, "TPacketCGAttackLuria rozmiar musi wynosic 8 B");

    struct TPacketCGMoveLuria
    {
        uint8_t header{ CG::MOVE };
        uint8_t bFunc{};
        uint8_t bArg{};
        float fRot{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
    };
    static_assert(sizeof(TPacketCGMoveLuria) == 19, "TPacketCGMoveLuria rozmiar musi wynosic 19 B");

    struct TPacketCGChatLuria
    {
        uint8_t header{ CG::CHAT };
        uint16_t size{0};
    };
    static_assert(sizeof(TPacketCGChatLuria) == 3, "TPacketCGChatLuria bazowy rozmiar dynamiczny musi wynosic 3 B");

    struct TPacketCGCreateCharacterLuria
    {
        uint8_t header{ CG::CHARACTER_CREATE };
        uint8_t index{};
        char name[25]{};
        uint16_t job{};
        uint8_t shape{};
        uint8_t CON{};
        uint8_t INT{};
        uint8_t STR{};
        uint8_t DEX{};
    };
    static_assert(sizeof(TPacketCGCreateCharacterLuria) == 34, "TPacketCGCreateCharacterLuria rozmiar musi wynosic 34 B");

    struct TPacketCGDestroyCharacterLuria
    {
        uint8_t header{ CG::CHARACTER_DELETE };
        uint8_t index{};
        char szPrivateCode[8]{};
    };
    static_assert(sizeof(TPacketCGDestroyCharacterLuria) == 10, "TPacketCGDestroyCharacterLuria rozmiar musi wynosic 10 B");

    struct TPacketCGSelectCharacterLuria
    {
        uint8_t header{ CG::CHARACTER_SELECT };
        uint8_t index{};
    };
    static_assert(sizeof(TPacketCGSelectCharacterLuria) == 2, "TPacketCGSelectCharacterLuria rozmiar musi wynosic 2 B");

    struct TPacketCGEntergameLuria
    {
        uint8_t header{ CG::ENTERGAME };
    };
    static_assert(sizeof(TPacketCGEntergameLuria) == 1, "TPacketCGEntergameLuria rozmiar musi wynosic 1 B");

    struct TPacketCGItemUseLuria
    {
        uint8_t header{ CG::ITEM_USE };
        uint8_t window_type{};
        uint16_t cell{};
    };
    static_assert(sizeof(TPacketCGItemUseLuria) == 4, "TPacketCGItemUseLuria rozmiar musi wynosic 4 B");

    struct TPacketCGItemDropLuria
    {
        uint8_t header{ CG::ITEM_DROP };
        uint8_t window_type{};
        uint16_t cell{};
        uint32_t count{};
    };
    static_assert(sizeof(TPacketCGItemDropLuria) == 8, "TPacketCGItemDropLuria rozmiar musi wynosic 8 B");

    struct TPacketCGItemMoveLuria
    {
        uint8_t header{ CG::ITEM_MOVE };
        uint8_t srcWindow{};
        uint16_t srcCell{};
        uint8_t dstWindow{};
        uint16_t dstCell{};
        uint16_t count{};
    };
    static_assert(sizeof(TPacketCGItemMoveLuria) == 9, "TPacketCGItemMoveLuria rozmiar musi wynosic 9 B");

    struct TPacketCGItemPickUpLuria
    {
        uint8_t header{ CG::ITEM_PICKUP };
        uint32_t dwVID{};
    };
    static_assert(sizeof(TPacketCGItemPickUpLuria) == 5, "TPacketCGItemPickUpLuria rozmiar musi wynosic 5 B");

    struct TPacketCGOnClickLuria
    {
        uint8_t header{ CG::ON_CLICK };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketCGOnClickLuria) == 6, "TPacketCGOnClickLuria rozmiar musi wynosic 6 B");

    struct TPacketCGTargetLuria
    {
        uint8_t header{ CG::TARGET };
        uint32_t targetVid{};
    };
    static_assert(sizeof(TPacketCGTargetLuria) == 5, "TPacketCGTargetLuria rozmiar musi wynosic 5 B");

    struct TPacketCGEmpireLuria
    {
        uint8_t header{ CG::EMPIRE };
        uint8_t empire{};
    };
    static_assert(sizeof(TPacketCGEmpireLuria) == 2, "TPacketCGEmpireLuria rozmiar musi wynosic 2 B");

    struct TPacketCGLogin2Luria
    {
        uint8_t header{ CG::LOGIN2 };
        char szLogin[31]{};
        uint32_t dwLoginKey{};
        uint32_t adwClientKeys[4]{};
    };
    static_assert(sizeof(TPacketCGLogin2Luria) == 52, "TPacketCGLogin2Luria rozmiar musi wynosic 52 B");

    struct TPacketCGAuthLuria
    {
        uint8_t header{ CG::AUTH_LOGIN };
        char szLogin[31]{};
        char szPassword[17]{};
        char szPin[5]{};
        char szToken[33]{};
        uint32_t adwClientKeys[4]{};
        char szHwidSha256[64]{};
        uint8_t bNullTerminator{};
    };
    static_assert(sizeof(TPacketCGAuthLuria) == 168, "TPacketCGAuthLuria rozmiar musi wynosic 168 B");

    struct TPacketCGClientVersionLuria
    {
        uint8_t header{ CG::CLIENT_VERSION };
        char szClientName[33]{};
        char szTimestamp[33]{};
    };
    static_assert(sizeof(TPacketCGClientVersionLuria) == 67, "TPacketCGClientVersionLuria rozmiar musi wynosic 67 B");

    struct TPacketCGKeyAgreementLuria
    {
        uint8_t header{ CG::KEY_AGREEMENT };
        uint16_t wType{};
        uint16_t wLength{};
        uint8_t abData[256]{};
    };
    static_assert(sizeof(TPacketCGKeyAgreementLuria) == 261, "TPacketCGKeyAgreementLuria rozmiar musi wynosic 261 B");

    struct TPacketCGPongLuria
    {
        uint8_t header{ CG::PONG };
    };
    static_assert(sizeof(TPacketCGPongLuria) == 1, "TPacketCGPongLuria rozmiar musi wynosic 1 B");

    struct TPacketCGHandshakeLuria
    {
        uint8_t header{ CG::HANDSHAKE };
        uint32_t dwHandshake{};
        uint32_t dwTime{};
        int32_t lDelta{};
    };
    static_assert(sizeof(TPacketCGHandshakeLuria) == 13, "TPacketCGHandshakeLuria rozmiar musi wynosic 13 B");

    // --- Pakiety GC (Game -> Client) ---
    struct TPacketGCCharacterAddLuria
    {
        uint8_t header{ GC::CHARACTER_ADD };
        uint8_t payload[36]{};
    };
    static_assert(sizeof(TPacketGCCharacterAddLuria) == 37, "TPacketGCCharacterAddLuria rozmiar musi wynosic 37 B");

    struct TPacketGCCharacterDelLuria
    {
        uint8_t header{ GC::CHARACTER_DEL };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCCharacterDelLuria) == 5, "TPacketGCCharacterDelLuria rozmiar musi wynosic 5 B");

    struct TPacketGCMoveLuria
    {
        uint8_t header{ GC::CHARACTER_MOVE };
        uint8_t bFunc{};
        uint8_t bArg{};
        uint8_t bRot{};
        uint32_t dwVID{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
        uint32_t dwDuration{};
    };
    static_assert(sizeof(TPacketGCMoveLuria) == 24, "TPacketGCMoveLuria rozmiar musi wynosic 24 B");

    struct TPacketGCChatLuria
    {
        uint8_t header{ GC::CHAT };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCChatLuria) == 9, "TPacketGCChatLuria rozmiar musi wynosic 9 B");

    struct TPacketGCWhisperLuria
    {
        uint8_t header{ GC::WHISPER };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCWhisperLuria) == 3, "TPacketGCWhisperLuria rozmiar musi wynosic 3 B");

    struct TPacketGCLoginSuccess3Luria
    {
        uint8_t header{ GC::LOGIN_SUCCESS3 };
        uint8_t payload[296]{};
    };
    static_assert(sizeof(TPacketGCLoginSuccess3Luria) == 297, "TPacketGCLoginSuccess3Luria rozmiar musi wynosic 297 B");

    struct TPacketGCLoginFailureLuria
    {
        uint8_t header{ GC::LOGIN_FAILURE };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCLoginFailureLuria) == 10, "TPacketGCLoginFailureLuria rozmiar musi wynosic 10 B");

    struct TPacketGCGc08Luria
    {
        uint8_t header{ GC::GC_08 };
        uint8_t payload[80]{};
    };
    static_assert(sizeof(TPacketGCGc08Luria) == 81, "TPacketGCGc08Luria rozmiar musi wynosic 81 B");

    struct TPacketGCGc09Luria
    {
        uint8_t header{ GC::GC_09 };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCGc09Luria) == 2, "TPacketGCGc09Luria rozmiar musi wynosic 2 B");

    struct TPacketGCGc0aLuria
    {
        uint8_t header{ GC::GC_0A };
    };
    static_assert(sizeof(TPacketGCGc0aLuria) == 1, "TPacketGCGc0aLuria rozmiar musi wynosic 1 B");

    struct TPacketGCGc0bLuria
    {
        uint8_t header{ GC::GC_0B };
    };
    static_assert(sizeof(TPacketGCGc0bLuria) == 1, "TPacketGCGc0bLuria rozmiar musi wynosic 1 B");

    struct TPacketGCAttackLuria
    {
        uint8_t header{ GC::ATTACK };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCAttackLuria) == 10, "TPacketGCAttackLuria rozmiar musi wynosic 10 B");

    struct TPacketGCGc0dLuria
    {
        uint8_t header{ GC::GC_0D };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCGc0dLuria) == 5, "TPacketGCGc0dLuria rozmiar musi wynosic 5 B");

    struct TPacketGCDeadLuria
    {
        uint8_t header{ GC::DEAD };
        uint32_t dwVID{};
    };
    static_assert(sizeof(TPacketGCDeadLuria) == 5, "TPacketGCDeadLuria rozmiar musi wynosic 5 B");

    struct TPacketGCMainCharacterLuria
    {
        uint8_t header{ GC::MAIN_CHARACTER };
        uint8_t payload[46]{};
    };
    static_assert(sizeof(TPacketGCMainCharacterLuria) == 47, "TPacketGCMainCharacterLuria rozmiar musi wynosic 47 B");

    struct TPacketGCCharacterUpdateLuria
    {
        uint8_t header{ GC::CHARACTER_UPDATE };
        uint8_t payload[1020]{};
    };
    static_assert(sizeof(TPacketGCCharacterUpdateLuria) == 1021, "TPacketGCCharacterUpdateLuria rozmiar musi wynosic 1021 B");

    struct TPacketGCPointChangeLuria
    {
        uint8_t header{ GC::POINT_CHANGE };
        uint8_t payload[16]{};
    };
    static_assert(sizeof(TPacketGCPointChangeLuria) == 17, "TPacketGCPointChangeLuria rozmiar musi wynosic 17 B");

    struct TPacketGCGc12Luria
    {
        uint8_t header{ GC::GC_12 };
        uint8_t payload[6]{};
    };
    static_assert(sizeof(TPacketGCGc12Luria) == 7, "TPacketGCGc12Luria rozmiar musi wynosic 7 B");

    struct TPacketGCGc13Luria
    {
        uint8_t header{ GC::GC_13 };
        uint8_t payload[66]{};
    };
    static_assert(sizeof(TPacketGCGc13Luria) == 67, "TPacketGCGc13Luria rozmiar musi wynosic 67 B");

    struct TPacketGCGc14Luria
    {
        uint8_t header{ GC::GC_14 };
        uint8_t payload[53]{};
    };
    static_assert(sizeof(TPacketGCGc14Luria) == 54, "TPacketGCGc14Luria rozmiar musi wynosic 54 B");

    struct TPacketGCGc15Luria
    {
        uint8_t header{ GC::GC_15 };
        uint8_t payload[63]{};
    };
    static_assert(sizeof(TPacketGCGc15Luria) == 64, "TPacketGCGc15Luria rozmiar musi wynosic 64 B");

    struct TPacketGCGc16Luria
    {
        uint8_t header{ GC::GC_16 };
        uint8_t payload[15]{};
    };
    static_assert(sizeof(TPacketGCGc16Luria) == 16, "TPacketGCGc16Luria rozmiar musi wynosic 16 B");

    struct TPacketGCGc19Luria
    {
        uint8_t header{ GC::GC_19 };
        uint8_t payload[50]{};
    };
    static_assert(sizeof(TPacketGCGc19Luria) == 51, "TPacketGCGc19Luria rozmiar musi wynosic 51 B");

    struct TPacketGCGc1aLuria
    {
        uint8_t header{ GC::GC_1A };
        uint8_t payload[20]{};
    };
    static_assert(sizeof(TPacketGCGc1aLuria) == 21, "TPacketGCGc1aLuria rozmiar musi wynosic 21 B");

    struct TPacketGCGc1bLuria
    {
        uint8_t header{ GC::GC_1B };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCGc1bLuria) == 5, "TPacketGCGc1bLuria rozmiar musi wynosic 5 B");

    struct TPacketGCGc1cLuria
    {
        uint8_t header{ GC::GC_1C };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc1cLuria) == 4, "TPacketGCGc1cLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc1dLuria
    {
        uint8_t header{ GC::GC_1D };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCGc1dLuria) == 2, "TPacketGCGc1dLuria rozmiar musi wynosic 2 B");

    struct TPacketGCGc1eLuria
    {
        uint8_t header{ GC::GC_1E };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCGc1eLuria) == 3, "TPacketGCGc1eLuria rozmiar musi wynosic 3 B");

    struct TPacketGCGc1fLuria
    {
        uint8_t header{ GC::GC_1F };
        uint8_t payload[29]{};
    };
    static_assert(sizeof(TPacketGCGc1fLuria) == 30, "TPacketGCGc1fLuria rozmiar musi wynosic 30 B");

    struct TPacketGCLoginSuccessLuria
    {
        uint8_t header{ GC::LOGIN_SUCCESS };
        uint8_t payload[488]{};
    };
    static_assert(sizeof(TPacketGCLoginSuccessLuria) == 489, "TPacketGCLoginSuccessLuria rozmiar musi wynosic 489 B");

    struct TPacketGCGc22Luria
    {
        uint8_t header{ GC::GC_22 };
        uint8_t payload[28]{};
    };
    static_assert(sizeof(TPacketGCGc22Luria) == 29, "TPacketGCGc22Luria rozmiar musi wynosic 29 B");

    struct TPacketGCGc24Luria
    {
        uint8_t header{ GC::GC_24 };
        uint8_t payload[10]{};
    };
    static_assert(sizeof(TPacketGCGc24Luria) == 11, "TPacketGCGc24Luria rozmiar musi wynosic 11 B");

    struct TPacketGCScriptLuria
    {
        uint8_t header{ GC::SCRIPT };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCScriptLuria) == 4, "TPacketGCScriptLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc27Luria
    {
        uint8_t header{ GC::GC_27 };
        uint8_t payload[37]{};
    };
    static_assert(sizeof(TPacketGCGc27Luria) == 38, "TPacketGCGc27Luria rozmiar musi wynosic 38 B");

    struct TPacketGCGc28Luria
    {
        uint8_t header{ GC::GC_28 };
        uint8_t payload[2]{};
    };
    static_assert(sizeof(TPacketGCGc28Luria) == 3, "TPacketGCGc28Luria rozmiar musi wynosic 3 B");

    struct TPacketGCTargetLuria
    {
        uint8_t header{ GC::TARGET };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCTargetLuria) == 10, "TPacketGCTargetLuria rozmiar musi wynosic 10 B");

    struct TPacketGCGc2aLuria
    {
        uint8_t header{ GC::GC_2A };
        uint8_t payload[62]{};
    };
    static_assert(sizeof(TPacketGCGc2aLuria) == 63, "TPacketGCGc2aLuria rozmiar musi wynosic 63 B");

    struct TPacketGCGc2bLuria
    {
        uint8_t header{ GC::GC_2B };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCGc2bLuria) == 6, "TPacketGCGc2bLuria rozmiar musi wynosic 6 B");

    struct TPacketGCPingLuria
    {
        uint8_t header{ GC::PING };
    };
    static_assert(sizeof(TPacketGCPingLuria) == 1, "TPacketGCPingLuria rozmiar musi wynosic 1 B");

    struct TPacketGCGc2dLuria
    {
        uint8_t header{ GC::GC_2D };
        uint8_t payload[5]{};
    };
    static_assert(sizeof(TPacketGCGc2dLuria) == 6, "TPacketGCGc2dLuria rozmiar musi wynosic 6 B");

    struct TPacketGCGc2eLuria
    {
        uint8_t header{ GC::GC_2E };
        uint8_t payload[73]{};
    };
    static_assert(sizeof(TPacketGCGc2eLuria) == 74, "TPacketGCGc2eLuria rozmiar musi wynosic 74 B");

    struct TPacketGCGc32Luria
    {
        uint8_t header{ GC::GC_32 };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc32Luria) == 4, "TPacketGCGc32Luria rozmiar musi wynosic 4 B");

    struct TPacketGCGc39Luria
    {
        uint8_t header{ GC::GC_39 };
    };
    static_assert(sizeof(TPacketGCGc39Luria) == 1, "TPacketGCGc39Luria rozmiar musi wynosic 1 B");

    struct TPacketGCGc3bLuria
    {
        uint8_t header{ GC::GC_3B };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc3bLuria) == 4, "TPacketGCGc3bLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc3cLuria
    {
        uint8_t header{ GC::GC_3C };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc3cLuria) == 4, "TPacketGCGc3cLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc3dLuria
    {
        uint8_t header{ GC::GC_3D };
        uint8_t payload[17]{};
    };
    static_assert(sizeof(TPacketGCGc3dLuria) == 18, "TPacketGCGc3dLuria rozmiar musi wynosic 18 B");

    struct TPacketGCGc3eLuria
    {
        uint8_t header{ GC::GC_3E };
        uint8_t payload[8]{};
    };
    static_assert(sizeof(TPacketGCGc3eLuria) == 9, "TPacketGCGc3eLuria rozmiar musi wynosic 9 B");

    struct TPacketGCGc3fLuria
    {
        uint8_t header{ GC::GC_3F };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketGCGc3fLuria) == 13, "TPacketGCGc3fLuria rozmiar musi wynosic 13 B");

    struct TPacketGCWarpLuria
    {
        uint8_t header{ GC::WARP };
        int32_t lX{};
        int32_t lY{};
        int32_t lAddr{};
        uint16_t wPort{};
    };
    static_assert(sizeof(TPacketGCWarpLuria) == 15, "TPacketGCWarpLuria rozmiar musi wynosic 15 B");

    struct TPacketGCGc45Luria
    {
        uint8_t header{ GC::GC_45 };
        uint8_t payload[16]{};
    };
    static_assert(sizeof(TPacketGCGc45Luria) == 17, "TPacketGCGc45Luria rozmiar musi wynosic 17 B");

    struct TPacketGCGc46Luria
    {
        uint8_t header{ GC::GC_46 };
        uint8_t payload[9]{};
    };
    static_assert(sizeof(TPacketGCGc46Luria) == 10, "TPacketGCGc46Luria rozmiar musi wynosic 10 B");

    struct TPacketGCGc47Luria
    {
        uint8_t header{ GC::GC_47 };
        uint8_t payload[16]{};
    };
    static_assert(sizeof(TPacketGCGc47Luria) == 17, "TPacketGCGc47Luria rozmiar musi wynosic 17 B");

    struct TPacketGCGc48Luria
    {
        uint8_t header{ GC::GC_48 };
        uint8_t payload[255]{};
    };
    static_assert(sizeof(TPacketGCGc48Luria) == 256, "TPacketGCGc48Luria rozmiar musi wynosic 256 B");

    struct TPacketGCGc49Luria
    {
        uint8_t header{ GC::GC_49 };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCGc49Luria) == 2, "TPacketGCGc49Luria rozmiar musi wynosic 2 B");

    struct TPacketGCGc4aLuria
    {
        uint8_t header{ GC::GC_4A };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc4aLuria) == 4, "TPacketGCGc4aLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc4bLuria
    {
        uint8_t header{ GC::GC_4B };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCGc4bLuria) == 4, "TPacketGCGc4bLuria rozmiar musi wynosic 4 B");

    struct TPacketGCGc4cLuria
    {
        uint8_t header{ GC::GC_4C };
        uint8_t payload[1530]{};
    };
    static_assert(sizeof(TPacketGCGc4cLuria) == 1531, "TPacketGCGc4cLuria rozmiar musi wynosic 1531 B");

    struct TPacketGCGc4dLuria
    {
        uint8_t header{ GC::GC_4D };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCGc4dLuria) == 5, "TPacketGCGc4dLuria rozmiar musi wynosic 5 B");

    struct TPacketGCGc4eLuria
    {
        uint8_t header{ GC::GC_4E };
        uint8_t payload[29]{};
    };
    static_assert(sizeof(TPacketGCGc4eLuria) == 30, "TPacketGCGc4eLuria rozmiar musi wynosic 30 B");

    struct TPacketGCGc4fLuria
    {
        uint8_t header{ GC::GC_4F };
        uint8_t payload[20]{};
    };
    static_assert(sizeof(TPacketGCGc4fLuria) == 21, "TPacketGCGc4fLuria rozmiar musi wynosic 21 B");

    struct TPacketGCGc50Luria
    {
        uint8_t header{ GC::GC_50 };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCGc50Luria) == 5, "TPacketGCGc50Luria rozmiar musi wynosic 5 B");

    struct TPacketGCEmpireLuria
    {
        uint8_t header{ GC::EMPIRE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCEmpireLuria) == 2, "TPacketGCEmpireLuria rozmiar musi wynosic 2 B");

    struct TPacketGCTargetHPLuria
    {
        uint8_t header{ GC::TARGET_HP };
        uint32_t dwTargetVID{};
        int64_t llCurrentHP{};
    };
    static_assert(sizeof(TPacketGCTargetHPLuria) == 13, "TPacketGCTargetHPLuria rozmiar musi wynosic 13 B");

    struct TPacketGCDamageInfoLuria
    {
        uint8_t header{ GC::DAMAGE_INFO };
        uint32_t dwVID{};
        uint8_t flag{};
        int32_t damage{};
    };
    static_assert(sizeof(TPacketGCDamageInfoLuria) == 10, "TPacketGCDamageInfoLuria rozmiar musi wynosic 10 B");

    struct TPacketGCAuthSuccessLuria
    {
        uint8_t header{ GC::AUTH_SUCCESS };
        uint32_t dwLoginKey{};
        uint8_t bResult{};
    };
    static_assert(sizeof(TPacketGCAuthSuccessLuria) == 6, "TPacketGCAuthSuccessLuria rozmiar musi wynosic 6 B");

    struct TPacketGCDynamic32Luria
    {
        uint8_t header{ GC::DYNAMIC_32 };
        uint8_t payload[4]{};
    };
    static_assert(sizeof(TPacketGCDynamic32Luria) == 5, "TPacketGCDynamic32Luria rozmiar musi wynosic 5 B");

    struct TPacketGCKeyAgreementAckLuria
    {
        uint8_t header{ GC::KEY_AGREEMENT_ACK };
        uint8_t payload[3]{};
    };
    static_assert(sizeof(TPacketGCKeyAgreementAckLuria) == 4, "TPacketGCKeyAgreementAckLuria rozmiar musi wynosic 4 B");

    struct TPacketGCKeyAgreementLuria
    {
        uint8_t header{ GC::KEY_AGREEMENT };
        uint8_t payload[260]{};
    };
    static_assert(sizeof(TPacketGCKeyAgreementLuria) == 261, "TPacketGCKeyAgreementLuria rozmiar musi wynosic 261 B");

    struct TPacketGCPhaseLuria
    {
        uint8_t header{ GC::PHASE };
        uint8_t payload[1]{};
    };
    static_assert(sizeof(TPacketGCPhaseLuria) == 2, "TPacketGCPhaseLuria rozmiar musi wynosic 2 B");

    struct TPacketGCPongLuria
    {
        uint8_t header{ GC::PONG };
        uint8_t payload[6]{};
    };
    static_assert(sizeof(TPacketGCPongLuria) == 7, "TPacketGCPongLuria rozmiar musi wynosic 7 B");

    struct TPacketGCHandshakeLuria
    {
        uint8_t header{ GC::HANDSHAKE };
        uint8_t payload[12]{};
    };
    static_assert(sizeof(TPacketGCHandshakeLuria) == 13, "TPacketGCHandshakeLuria rozmiar musi wynosic 13 B");

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
#include "luria_gc_table.inl"
        default:
            return false;
        }
    }

} // namespace Protocol::Generated::Luria
