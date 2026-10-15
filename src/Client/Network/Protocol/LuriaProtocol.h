#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

namespace Luria
{
    // ========================================================================
    // Opcody CG (Client -> Game Server) dla Lurii
    // ========================================================================
    namespace CG
    {
        constexpr uint8_t LOGIN            = 0x01; // 49B (naglowek, login 30B, haslo 16B)
        constexpr uint8_t ATTACK           = 0x02; // 8B (TPacketCGAttackLuria)
        constexpr uint8_t MOVE             = 0x03; // 19B (TPacketCGMoveLuria)
        constexpr uint8_t CHAT             = 0x04; // Dynamiczny pakiet czatu
        constexpr uint8_t CHARACTER_CREATE = 0x04; // 34B (tworzenie postaci)
        constexpr uint8_t CHARACTER_DELETE = 0x05; // 10B (usuniecie postaci kodem PIN)
        constexpr uint8_t CHARACTER_SELECT = 0x06; // 2B (wybor postaci ze slotu)
        constexpr uint8_t ENTERGAME        = 0x0A; // 1B (wejscie do gry)
        constexpr uint8_t ITEM_USE         = 0x0B; // 4B (TPacketCGItemUseLuria)
        constexpr uint8_t ITEM_DROP        = 0x0C; // 8B (TPacketCGItemDropLuria)
        constexpr uint8_t ITEM_MOVE        = 0x0D; // 9B (TPacketCGItemMoveLuria)
        constexpr uint8_t ITEM_PICKUP      = 0x0F; // 5B (TPacketCGItemPickUpLuria)
        constexpr uint8_t ON_CLICK         = 0x14; // Klikniecie na obiekt (6B)
        constexpr uint8_t TARGET           = 0x3D; // 5B (TPacketCGTargetLuria)
        constexpr uint8_t EMPIRE           = 0x5A; // 2B (wybor krolestwa)
        constexpr uint8_t LOGIN2           = 0x6D; // 52B (gra: login 30B, loginKey 4B, hwid 16B)
        constexpr uint8_t AUTH_LOGIN       = 0x6F; // 168B (autoryzacja: login, haslo, pin, hwid sha256)
        constexpr uint8_t CLIENT_VERSION   = 0xF1; // 67B (TPacketCGClientVersionLuria)
        constexpr uint8_t KEY_AGREEMENT    = 0xFB; // 261B (TPacketCGKeyAgreementLuria)
        constexpr uint8_t PONG             = 0xFE; // 1B (odpowiedz keepalive na PING 0x2C)
        constexpr uint8_t HANDSHAKE        = 0xFF; // 13B (TPacketCGHandshakeLuria)
    }

    // ========================================================================
    // Opcody GC (Game Server -> Client) dla Lurii (137 pakietow w rejestrze)
    // ========================================================================
    namespace GC
    {
        constexpr uint8_t CHARACTER_ADD       = 0x01; // 37B (dodanie postaci w widoku)
        constexpr uint8_t CHARACTER_DEL       = 0x02; // 5B (usuniecie postaci)
        constexpr uint8_t CHARACTER_MOVE      = 0x03; // 24B (ruch postaci)
        constexpr uint8_t CHAT                = 0x04; // Dynamiczny (bazowy naglowek 9B)
        constexpr uint8_t WHISPER             = 0x05; // Dynamiczny (bazowy naglowek 3B)
        constexpr uint8_t LOGIN_SUCCESS3      = 0x06; // 297B
        constexpr uint8_t LOGIN_FAILURE       = 0x07; // 10B
        constexpr uint8_t ATTACK              = 0x0C; // 10B
        constexpr uint8_t DEAD                = 0x0E; // 5B
        constexpr uint8_t MAIN_CHARACTER      = 0x0F; // 47B (dane glownej postaci po wejsciu)
        constexpr uint8_t CHARACTER_UPDATE    = 0x10; // 1021B
        constexpr uint8_t POINT_CHANGE        = 0x11; // 17B (aktualizacja statystyk)
        constexpr uint8_t LOGIN_SUCCESS       = 0x20; // 489B (5 slotow postaci)
        constexpr uint8_t PING                = 0x2C; // 1B (keepalive ping serwera)
        constexpr uint8_t WARP                = 0x41; // 15B (teleportacja i koordynaty bazy)
        constexpr uint8_t EMPIRE              = 0x5A; // 2B (krolestwo)
        constexpr uint8_t TARGET_HP           = 0x6C; // 13B
        constexpr uint8_t DAMAGE_INFO         = 0x87; // 10B
        constexpr uint8_t AUTH_SUCCESS        = 0x96; // 6B (dwLoginKey 4B, wynik 1B)
        constexpr uint8_t DYNAMIC_32          = 0xBE; // 5B (pakiet dynamiczny z 32-bitowym rozmiarem)
        constexpr uint8_t KEY_AGREEMENT_ACK   = 0xFA; // 4B (potwierdzenie uzgodnienia kluczy DH2)
        constexpr uint8_t KEY_AGREEMENT       = 0xFB; // 261B (oferta klucza publicznego DH2)
        constexpr uint8_t PHASE               = 0xFD; // 2B (zmiana fazy polaczenia)
        constexpr uint8_t PONG                = 0xFE; // 7B
        constexpr uint8_t HANDSHAKE           = 0xFF; // 13B (pakiet synchronizacji czasu)
    }

    // ========================================================================
    // Fazy polaczenia sieciowego Lurii
    // ========================================================================
    enum class Phase : uint8_t
    {
        Close     = 0,
        Handshake = 1,
        Login     = 2,
        Select    = 3,
        Loading   = 4,
        Game      = 5,
        Dead      = 6,
        Auth      = 10
    };

    // ========================================================================
    // Tablica XOR CRC i algorytm zabezpieczenia pakietu ataku Lurii
    // ========================================================================
    inline constexpr uint8_t CRC_XOR_TABLE[8] = { 102, 30, 188, 44, 39, 201, 43, 5 };

    inline uint8_t GetLuriaAttackProcPiece(uint32_t seq)
    {
        return CRC_XOR_TABLE[seq % 8];
    }

    // ========================================================================
    // Struktury Pakietow Lurii (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    // TPacketCGAttackLuria (8B)
    struct TPacketCGAttackLuria
    {
        uint8_t  header{ CG::ATTACK };             // 0x02
        uint8_t  bType;                            // Typ ataku
        uint32_t dwVictimVID;                      // VID ofiary
        uint8_t  bCRCMagicCubeProcPiece;           // CRC Magic Cube Proc Piece
        uint8_t  bCRCMagicCubeFilePiece;           // CRC Magic Cube File Piece
    };
    static_assert(sizeof(TPacketCGAttackLuria) == 8, "TPacketCGAttackLuria must be 8 bytes");

    // TPacketCGMoveLuria (19B)
    struct TPacketCGMoveLuria
    {
        uint8_t  header{ CG::MOVE };               // 0x03
        uint8_t  bFunc;                            // Funkcja ruchu (0=WAIT, 1=MOVE, 2=ROTATE, 3=COMBO)
        uint8_t  bArg;                             // Argument
        float    fRot;                             // Kat rotacji (4B float)
        int32_t  lX;                               // Koordynat globalny X (4B int32_t)
        int32_t  lY;                               // Koordynat globalny Y (4B int32_t)
        uint32_t dwTime;                           // Znacznik czasu klienta (4B uint32_t)
    };
    static_assert(sizeof(TPacketCGMoveLuria) == 19, "TPacketCGMoveLuria must be 19 bytes");

    // TPacketCGClientVersionLuria (67B)
    struct TPacketCGClientVersionLuria
    {
        uint8_t  header{ CG::CLIENT_VERSION };     // 0xF1 (1B)
        char     szClientName[33]{};               // Nazwa klienta (33B, null-terminated "metin2client.exe")
        char     szTimestamp[33]{};                // Timestamp (33B, null-terminated "1215955205")
    };
    static_assert(sizeof(TPacketCGClientVersionLuria) == 67, "TPacketCGClientVersionLuria must be 67 bytes");

    // TPacketCGItemUseLuria (4B)
    struct TPacketCGItemUseLuria
    {
        uint8_t  header{ CG::ITEM_USE };           // 0x0B
        uint8_t  window_type;                      // Typ okna ekwipunku
        uint16_t cell;                             // Slot / komorka przedmiotu
    };
    static_assert(sizeof(TPacketCGItemUseLuria) == 4, "TPacketCGItemUseLuria must be 4 bytes");

    // TPacketCGTargetLuria (5B)
    struct TPacketCGTargetLuria
    {
        uint8_t  header{ CG::TARGET };             // 0x3D
        uint32_t targetVid;                        // VID namierzanego celu
    };
    static_assert(sizeof(TPacketCGTargetLuria) == 5, "TPacketCGTargetLuria must be 5 bytes");

    // TPacketCGHandshakeLuria (13B)
    struct TPacketCGHandshakeLuria
    {
        uint8_t  header{ CG::HANDSHAKE };          // 0xFF
        uint32_t dwHandshake;                      // Ziarno / token handshake serwera
        uint32_t dwTime;                           // Czas klienta
        int32_t  lDelta;                           // Delta pomiedzy zegarami
    };
    static_assert(sizeof(TPacketCGHandshakeLuria) == 13, "TPacketCGHandshakeLuria must be 13 bytes");

    // TPacketCGKeyAgreementLuria (261B)
    struct TPacketCGKeyAgreementLuria
    {
        uint8_t  header{ CG::KEY_AGREEMENT };      // 0xFB
        uint16_t wType;                            // Typ algorytmu / dlugosc klucza (np. 256)
        uint16_t wLength;                          // Rozmiar danych klucza publicznego
        uint8_t  abData[256];                      // Bufor danych klucza (256B)
    };
    static_assert(sizeof(TPacketCGKeyAgreementLuria) == 261, "TPacketCGKeyAgreementLuria must be 261 bytes");

    // TPacketCGAuthLuria (168B) - Autoryzacja Luria z kodem PIN i HWID
    struct TPacketCGAuthLuria
    {
        uint8_t  header{ CG::AUTH_LOGIN };         // 0x6F
        char     szLogin[31];                      // Login (31B)
        char     szPassword[17];                   // Haslo (17B)
        char     szPin[5];                         // Kod PIN (5B, 4 cyfry + null)
        char     szToken[33];                      // Token autoryzacyjny ("STARRS3543S22", 33B)
        uint32_t adwClientKeys[4];                 // Klucze sprzetowe klienta (16B)
        char     szHwidSha256[64];                 // Skrot SHA-256 HWID (64B)
        uint8_t  bNullTerminator{0};               // Bajt koncowy 0x00 (1B)
    };
    static_assert(sizeof(TPacketCGAuthLuria) == 168, "TPacketCGAuthLuria must be 168 bytes");

    // TPacketCGLogin2Luria (52B) - Logowanie do serwera gry
    struct TPacketCGLogin2Luria
    {
        uint8_t  header{ CG::LOGIN2 };             // 0x6D
        char     szLogin[31];                      // Login (31B)
        uint32_t dwLoginKey;                       // Klucz sesji logowania z Auth (4B)
        uint32_t adwClientKeys[4];                 // Klucze sprzetowe / identyfikator klienta (16B)
    };
    static_assert(sizeof(TPacketCGLogin2Luria) == 52, "TPacketCGLogin2Luria must be 52 bytes");

    // ========================================================================
    // Struktury Pakietow GC (Game -> Client) dla Lurii
    // ========================================================================

    // TPacketGCMoveLuria (24B)
    struct TPacketGCMoveLuria
    {
        uint8_t  header{ GC::CHARACTER_MOVE };     // 0x03
        uint8_t  bFunc;                            // Funkcja ruchu
        uint8_t  bArg;                             // Argument
        uint8_t  bRot;                             // Rotacja
        uint32_t dwVID;                            // VID poruszajacej sie postaci
        int32_t  lX;                               // Koordynat globalny X (4B int32_t)
        int32_t  lY;                               // Koordynat globalny Y (4B int32_t)
        uint32_t dwTime;                           // Znacznik czasu klienta (4B uint32_t)
        uint32_t dwDuration;                       // Czas trwania animacji ruchu
    };
    static_assert(sizeof(TPacketGCMoveLuria) == 24, "TPacketGCMoveLuria must be 24 bytes");

    // TPacketGCDeadLuria (5B)
    struct TPacketGCDeadLuria
    {
        uint8_t  header{ GC::DEAD };               // 0x0E
        uint32_t dwVID;                            // VID martwego bytu
    };
    static_assert(sizeof(TPacketGCDeadLuria) == 5, "TPacketGCDeadLuria must be 5 bytes");

    // TPacketGCDamageInfoLuria (10B)
    struct TPacketGCDamageInfoLuria
    {
        uint8_t  header{ GC::DAMAGE_INFO };        // 0x87
        uint32_t dwVID;                            // VID celu
        uint8_t  flag;                             // Flaga obrazen
        int32_t  damage;                           // Wartosc obrazen
    };
    static_assert(sizeof(TPacketGCDamageInfoLuria) == 10, "TPacketGCDamageInfoLuria must be 10 bytes");

    // TPacketGCTargetHPLuria (13B)
    struct TPacketGCTargetHPLuria
    {
        uint8_t  header{ GC::TARGET_HP };          // 0x6C
        uint32_t dwTargetVID;                      // VID celu
        int64_t  llCurrentHP;                      // Wartosc HP
    };
    static_assert(sizeof(TPacketGCTargetHPLuria) == 13, "TPacketGCTargetHPLuria must be 13 bytes");

    // TPacketGCAuthSuccessLuria (6B)
    struct TPacketGCAuthSuccessLuria
    {
        uint8_t  header{ GC::AUTH_SUCCESS };       // 0x96
        uint32_t dwLoginKey;                       // Klucz sesji logowania (4B)
        uint8_t  bResult;                          // Wynik autoryzacji (1B)
    };
    static_assert(sizeof(TPacketGCAuthSuccessLuria) == 6, "TPacketGCAuthSuccessLuria must be 6 bytes");

    // TPacketCGSelectCharacterLuria (2B)
    struct TPacketCGSelectCharacterLuria
    {
        uint8_t header{ CG::CHARACTER_SELECT }; // 0x06
        uint8_t index;                          // Numer slotu postaci (0..4)
    };
    static_assert(sizeof(TPacketCGSelectCharacterLuria) == 2, "TPacketCGSelectCharacterLuria must be 2 bytes");

    // TPacketCGEmpireLuria (2B)
    struct TPacketCGEmpireLuria
    {
        uint8_t header{ CG::EMPIRE };           // 0x5A
        uint8_t empire;                         // Numer krolestwa (1..3)
    };
    static_assert(sizeof(TPacketCGEmpireLuria) == 2, "TPacketCGEmpireLuria must be 2 bytes");

    // TPacketCGDestroyCharacterLuria (10B)
    struct TPacketCGDestroyCharacterLuria
    {
        uint8_t header{ CG::CHARACTER_DELETE }; // 0x05
        uint8_t index;                          // Numer slotu (1B)
        char    szPrivateCode[8];               // Kod PIN usuniecia postaci (8B)
    };
    static_assert(sizeof(TPacketCGDestroyCharacterLuria) == 10, "TPacketCGDestroyCharacterLuria must be 10 bytes");

    // TPacketCGCreateCharacterLuria (34B)
    struct TPacketCGCreateCharacterLuria
    {
        uint8_t  header{ CG::CHARACTER_CREATE }; // 0x04
        uint8_t  index;                          // Numer slotu (1B)
        char     name[25];                       // Nazwa postaci (25B)
        uint16_t job;                            // Klasa postaci (2B ushort)
        uint8_t  shape;                          // Wyglad (1B)
        uint8_t  CON;                            // Witalnosc (1B)
        uint8_t  INT;                            // Inteligencja (1B)
        uint8_t  STR;                            // Sila (1B)
        uint8_t  DEX;                            // Zrecznosc (1B)
    };
    static_assert(sizeof(TPacketCGCreateCharacterLuria) == 34, "TPacketCGCreateCharacterLuria must be 34 bytes");

    // TPacketCGItemDropLuria (8B)
    struct TPacketCGItemDropLuria
    {
        uint8_t  header{ CG::ITEM_DROP };       // 0x0C
        uint8_t  window_type;                   // Okno (1B)
        uint16_t cell;                          // Slot (2B)
        uint32_t count;                         // Liczba / zloto (4B)
    };
    static_assert(sizeof(TPacketCGItemDropLuria) == 8, "TPacketCGItemDropLuria must be 8 bytes");

    // TPacketCGItemMoveLuria (9B)
    struct TPacketCGItemMoveLuria
    {
        uint8_t  header{ CG::ITEM_MOVE };       // 0x0D
        uint8_t  srcWindow;                     // Okno zrodlowe (1B)
        uint16_t srcCell;                       // Slot zrodlowy (2B)
        uint8_t  dstWindow;                     // Okno docelowe (1B)
        uint16_t dstCell;                       // Slot docelowy (2B)
        uint16_t count;                         // Liczba sztuk (2B)
    };
    static_assert(sizeof(TPacketCGItemMoveLuria) == 9, "TPacketCGItemMoveLuria must be 9 bytes");

    // TPacketCGItemPickUpLuria (5B)
    struct TPacketCGItemPickUpLuria
    {
        uint8_t  header{ CG::ITEM_PICKUP };     // 0x0F
        uint32_t dwVID;                         // VID przedmiotu lezacego na ziemi (4B)
    };
    static_assert(sizeof(TPacketCGItemPickUpLuria) == 5, "TPacketCGItemPickUpLuria must be 5 bytes");

    // TPacketGCWarpLuria (15B)
    struct TPacketGCWarpLuria
    {
        uint8_t  header{ GC::WARP };             // 0x41
        int32_t  lX;                             // Koordynat globalny X (4B)
        int32_t  lY;                             // Koordynat globalny Y (4B)
        int32_t  lAddr;                          // Adres IP serwera docelowego (4B)
        uint16_t wPort;                          // Port serwera docelowego (2B)
    };
    static_assert(sizeof(TPacketGCWarpLuria) == 15, "TPacketGCWarpLuria must be 15 bytes");

#pragma pack(pop)

    // ========================================================================
    // Tabela Informacji o Pakietach GC (137 zarejestrowanych pakietow)
    // ========================================================================
    struct LuriaGCInfo
    {
        uint32_t size;
        bool isDynamic;
    };

    inline bool GetGCInfo(uint8_t header, LuriaGCInfo& info)
    {
        switch (header)
        {
#include "luria_gc_table.inl"
        default:
            info = { 2, false };
            return true;
        }
    }

} // namespace Luria
