#pragma once

#include <cstdint>

namespace Client::Network::Protocol
{
    // ========================================================================
    // CG -- Client -> Game (Naglowki pakietow wychodzacych z klienta)
    // ========================================================================
    namespace CG
    {
        // Control
        constexpr uint16_t CLIENT_VERSION     = 0x000D;
        constexpr uint16_t STATE_CHECKER      = 0x000F;
        constexpr uint16_t TEXT               = 0x0011;

        // Authentication
        constexpr uint16_t LOGIN2             = 0x0101;
        constexpr uint16_t LOGIN3             = 0x0102;
        constexpr uint16_t LOGIN_SECURE       = 0x0103;
        constexpr uint16_t EMPIRE             = 0x010A;
        constexpr uint16_t CHANGE_NAME        = 0x010B;

        // Character
        constexpr uint16_t CHARACTER_CREATE   = 0x0201;
        constexpr uint16_t CHARACTER_DELETE   = 0x0202;
        constexpr uint16_t CHARACTER_SELECT   = 0x0203;
        constexpr uint16_t ENTERGAME          = 0x0204;

        // Movement
        constexpr uint16_t MOVE               = 0x0301;
        constexpr uint16_t SYNC_POSITION      = 0x0303;
        constexpr uint16_t WARP               = 0x0305;

        // Combat
        constexpr uint16_t ATTACK             = 0x0401;
        constexpr uint16_t USE_SKILL          = 0x0402;
        constexpr uint16_t SHOOT              = 0x0403;
        constexpr uint16_t FLY_TARGETING      = 0x0404;
        constexpr uint16_t ADD_FLY_TARGETING  = 0x0405;

        // Items
        constexpr uint16_t ITEM_USE           = 0x0501;
        constexpr uint16_t ITEM_DROP          = 0x0502;
        constexpr uint16_t ITEM_DROP2         = 0x0503;
        constexpr uint16_t ITEM_MOVE          = 0x0504;
        constexpr uint16_t ITEM_PICKUP        = 0x0505;
        constexpr uint16_t ITEM_USE_TO_ITEM   = 0x0506;
        constexpr uint16_t ITEM_GIVE          = 0x0507;
        constexpr uint16_t EXCHANGE           = 0x0508;
        constexpr uint16_t QUICKSLOT_ADD      = 0x0509;
        constexpr uint16_t QUICKSLOT_DEL      = 0x050A;
        constexpr uint16_t QUICKSLOT_SWAP     = 0x050B;
        constexpr uint16_t REFINE             = 0x050C;
        constexpr uint16_t DRAGON_SOUL_REFINE = 0x050D;

        // Chat
        constexpr uint16_t CHAT               = 0x0601;
        constexpr uint16_t WHISPER            = 0x0602;

        // Social
        constexpr uint16_t PARTY_INVITE       = 0x0701;
        constexpr uint16_t PARTY_INVITE_ANSWER = 0x0702;
        constexpr uint16_t PARTY_REMOVE       = 0x0703;
        constexpr uint16_t PARTY_SET_STATE    = 0x0704;
        constexpr uint16_t PARTY_USE_SKILL    = 0x0705;
        constexpr uint16_t PARTY_PARAMETER    = 0x0706;
        constexpr uint16_t GUILD              = 0x0720;
        constexpr uint16_t ANSWER_MAKE_GUILD  = 0x0721;
        constexpr uint16_t GUILD_SYMBOL_UPLOAD = 0x0722;
        constexpr uint16_t SYMBOL_CRC         = 0x0723;
        constexpr uint16_t MESSENGER          = 0x0740;

        // Shop / Safebox / Mall
        constexpr uint16_t SHOP               = 0x0801;
        constexpr uint16_t MYSHOP             = 0x0802;
        constexpr uint16_t SAFEBOX_CHECKIN    = 0x0820;
        constexpr uint16_t SAFEBOX_CHECKOUT   = 0x0821;
        constexpr uint16_t SAFEBOX_ITEM_MOVE  = 0x0822;
        constexpr uint16_t SAFEBOX_MONEY      = 0x0823;
        constexpr uint16_t MALL_CHECKOUT      = 0x0840;

        // Quest
        constexpr uint16_t SCRIPT_ANSWER      = 0x0901;
        constexpr uint16_t SCRIPT_BUTTON      = 0x0902;
        constexpr uint16_t SCRIPT_SELECT_ITEM = 0x0903;
        constexpr uint16_t QUEST_INPUT_STRING = 0x0904;
        constexpr uint16_t QUEST_CONFIRM      = 0x0905;
        constexpr uint16_t QUEST_CANCEL       = 0x0906;

        // UI / Targeting
        constexpr uint16_t TARGET             = 0x0A01;
        constexpr uint16_t ON_CLICK           = 0x0A02;
        constexpr uint16_t CHARACTER_POSITION = 0x0A60;

        // World
        constexpr uint16_t FISHING            = 0x0B01;
        constexpr uint16_t DUNGEON            = 0x0B02;
        constexpr uint16_t HACK               = 0x0B03;

        // Guild Marks
        constexpr uint16_t MARK_LOGIN         = 0x0C01;
        constexpr uint16_t MARK_CRCLIST       = 0x0C02;
        constexpr uint16_t MARK_UPLOAD        = 0x0C03;
        constexpr uint16_t MARK_IDXLIST       = 0x0C04;
    }

    // ========================================================================
    // GC -- Game -> Client (Naglowki pakietow przychodzacych z serwera)
    // ========================================================================
    namespace GC
    {
        // Control
        constexpr uint16_t RESPOND_CHANNELSTATUS = 0x0010;

        // Authentication
        constexpr uint16_t LOGIN_SUCCESS3     = 0x0104;
        constexpr uint16_t LOGIN_SUCCESS4     = 0x0105;
        constexpr uint16_t LOGIN_FAILURE      = 0x0106;
        constexpr uint16_t LOGIN_KEY          = 0x0107;
        constexpr uint16_t AUTH_SUCCESS       = 0x0108;
        constexpr uint16_t EMPIRE             = 0x0109;
        constexpr uint16_t CHANGE_NAME        = 0x010C;

        // Character
        constexpr uint16_t CHARACTER_ADD      = 0x0205;
        constexpr uint16_t CHARACTER_ADD2     = 0x0206;
        constexpr uint16_t CHAR_ADDITIONAL_INFO = 0x0207;
        constexpr uint16_t CHARACTER_DEL      = 0x0208;
        constexpr uint16_t CHARACTER_UPDATE   = 0x0209;
        constexpr uint16_t CHARACTER_UPDATE2  = 0x020A;
        constexpr uint16_t CHARACTER_POSITION = 0x020B;
        constexpr uint16_t PLAYER_CREATE_SUCCESS = 0x020C;
        constexpr uint16_t PLAYER_CREATE_FAILURE = 0x020D;
        constexpr uint16_t PLAYER_DELETE_SUCCESS = 0x020E;
        constexpr uint16_t PLAYER_DELETE_WRONG_SOCIAL_ID = 0x020F;
        constexpr uint16_t MAIN_CHARACTER     = 0x0210;
        constexpr uint16_t PLAYER_POINTS      = 0x0214;
        constexpr uint16_t PLAYER_POINT_CHANGE = 0x0215;
        constexpr uint16_t STUN               = 0x0216;
        constexpr uint16_t DEAD               = 0x0217;
        constexpr uint16_t CHANGE_SPEED       = 0x0218;
        constexpr uint16_t WALK_MODE          = 0x0219;
        constexpr uint16_t SKILL_LEVEL        = 0x021A;
        constexpr uint16_t SKILL_LEVEL_NEW    = 0x021B;
        constexpr uint16_t SKILL_COOLTIME_END = 0x021C;
        constexpr uint16_t CHANGE_SKILL_GROUP = 0x021D;
        constexpr uint16_t VIEW_EQUIP         = 0x021E;

        // Movement
        constexpr uint16_t MOVE               = 0x0302;
        constexpr uint16_t SYNC_POSITION      = 0x0304;
        constexpr uint16_t WARP               = 0x0306;
        constexpr uint16_t MOTION             = 0x0307;
        constexpr uint16_t DIG_MOTION         = 0x0308;

        // Combat
        constexpr uint16_t DAMAGE_INFO        = 0x0410;
        constexpr uint16_t FLY_TARGETING      = 0x0411;
        constexpr uint16_t ADD_FLY_TARGETING  = 0x0412;
        constexpr uint16_t CREATE_FLY         = 0x0413;
        constexpr uint16_t PVP                = 0x0414;
        constexpr uint16_t DUEL_START         = 0x0415;

        // Items
        constexpr uint16_t ITEM_DEL           = 0x0510;
        constexpr uint16_t ITEM_SET           = 0x0511;
        constexpr uint16_t ITEM_USE           = 0x0512;
        constexpr uint16_t ITEM_DROP          = 0x0513;
        constexpr uint16_t ITEM_UPDATE        = 0x0514;
        constexpr uint16_t ITEM_GROUND_ADD    = 0x0515;
        constexpr uint16_t ITEM_GROUND_DEL    = 0x0516;
        constexpr uint16_t ITEM_OWNERSHIP     = 0x0517;
        constexpr uint16_t ITEM_GET           = 0x0518;
        constexpr uint16_t QUICKSLOT_ADD      = 0x0519;
        constexpr uint16_t QUICKSLOT_DEL      = 0x051A;
        constexpr uint16_t QUICKSLOT_SWAP     = 0x051B;
        constexpr uint16_t EXCHANGE           = 0x051C;
        constexpr uint16_t REFINE_INFORMATION = 0x051D;
        constexpr uint16_t REFINE_INFORMATION_NEW = 0x051E;
        constexpr uint16_t DRAGON_SOUL_REFINE = 0x051F;

        // Chat
        constexpr uint16_t CHAT               = 0x0603;
        constexpr uint16_t WHISPER            = 0x0604;

        // Social
        constexpr uint16_t PARTY_INVITE       = 0x0710;
        constexpr uint16_t PARTY_ADD          = 0x0711;
        constexpr uint16_t PARTY_UPDATE       = 0x0712;
        constexpr uint16_t PARTY_REMOVE       = 0x0713;
        constexpr uint16_t PARTY_LINK         = 0x0714;
        constexpr uint16_t PARTY_UNLINK       = 0x0715;
        constexpr uint16_t PARTY_PARAMETER    = 0x0716;
        constexpr uint16_t GUILD              = 0x0730;
        constexpr uint16_t REQUEST_MAKE_GUILD = 0x0731;
        constexpr uint16_t SYMBOL_DATA        = 0x0732;
        constexpr uint16_t MESSENGER          = 0x0741;
        constexpr uint16_t LOVER_INFO         = 0x0750;
        constexpr uint16_t LOVE_POINT_UPDATE  = 0x0751;

        // Shop / Safebox / Mall
        constexpr uint16_t SHOP               = 0x0810;
        constexpr uint16_t SHOP_SIGN          = 0x0811;
        constexpr uint16_t SAFEBOX_SET        = 0x0830;
        constexpr uint16_t SAFEBOX_DEL        = 0x0831;
        constexpr uint16_t SAFEBOX_WRONG_PASSWORD = 0x0832;
        constexpr uint16_t SAFEBOX_SIZE       = 0x0833;
        constexpr uint16_t SAFEBOX_MONEY_CHANGE = 0x0834;
        constexpr uint16_t MALL_OPEN          = 0x0841;
        constexpr uint16_t MALL_SET           = 0x0842;
        constexpr uint16_t MALL_DEL           = 0x0843;

        // Quest
        constexpr uint16_t SCRIPT             = 0x0910;
        constexpr uint16_t QUEST_CONFIRM      = 0x0911;
        constexpr uint16_t QUEST_INFO         = 0x0912;

        // UI / Effects / Targeting
        constexpr uint16_t TARGET             = 0x0A10;
        constexpr uint16_t TARGET_UPDATE      = 0x0A11;
        constexpr uint16_t TARGET_DELETE      = 0x0A12;
        constexpr uint16_t TARGET_CREATE_NEW  = 0x0A13;
        constexpr uint16_t AFFECT_ADD         = 0x0A20;
        constexpr uint16_t AFFECT_REMOVE      = 0x0A21;
        constexpr uint16_t SEPCIAL_EFFECT     = 0x0A30;
        constexpr uint16_t SPECIFIC_EFFECT    = 0x0A31;
        constexpr uint16_t MOUNT              = 0x0A40;
        constexpr uint16_t OWNERSHIP          = 0x0A41;
        constexpr uint16_t NPC_POSITION       = 0x0A50;

        // World
        constexpr uint16_t FISHING            = 0x0B10;
        constexpr uint16_t DUNGEON            = 0x0B11;
        constexpr uint16_t LAND_LIST          = 0x0B12;
        constexpr uint16_t TIME               = 0x0B13;
        constexpr uint16_t CHANNEL            = 0x0B14;
        constexpr uint16_t MARK_UPDATE        = 0x0B15;
        constexpr uint16_t OBSERVER_ADD       = 0x0B20;
        constexpr uint16_t OBSERVER_REMOVE    = 0x0B21;
        constexpr uint16_t OBSERVER_MOVE      = 0x0B22;

        // Guild Marks
        constexpr uint16_t MARK_BLOCK         = 0x0C10;
        constexpr uint16_t MARK_IDXLIST       = 0x0C11;
        constexpr uint16_t MARK_DIFF_DATA     = 0x0C12;
    }

    enum class Phase : uint8_t
    {
        Close = 0,
        Handshake = 1,
        Login = 2,
        Select = 3,
        Loading = 4,
        Game = 5,
        Dead = 6,
        ClientConnecting = 7,
        DBClient = 8,
        P2P = 9,
        Auth = 10
    };

} // namespace Client::Network::Protocol

// Aliasy globalne
namespace CG {
    using namespace Client::Network::Protocol::CG;
}
namespace GC {
    using namespace Client::Network::Protocol::GC;
}
