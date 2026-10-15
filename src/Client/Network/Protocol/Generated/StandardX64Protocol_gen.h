#pragma once

// ============================================================================
// AUTOGEN PROTOCOL SPECIFICATION - Standard Metin2 x64 2026
// Single Source of Truth: docs/AI_PROTOCOL_MAP.json
// Standard: C++23 | Zero Padding | static_assert verified
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace Protocol::Generated::StandardX64
{
    // ========================================================================
    // Opcody CG (Client -> Game Server)
    // ========================================================================
    namespace CG
    {
        constexpr uint16_t CLIENT_VERSION           = 0x000D; // Wersja klienta
        constexpr uint16_t STATE_CHECKER            = 0x000F; // Sprawdzanie stanu serwera
        constexpr uint16_t TEXT                     = 0x0011; // Tekst kontrolny
        constexpr uint16_t LOGIN2                   = 0x0101; // Logowanie do serwera gry
        constexpr uint16_t LOGIN3                   = 0x0102; // Logowanie alternatywne
        constexpr uint16_t LOGIN_SECURE             = 0x0103; // Logowanie z podpisem
        constexpr uint16_t EMPIRE                   = 0x010A; // Wybor krolestwa
        constexpr uint16_t CHANGE_NAME              = 0x010B; // Zmiana nazwy postaci
        constexpr uint16_t CHARACTER_CREATE         = 0x0201; // Tworzenie nowej postaci
        constexpr uint16_t CHARACTER_DELETE         = 0x0202; // Usuniecie postaci
        constexpr uint16_t CHARACTER_SELECT         = 0x0203; // Wybor postaci ze slotu
        constexpr uint16_t ENTERGAME                = 0x0204; // Wejscie do swiata gry
        constexpr uint16_t MOVE                     = 0x0301; // Komenda ruchu gracza
        constexpr uint16_t SYNC_POSITION            = 0x0303; // Synchronizacja pozycji
        constexpr uint16_t WARP                     = 0x0305; // Teleportacja
        constexpr uint16_t ATTACK                   = 0x0401; // Komenda ataku
        constexpr uint16_t USE_SKILL                = 0x0402; // Uzycie umiejetnosci
        constexpr uint16_t SHOOT                    = 0x0403; // Strzal z luku
        constexpr uint16_t FLY_TARGETING            = 0x0404; // Namierzenie pocisku
        constexpr uint16_t ADD_FLY_TARGETING        = 0x0405; // Dodanie celu lotu
        constexpr uint16_t ITEM_USE                 = 0x0501; // Uzycie przedmiotu
        constexpr uint16_t ITEM_DROP                = 0x0502; // Wyrzucenie przedmiotu
        constexpr uint16_t ITEM_DROP2               = 0x0503; // Wyrzucenie zlota
        constexpr uint16_t ITEM_MOVE                = 0x0504; // Przeniesienie przedmiotu
        constexpr uint16_t ITEM_PICKUP              = 0x0505; // Podniesienie przedmiotu
        constexpr uint16_t ITEM_USE_TO_ITEM         = 0x0506; // Nalozenie przedmiotu na przedmiot
        constexpr uint16_t ITEM_GIVE                = 0x0507; // Przekazanie przedmiotu NPC
        constexpr uint16_t EXCHANGE                 = 0x0508; // Handel z innym graczem
        constexpr uint16_t QUICKSLOT_ADD            = 0x0509; // Dodanie do paska szybkiego dostepu
        constexpr uint16_t QUICKSLOT_DEL            = 0x050A; // Usuniecie z paska szybkiego dostepu
        constexpr uint16_t QUICKSLOT_SWAP           = 0x050B; // Zamiana na pasku szybkiego dostepu
        constexpr uint16_t REFINE                   = 0x050C; // Ulepszanie przedmiotu u kowala
        constexpr uint16_t DRAGON_SOUL_REFINE       = 0x050D; // Ulepszanie alchemii smoczych kamieni
        constexpr uint16_t CHAT                     = 0x0601; // Wiadomosc na czacie
        constexpr uint16_t WHISPER                  = 0x0602; // Wiadomosc prywatna
        constexpr uint16_t PARTY_INVITE             = 0x0701; // Zaproszenie do grupy
        constexpr uint16_t PARTY_INVITE_ANSWER      = 0x0702; // Odpowiedz na zaproszenie do grupy
        constexpr uint16_t PARTY_REMOVE             = 0x0703; // Wyrzucenie z grupy
        constexpr uint16_t PARTY_SET_STATE          = 0x0704; // Zmiana statusu grupy
        constexpr uint16_t PARTY_USE_SKILL          = 0x0705; // Umiejetnosc dowodzenia grupy
        constexpr uint16_t PARTY_PARAMETER          = 0x0706; // Parametry grupy
        constexpr uint16_t GUILD                    = 0x0720; // Akcje gildii
        constexpr uint16_t ANSWER_MAKE_GUILD        = 0x0721; // Zalozenie gildii
        constexpr uint16_t GUILD_SYMBOL_UPLOAD      = 0x0722; // Wgranie symbolu gildii
        constexpr uint16_t SYMBOL_CRC               = 0x0723; // Suma kontrolna symbolu
        constexpr uint16_t MESSENGER                = 0x0740; // Lista przyjaciol
        constexpr uint16_t SHOP                     = 0x0801; // Interakcja ze sklepem NPC
        constexpr uint16_t MYSHOP                   = 0x0802; // Otwarcie prywatnego sklepu
        constexpr uint16_t SAFEBOX_CHECKIN          = 0x0820; // Zlozenie przedmiotu w magazynie
        constexpr uint16_t SAFEBOX_CHECKOUT         = 0x0821; // Wyjecie przedmiotu z magazynu
        constexpr uint16_t SAFEBOX_ITEM_MOVE        = 0x0822; // Przeniesienie w magazynie
        constexpr uint16_t SAFEBOX_MONEY            = 0x0823; // Zlozenie/wyjecie zlota z magazynu
        constexpr uint16_t MALL_CHECKOUT            = 0x0840; // Magazyn Item Shop
        constexpr uint16_t SCRIPT_ANSWER            = 0x0901; // Odpowiedz w dialogu questowym
        constexpr uint16_t SCRIPT_BUTTON            = 0x0902; // Klikniecie przycisku questu
        constexpr uint16_t SCRIPT_SELECT_ITEM       = 0x0903; // Wybor przedmiotu w quescie
        constexpr uint16_t QUEST_INPUT_STRING       = 0x0904; // Wprowadzenie tekstu w quescie
        constexpr uint16_t QUEST_CONFIRM            = 0x0905; // Potwierdzenie questu
        constexpr uint16_t QUEST_CANCEL             = 0x0906; // Anulowanie questu
        constexpr uint16_t TARGET                   = 0x0A01; // Wybor celu
        constexpr uint16_t ON_CLICK                 = 0x0A02; // Klikniecie na byt w swiecie
        constexpr uint16_t CHARACTER_POSITION       = 0x0A60; // Pozycja kamery i postaci
        constexpr uint16_t FISHING                  = 0x0B01; // Lowienie ryb
        constexpr uint16_t DUNGEON                  = 0x0B02; // Akcje w lochach
        constexpr uint16_t MARK_LOGIN               = 0x0C01; // Pobieranie znakow gildii
        constexpr uint16_t MARK_CRCLIST             = 0x0C02; // Lista sum kontrolnych znakow
        constexpr uint16_t MARK_UPLOAD              = 0x0C03; // Wgranie znaku gildii
        constexpr uint16_t MARK_IDXLIST             = 0x0C04; // Indeks znakow gildii
    }

    // ========================================================================
    // Opcody GC (Game Server -> Client)
    // ========================================================================
    namespace GC
    {
        constexpr uint16_t RESPOND_CHANNELSTATUS    = 0x0010;
        constexpr uint16_t LOGIN_SUCCESS3           = 0x0104;
        constexpr uint16_t LOGIN_SUCCESS4           = 0x0105;
        constexpr uint16_t LOGIN_FAILURE            = 0x0106;
        constexpr uint16_t LOGIN_KEY                = 0x0107;
        constexpr uint16_t AUTH_SUCCESS             = 0x0108;
        constexpr uint16_t EMPIRE                   = 0x0109;
        constexpr uint16_t CHANGE_NAME              = 0x010C;
        constexpr uint16_t CHARACTER_ADD            = 0x0205;
        constexpr uint16_t CHARACTER_ADD2           = 0x0206;
        constexpr uint16_t CHAR_ADDITIONAL_INFO     = 0x0207;
        constexpr uint16_t CHARACTER_DEL            = 0x0208;
        constexpr uint16_t CHARACTER_UPDATE         = 0x0209;
        constexpr uint16_t CHARACTER_UPDATE2        = 0x020A;
        constexpr uint16_t CHARACTER_POSITION       = 0x020B;
        constexpr uint16_t PLAYER_CREATE_SUCCESS    = 0x020C;
        constexpr uint16_t PLAYER_CREATE_FAILURE    = 0x020D;
        constexpr uint16_t PLAYER_DELETE_SUCCESS    = 0x020E;
        constexpr uint16_t PLAYER_DELETE_WRONG_SOCIAL_ID = 0x020F;
        constexpr uint16_t MAIN_CHARACTER           = 0x0210;
        constexpr uint16_t PLAYER_POINTS            = 0x0214;
        constexpr uint16_t PLAYER_POINT_CHANGE      = 0x0215;
        constexpr uint16_t STUN                     = 0x0216;
        constexpr uint16_t DEAD                     = 0x0217;
        constexpr uint16_t CHANGE_SPEED             = 0x0218;
        constexpr uint16_t WALK_MODE                = 0x0219;
        constexpr uint16_t SKILL_LEVEL              = 0x021A;
        constexpr uint16_t SKILL_LEVEL_NEW          = 0x021B;
        constexpr uint16_t SKILL_COOLTIME_END       = 0x021C;
        constexpr uint16_t CHANGE_SKILL_GROUP       = 0x021D;
        constexpr uint16_t VIEW_EQUIP               = 0x021E;
        constexpr uint16_t MOVE                     = 0x0302;
        constexpr uint16_t SYNC_POSITION            = 0x0304;
        constexpr uint16_t WARP                     = 0x0306;
        constexpr uint16_t MOTION                   = 0x0307;
        constexpr uint16_t DIG_MOTION               = 0x0308;
        constexpr uint16_t DAMAGE_INFO              = 0x0410;
        constexpr uint16_t FLY_TARGETING            = 0x0411;
        constexpr uint16_t ADD_FLY_TARGETING        = 0x0412;
        constexpr uint16_t CREATE_FLY               = 0x0413;
        constexpr uint16_t PVP                      = 0x0414;
        constexpr uint16_t DUEL_START               = 0x0415;
        constexpr uint16_t ITEM_DEL                 = 0x0510;
        constexpr uint16_t ITEM_SET                 = 0x0511;
        constexpr uint16_t ITEM_USE                 = 0x0512;
        constexpr uint16_t ITEM_DROP                = 0x0513;
        constexpr uint16_t ITEM_UPDATE              = 0x0514;
        constexpr uint16_t ITEM_GROUND_ADD          = 0x0515;
        constexpr uint16_t ITEM_GROUND_DEL          = 0x0516;
        constexpr uint16_t ITEM_OWNERSHIP           = 0x0517;
        constexpr uint16_t ITEM_GET                 = 0x0518;
        constexpr uint16_t QUICKSLOT_ADD            = 0x0519;
        constexpr uint16_t QUICKSLOT_DEL            = 0x051A;
        constexpr uint16_t QUICKSLOT_SWAP           = 0x051B;
        constexpr uint16_t EXCHANGE                 = 0x051C;
        constexpr uint16_t REFINE_INFORMATION       = 0x051D;
        constexpr uint16_t REFINE_INFORMATION_NEW   = 0x051E;
        constexpr uint16_t DRAGON_SOUL_REFINE       = 0x051F;
        constexpr uint16_t CHAT                     = 0x0603;
        constexpr uint16_t WHISPER                  = 0x0604;
        constexpr uint16_t PARTY_INVITE             = 0x0710;
        constexpr uint16_t PARTY_ADD                = 0x0711;
        constexpr uint16_t PARTY_UPDATE             = 0x0712;
        constexpr uint16_t PARTY_REMOVE             = 0x0713;
        constexpr uint16_t PARTY_LINK               = 0x0714;
        constexpr uint16_t PARTY_UNLINK             = 0x0715;
        constexpr uint16_t PARTY_PARAMETER          = 0x0716;
        constexpr uint16_t GUILD                    = 0x0730;
        constexpr uint16_t REQUEST_MAKE_GUILD       = 0x0731;
        constexpr uint16_t SYMBOL_DATA              = 0x0732;
        constexpr uint16_t MESSENGER                = 0x0741;
        constexpr uint16_t LOVER_INFO               = 0x0750;
        constexpr uint16_t LOVE_POINT_UPDATE        = 0x0751;
        constexpr uint16_t SHOP                     = 0x0810;
        constexpr uint16_t SHOP_SIGN                = 0x0811;
        constexpr uint16_t SAFEBOX_SET              = 0x0830;
        constexpr uint16_t SAFEBOX_DEL              = 0x0831;
        constexpr uint16_t SAFEBOX_WRONG_PASSWORD   = 0x0832;
        constexpr uint16_t SAFEBOX_SIZE             = 0x0833;
        constexpr uint16_t SAFEBOX_MONEY_CHANGE     = 0x0834;
        constexpr uint16_t MALL_OPEN                = 0x0841;
        constexpr uint16_t MALL_SET                 = 0x0842;
        constexpr uint16_t MALL_DEL                 = 0x0843;
        constexpr uint16_t SCRIPT                   = 0x0910;
        constexpr uint16_t QUEST_CONFIRM            = 0x0911;
        constexpr uint16_t QUEST_INFO               = 0x0912;
        constexpr uint16_t TARGET                   = 0x0A10;
        constexpr uint16_t TARGET_UPDATE            = 0x0A11;
        constexpr uint16_t TARGET_DELETE            = 0x0A12;
        constexpr uint16_t TARGET_CREATE_NEW        = 0x0A13;
        constexpr uint16_t AFFECT_ADD               = 0x0A20;
        constexpr uint16_t AFFECT_REMOVE            = 0x0A21;
        constexpr uint16_t SEPCIAL_EFFECT           = 0x0A30;
        constexpr uint16_t SPECIFIC_EFFECT          = 0x0A31;
        constexpr uint16_t MOUNT                    = 0x0A40;
        constexpr uint16_t OWNERSHIP                = 0x0A41;
        constexpr uint16_t NPC_POSITION             = 0x0A50;
        constexpr uint16_t FISHING                  = 0x0B10;
        constexpr uint16_t DUNGEON                  = 0x0B11;
        constexpr uint16_t LAND_LIST                = 0x0B12;
        constexpr uint16_t TIME                     = 0x0B13;
        constexpr uint16_t CHANNEL                  = 0x0B14;
        constexpr uint16_t MARK_UPDATE              = 0x0B15;
        constexpr uint16_t OBSERVER_ADD             = 0x0B20;
        constexpr uint16_t OBSERVER_REMOVE          = 0x0B21;
        constexpr uint16_t OBSERVER_MOVE            = 0x0B22;
        constexpr uint16_t MARK_BLOCK               = 0x0C10;
        constexpr uint16_t MARK_IDXLIST             = 0x0C11;
        constexpr uint16_t MARK_DIFF_DATA           = 0x0C12;
    }

    // ========================================================================
    // Struktury Pakietow (#pragma pack 1)
    // ========================================================================
#pragma pack(push, 1)

    // --- Pakiety CG (Client -> Game) ---
    struct TPacketCGMove
    {
        uint16_t header{ CG::MOVE };
        uint16_t length{ 19 };
        uint8_t bFunc{};
        uint8_t bArg{};
        uint8_t bRot{};
        int32_t lX{};
        int32_t lY{};
        uint32_t dwTime{};
    };
    static_assert(sizeof(TPacketCGMove) == 19, "TPacketCGMove rozmiar musi wynosic 19 B");

    struct TPacketCGAttack
    {
        uint16_t header{ CG::ATTACK };
        uint16_t length{ 11 };
        uint8_t bType{};
        uint32_t dwVictimVID{};
        uint8_t bCRCMagicCubeProcPiece{};
        uint8_t bCRCMagicCubeFilePiece{};
    };
    static_assert(sizeof(TPacketCGAttack) == 11, "TPacketCGAttack rozmiar musi wynosic 11 B");

    struct TPacketCGItemUse
    {
        uint16_t header{ CG::ITEM_USE };
        uint16_t length{ 6 };
        uint8_t window{};
        uint8_t cell{};
    };
    static_assert(sizeof(TPacketCGItemUse) == 6, "TPacketCGItemUse rozmiar musi wynosic 6 B");

    struct TPacketCGTarget
    {
        uint16_t header{ CG::TARGET };
        uint16_t length{ 8 };
        uint32_t targetVid{};
    };
    static_assert(sizeof(TPacketCGTarget) == 8, "TPacketCGTarget rozmiar musi wynosic 8 B");

    // --- Pakiety GC (Game -> Client) ---
#pragma pack(pop)

    // ========================================================================
    // Tabela Informacji o Pakietach GC
    // ========================================================================
    struct GCInfo
    {
        uint32_t size{0};
        bool isDynamic{false};
    };

    inline bool GetGCInfo(uint16_t header, GCInfo& info)
    {
        switch (header)
        {
#include "standard_x64_gc_table.inl"
        default:
            return false;
        }
    }

} // namespace Protocol::Generated::StandardX64
