#include "MartySamaProtocolDriver.h"
#include "UserInterface/Contracts/IGameEvents.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include "EterBase/EventBus.h"
#include <cstring>
#include <cmath>
#include <optional>
#include <algorithm>

#pragma pack(push, 1)

// ============================================================================
// STRUKTURY CG (Client -> Game) DLA PROTOKOLU MARTYSAMA
// ============================================================================

struct TPacketCGAttackMarty
{
    uint8_t  header;                 ///< HEADER_CG_ATTACK = 2 (0x02)
    uint8_t  bType;                  ///< Typ ataku
    uint32_t dwVictimVID;            ///< VID ofiary/celu
    uint8_t  bCRCMagicCubeProcPiece; ///< Bajt kontrolny CRC procesu
    uint8_t  bCRCMagicCubeFilePiece; ///< Bajt kontrolny CRC pliku
};
static_assert(sizeof(TPacketCGAttackMarty) == 8, "TPacketCGAttackMarty musi miec dokladnie 8 bajtow");

struct TPacketCGMoveMarty
{
    uint8_t  header; ///< HEADER_CG_MOVE = 7 (0x07)
    uint8_t  bFunc;  ///< Funkcja ruchu (1 = MOVE)
    uint8_t  bArg;   ///< Argument dodatkowy
    uint8_t  bRot;   ///< Segmenty rotacji w stopniach [0..71]
    int32_t  lX;     ///< Pozycja X
    int32_t  lY;     ///< Pozycja Y
    uint32_t dwTime; ///< Znacznik czasu klienta (ms)
};
static_assert(sizeof(TPacketCGMoveMarty) == 16, "TPacketCGMoveMarty musi miec dokladnie 16 bajtow");

struct TPacketCGItemUseMarty
{
    uint8_t  header;      ///< HEADER_CG_ITEM_USE = 11 (0x0B)
    uint8_t  window_type; ///< Typ okna ekwipunku (np. 1 = INVENTORY)
    uint16_t cell;        ///< Indeks slotu
};
static_assert(sizeof(TPacketCGItemUseMarty) == 4, "TPacketCGItemUseMarty musi miec dokladnie 4 bajty");

struct TPacketCGTargetMarty
{
    uint8_t  header;    ///< HEADER_CG_TARGET = 61 (0x3D)
    uint32_t targetVid; ///< Identyfikator celu (dwVID)
};
static_assert(sizeof(TPacketCGTargetMarty) == 5, "TPacketCGTargetMarty musi miec dokladnie 5 bajtow");

struct TPacketCGItemMoveMarty
{
    uint8_t  header;    ///< HEADER_CG_ITEM_MOVE = 13 (0x0D)
    uint8_t  srcWindow; ///< Typ okna zrodlowego
    uint16_t srcCell;   ///< Slot zrodlowy
    uint8_t  dstWindow; ///< Typ okna docelowego
    uint16_t dstCell;   ///< Slot docelowy
    uint8_t  count;     ///< Liczba sztuk
};
static_assert(sizeof(TPacketCGItemMoveMarty) == 8, "TPacketCGItemMoveMarty musi miec dokladnie 8 bajtow");

struct TPacketCGItemDrop2Marty
{
    uint8_t  header; ///< HEADER_CG_ITEM_DROP2 = 20 (0x14)
    uint8_t  window; ///< Typ okna
    uint16_t cell;   ///< Slot
    uint32_t gold;   ///< Ilosc zlota
    uint8_t  count;  ///< Liczba sztuk
};
static_assert(sizeof(TPacketCGItemDrop2Marty) == 9, "TPacketCGItemDrop2Marty musi miec dokladnie 9 bajtow");

struct TPacketCGItemPickUpMarty
{
    uint8_t  header; ///< HEADER_CG_ITEM_PICKUP = 15 (0x0F)
    uint32_t vid;    ///< Identyfikator upuszczonego przedmiotu dwVID
};
static_assert(sizeof(TPacketCGItemPickUpMarty) == 5, "TPacketCGItemPickUpMarty musi miec dokladnie 5 bajtow");

struct TPacketCGSelectEmpireMarty
{
    uint8_t header;   ///< HEADER_CG_EMPIRE = 90 (0x5A)
    uint8_t empireId; ///< Identyfikator imperium
};
static_assert(sizeof(TPacketCGSelectEmpireMarty) == 2, "TPacketCGSelectEmpireMarty musi miec dokladnie 2 bajty");

struct TPacketCGSelectCharacterMarty
{
    uint8_t header;     ///< HEADER_CG_PLAYER_SELECT = 6 (0x06)
    uint8_t playerIndex;///< Indeks slotu postaci
};
static_assert(sizeof(TPacketCGSelectCharacterMarty) == 2, "TPacketCGSelectCharacterMarty musi miec dokladnie 2 bajty");

struct TPacketCGCreateCharacterMarty
{
    uint8_t  header;   ///< HEADER_CG_PLAYER_CREATE = 4 (0x04)
    uint8_t  index;    ///< Indeks slotu
    char     name[25]; ///< Nazwa postaci w standardzie MartySama (24+1)
    uint16_t job;      ///< Klasa
    uint8_t  shape;    ///< Wyglad
    uint8_t  con;      ///< Witalnosc (CON)
    uint8_t  int_;     ///< Inteligencja (INT)
    uint8_t  str;      ///< Sila (STR)
    uint8_t  dex;      ///< Zrecznosc (DEX)
};
static_assert(sizeof(TPacketCGCreateCharacterMarty) == 34, "TPacketCGCreateCharacterMarty musi miec dokladnie 34 bajty");

struct TPacketCGDestroyCharacterMarty
{
    uint8_t header;         ///< HEADER_CG_PLAYER_DESTROY = 5 (0x05)
    uint8_t index;          ///< Indeks slotu
    char    privateCode[8]; ///< Kod usuniecia postaci (8B)
};
static_assert(sizeof(TPacketCGDestroyCharacterMarty) == 10, "TPacketCGDestroyCharacterMarty musi miec dokladnie 10 bajtow");

// ============================================================================
// STRUKTURY GC (Game -> Client) DLA PROTOKOLU MARTYSAMA
// ============================================================================

struct TPacketGCAttackMarty
{
    uint8_t  header;      ///< HEADER_GC_ATTACK = 12 (0x0C)
    uint32_t dwVID;       ///< VID atakujacego
    uint32_t dwVictimVID; ///< VID celu
    uint8_t  bType;       ///< Typ ataku
};
static_assert(sizeof(TPacketGCAttackMarty) == 10, "TPacketGCAttackMarty musi miec dokladnie 10 bajtow");

struct TPacketGCDeadMarty
{
    uint8_t  header; ///< HEADER_GC_DEAD = 14 (0x0E)
    uint32_t dwVID;  ///< VID martwego bytu
};
static_assert(sizeof(TPacketGCDeadMarty) == 5, "TPacketGCDeadMarty musi miec dokladnie 5 bajtow");

struct TPacketGCMoveMarty
{
    uint8_t  header;     ///< HEADER_GC_MOVE = 3 (0x03)
    uint8_t  bFunc;      ///< Funkcja ruchu
    uint8_t  bArg;       ///< Argument
    uint8_t  bRot;       ///< Segment rotacji (kat / 5)
    uint32_t dwVID;      ///< VID bytu
    int32_t  lX;         ///< Pozycja X
    int32_t  lY;         ///< Pozycja Y
    uint32_t dwTime;     ///< Znacznik czasu
    uint32_t dwDuration; ///< Czas trwania ruchu
};
static_assert(sizeof(TPacketGCMoveMarty) == 24, "TPacketGCMoveMarty musi miec dokladnie 24 bajty");

struct TPacketGCTargetMarty
{
    uint8_t  header;     ///< HEADER_GC_TARGET = 63 (0x3F)
    uint32_t dwVID;      ///< VID celu
    uint8_t  bHPPercent; ///< Procent zycia HP (0..100)
};
static_assert(sizeof(TPacketGCTargetMarty) == 6, "TPacketGCTargetMarty musi miec dokladnie 6 bajtow");

struct TPacketGCDamageInfoMarty
{
    uint8_t  header; ///< HEADER_GC_DAMAGE_INFO = 135 (0x87)
    uint32_t dwVID;  ///< VID celu
    uint8_t  flag;   ///< Flaga obrazen
    int32_t  damage; ///< Wartosc obrazen
};
static_assert(sizeof(TPacketGCDamageInfoMarty) == 10, "TPacketGCDamageInfoMarty musi miec dokladnie 10 bajtow");

struct TPacketGCItemSetMarty
{
    uint8_t  header; ///< HEADER_GC_ITEM_SET = 21 (0x15)
    uint8_t  window; ///< Typ okna
    uint16_t cell;   ///< Komorka
    uint32_t vnum;   ///< Vnum przedmiotu
    uint8_t  count;  ///< Liczba sztuk
};
static_assert(sizeof(TPacketGCItemSetMarty) == 9, "TPacketGCItemSetMarty musi miec dokladnie 9 bajtow");

#pragma pack(pop)

namespace Network::Protocol::Drivers
{
    namespace
    {
        // Sprawdza czy pakiet nalezy do kategorii dynamicznych pakietow MartySama
        // (gdzie 2-bajtowy naglowek dlugosci nastepuje zaraz po 1-bajtowym opkodzie).
        [[nodiscard]] constexpr bool IsMartySamaDynamicPacket(uint8_t header) noexcept
        {
            switch (header)
            {
            case 0x04: // HEADER_GC_CHAT (4)
            case 0x22: // HEADER_GC_WHISPER (34)
            case 0x26: // HEADER_GC_SHOP (38)
            case 0x27: // HEADER_GC_SHOP_SIGN (39)
            case 0x2D: // HEADER_GC_SCRIPT (45)
            case 0x40: // HEADER_GC_COMMAND (64)
            case 0x51: // HEADER_GC_QUEST_INFO (81)
            case 0x97: // HEADER_GC_PANAMA_PACK (151)
            case 0xD2: // HEADER_GC_RESPOND_CHANNELSTATUS (210)
                return true;
            default:
                return false;
            }
        }

        // Zwraca staly rozmiar pakietu MartySama na podstawie naglowka GC
        [[nodiscard]] constexpr std::optional<uint32_t> GetMartySamaFixedPacketSize(uint8_t header) noexcept
        {
            switch (header)
            {
            case 1:   return 39; // HEADER_GC_CHARACTER_ADD
            case 2:   return 5;  // HEADER_GC_CHARACTER_DEL
            case 3:   return 24; // HEADER_GC_MOVE (TPacketGCMoveMarty)
            case 5:   return 9;  // HEADER_GC_SYNC_POSITION
            case 6:   return 10; // HEADER_GC_LOGIN_SUCCESS
            case 7:   return 91; // HEADER_GC_LOGIN_FAILURE
            case 8:   return 43; // HEADER_GC_CHARACTER_CREATE_SUCCESS
            case 9:   return 2;  // HEADER_GC_CHARACTER_CREATE_FAILURE
            case 10:  return 2;  // HEADER_GC_CHARACTER_DELETE_SUCCESS
            case 11:  return 2;  // HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID
            case 12:  return 10; // HEADER_GC_ATTACK (TPacketGCAttackMarty)
            case 13:  return 5;  // HEADER_GC_STUN
            case 14:  return 5;  // HEADER_GC_DEAD (TPacketGCDeadMarty)
            case 15:  return 7;  // HEADER_GC_MAIN_CHARACTER
            case 16:  return 65; // HEADER_GC_CHARACTER_POINTS
            case 17:  return 14; // HEADER_GC_CHARACTER_POINT_CHANGE
            case 18:  return 6;  // HEADER_GC_CHANGE_SPEED
            case 19:  return 35; // HEADER_GC_CHARACTER_UPDATE
            case 20:  return 4;  // HEADER_GC_ITEM_DEL
            case 21:  return 51; // HEADER_GC_ITEM_SET
            case 22:  return 4;  // HEADER_GC_ITEM_USE
            case 23:  return 9;  // HEADER_GC_ITEM_DROP
            case 24:  return 42; // HEADER_GC_CHARACTER_UPDATE_NEW
            case 25:  return 51; // HEADER_GC_ITEM_UPDATE
            case 26:  return 19; // HEADER_GC_ITEM_GROUND_ADD
            case 27:  return 5;  // HEADER_GC_ITEM_GROUND_DEL
            case 28:  return 4;  // HEADER_GC_QUICKSLOT_ADD
            case 29:  return 3;  // HEADER_GC_QUICKSLOT_DEL
            case 30:  return 4;  // HEADER_GC_QUICKSLOT_SWAP
            case 31:  return 9;  // HEADER_GC_ITEM_OWNERSHIP
            case 36:  return 10; // HEADER_GC_MOTION
            case 37:  return 11; // HEADER_GC_PARTS
            case 40:  return 37; // HEADER_GC_DUEL_START
            case 41:  return 11; // HEADER_GC_PVP
            case 42:  return 277;// HEADER_GC_EXCHANGE
            case 43:  return 9;  // HEADER_GC_CHARACTER_POSITION
            case 44:  return 1;  // HEADER_GC_PING
            case 46:  return 73; // HEADER_GC_QUEST_CONFIRM
            case 61:  return 5;  // HEADER_GC_MOUNT
            case 62:  return 5;  // HEADER_GC_OWNERSHIP
            case 63:  return 6;  // HEADER_GC_TARGET (TPacketGCTargetMarty)
            case 65:  return 9;  // HEADER_GC_WARP
            case 69:  return 32; // HEADER_GC_ADD_FLY_TARGETING
            case 70:  return 4;  // HEADER_GC_CREATE_FLY
            case 71:  return 20; // HEADER_GC_FLY_TARGETING
            case 76:  return 17; // HEADER_GC_SKILL_LEVEL
            case 85:  return 51; // HEADER_GC_SAFEBOX_SET
            case 86:  return 4;  // HEADER_GC_SAFEBOX_DEL
            case 87:  return 2;  // HEADER_GC_SAFEBOX_WRONG_PASSWORD
            case 88:  return 2;  // HEADER_GC_SAFEBOX_SIZE
            case 113: return 7;  // HEADER_GC_MAIN_CHARACTER
            case 121: return 2;  // HEADER_GC_CHANNEL
            case 126: return 12; // HEADER_GC_AFFECT_ADD
            case 127: return 5;  // HEADER_GC_AFFECT_REMOVE
            case 135: return 10; // HEADER_GC_DAMAGE_INFO (TPacketGCDamageInfoMarty)
            case 252: return 5;  // HEADER_GC_TIME_SYNC
            case 253: return 2;  // HEADER_GC_PHASE
            case 254: return 9;  // HEADER_GC_BINDUDP
            case 255: return 13; // HEADER_GC_HANDSHAKE
            default:  return std::nullopt;
            }
        }
    }

    std::optional<FrameHeaderInfo> MartySamaProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagany co najmniej 1 bajt naglowka
        if (buffer.empty())
        {
            return std::nullopt;
        }

        uint8_t header = buffer[0];

        // 1. Sprawdzenie kategorii pakietow dynamicznych (naglowek 2-bajtowej dlugosci po opkodzie)
        if (IsMartySamaDynamicPacket(header))
        {
            if (buffer.size() < 3)
            {
                return std::nullopt;
            }

            uint32_t dynLength = static_cast<uint32_t>(buffer[1] | (static_cast<uint16_t>(buffer[2]) << 8));
            if (dynLength < 3 || dynLength > 65535)
            {
                return std::nullopt;
            }

            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = dynLength,
                .headerSize = 1
            };
        }

        // 2. Sprawdzenie pakietu o stalym rozmiarze w tabeli MartySama
        auto fixedSize = GetMartySamaFixedPacketSize(header);
        if (fixedSize.has_value())
        {
            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = *fixedSize,
                .headerSize = 1
            };
        }

        // Nieznany opkod
        return std::nullopt;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        TPacketCGAttackMarty packet{};
        packet.header = 2; // HEADER_CG_ATTACK
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid;
        packet.bCRCMagicCubeProcPiece = 0;
        packet.bCRCMagicCubeFilePiece = 0;

        std::vector<uint8_t> buffer(sizeof(TPacketCGAttackMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGAttackMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        TPacketCGMoveMarty packet{};
        packet.header = 7; // HEADER_CG_MOVE
        packet.bFunc = cmd.func;
        packet.bArg = static_cast<uint8_t>(cmd.arg);

        float rot = std::fmod(cmd.rotationDegrees, 360.0f);
        if (rot < 0.0f)
        {
            rot += 360.0f;
        }
        packet.bRot = static_cast<uint8_t>(rot / 5.0f);

        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;

        std::vector<uint8_t> buffer(sizeof(TPacketCGMoveMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGMoveMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeItemUse(
        uint16_t cell, uint8_t windowType) const
    {
        TPacketCGItemUseMarty packet{};
        packet.header = 11; // HEADER_CG_ITEM_USE (0x0B)
        packet.window_type = windowType;
        packet.cell = cell;

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemUseMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemUseMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeTarget(
        uint32_t targetVid) const
    {
        TPacketCGTargetMarty packet{};
        packet.header = 61; // HEADER_CG_TARGET (0x3D)
        packet.targetVid = targetVid;

        std::vector<uint8_t> buffer(sizeof(TPacketCGTargetMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGTargetMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeItemMove(
        const Domain::ItemMoveCommand& cmd) const
    {
        TPacketCGItemMoveMarty packet{};
        packet.header = 13; // HEADER_CG_ITEM_MOVE (0x0D)
        packet.srcWindow = cmd.srcWindow;
        packet.srcCell = cmd.srcCell;
        packet.dstWindow = cmd.dstWindow;
        packet.dstCell = cmd.dstCell;
        packet.count = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemMoveMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemMoveMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeItemDrop(
        const Domain::ItemDropCommand& cmd) const
    {
        TPacketCGItemDrop2Marty packet{};
        packet.header = 20; // HEADER_CG_ITEM_DROP2 (0x14)
        packet.window = cmd.window;
        packet.cell = cmd.cell;
        packet.gold = cmd.count;
        packet.count = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemDrop2Marty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemDrop2Marty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeItemPickUp(
        const Domain::ItemPickUpCommand& cmd) const
    {
        TPacketCGItemPickUpMarty packet{};
        packet.header = 15; // HEADER_CG_ITEM_PICKUP (0x0F)
        packet.vid = cmd.vid;

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemPickUpMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemPickUpMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeSelectEmpire(
        const Domain::EmpireSelectCommand& cmd) const
    {
        TPacketCGSelectEmpireMarty packet{};
        packet.header = 90; // HEADER_CG_EMPIRE (0x5A)
        packet.empireId = cmd.empireId;

        std::vector<uint8_t> buffer(sizeof(TPacketCGSelectEmpireMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGSelectEmpireMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeSelectCharacter(
        const Domain::CharacterSelectCommand& cmd) const
    {
        TPacketCGSelectCharacterMarty packet{};
        packet.header = 6; // HEADER_CG_PLAYER_SELECT (0x06)
        packet.playerIndex = cmd.slotIndex;

        std::vector<uint8_t> buffer(sizeof(TPacketCGSelectCharacterMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGSelectCharacterMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeCreateCharacter(
        const Domain::CharacterCreateCommand& cmd) const
    {
        TPacketCGCreateCharacterMarty packet{};
        packet.header = 4; // HEADER_CG_PLAYER_CREATE (0x04)
        packet.index = cmd.slotIndex;
        std::memset(packet.name, 0, sizeof(packet.name));
        if (!cmd.name.empty())
        {
            size_t copyLen = std::min(cmd.name.size(), sizeof(packet.name) - 1);
            std::memcpy(packet.name, cmd.name.data(), copyLen);
        }
        packet.job = cmd.job;
        packet.shape = cmd.shape;
        packet.con = cmd.con;
        packet.int_ = cmd.int_;
        packet.str = cmd.str;
        packet.dex = cmd.dex;

        std::vector<uint8_t> buffer(sizeof(TPacketCGCreateCharacterMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGCreateCharacterMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeDestroyCharacter(
        const Domain::CharacterDestroyCommand& cmd) const
    {
        TPacketCGDestroyCharacterMarty packet{};
        packet.header = 5; // HEADER_CG_PLAYER_DESTROY (0x05)
        packet.index = cmd.slotIndex;
        std::memset(packet.privateCode, 0, sizeof(packet.privateCode));
        if (!cmd.privateCode.empty())
        {
            size_t copyLen = std::min(cmd.privateCode.size(), sizeof(packet.privateCode));
            std::memcpy(packet.privateCode, cmd.privateCode.data(), copyLen);
        }

        std::vector<uint8_t> buffer(sizeof(TPacketCGDestroyCharacterMarty));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGDestroyCharacterMarty));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> MartySamaProtocolDriver::EncodeEnterGame() const
    {
        uint8_t header = 10; // HEADER_CG_ENTERGAME = 0x0A (1B)
        return std::vector<uint8_t>{ header };
    }

    bool MartySamaProtocolDriver::DispatchInbound(
        uint16_t unifiedOpcode,
        std::span<const uint8_t> payload,
        UserInterface::Contracts::IGameEventSink* pSink)
    {
        if (payload.empty())
        {
            return false;
        }

        uint8_t bHeader = static_cast<uint8_t>(unifiedOpcode & 0xFF);

        // 1. Publikacja ogolnego zdarzenia odbioru ramki w magistrali EventBus
        ::EterBase::EventBus::GetInstance().Publish(
            ::EterBase::NetworkPacketReceivedEvent(bHeader, payload));

        // 2. Mapowanie naglowkow na zdarzenia domenowe IGameEventSink oraz publikacja dedykowana w EventBus
        switch (bHeader)
        {
        case 12: // HEADER_GC_ATTACK (0x0C)
            if (payload.size() >= sizeof(TPacketGCAttackMarty))
            {
                const auto& atk = *reinterpret_cast<const TPacketGCAttackMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnAttackExecuted({
                        .dwAttackerVID = atk.dwVID,
                        .dwVictimVID = atk.dwVictimVID,
                        .byMotionType = atk.bType
                    });
                }
            }
            return true;

        case 14: // HEADER_GC_DEAD (0x0E)
            if (payload.size() >= sizeof(TPacketGCDeadMarty))
            {
                const auto& dead = *reinterpret_cast<const TPacketGCDeadMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorDead({
                        .dwVID = dead.dwVID
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::ActorDeadEvent(dead.dwVID));
            }
            return true;

        case 3: // HEADER_GC_MOVE (0x03)
            if (payload.size() >= sizeof(TPacketGCMoveMarty))
            {
                const auto& mv = *reinterpret_cast<const TPacketGCMoveMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorMoved({
                        .dwVID = mv.dwVID,
                        .lX = mv.lX,
                        .lY = mv.lY,
                        .fRot = static_cast<float>(mv.bRot * 5.0f),
                        .dwTime = mv.dwTime
                    });
                }
            }
            return true;

        case 63: // HEADER_GC_TARGET (0x3F)
            if (payload.size() >= sizeof(TPacketGCTargetMarty))
            {
                const auto& target = *reinterpret_cast<const TPacketGCTargetMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnTargetChanged({
                        .dwVID = target.dwVID
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::TargetBoardRefreshEvent(target.dwVID));
            }
            return true;

        case 21: // HEADER_GC_ITEM_SET (0x15)
            if (payload.size() >= sizeof(TPacketGCItemSetMarty))
            {
                const auto& item = *reinterpret_cast<const TPacketGCItemSetMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnItemReceived({
                        .dwCell = item.cell,
                        .dwVnum = item.vnum,
                        .dwCount = item.count
                    });
                }
            }
            return true;

        case 135: // HEADER_GC_DAMAGE_INFO (0x87)
            if (payload.size() >= sizeof(TPacketGCDamageInfoMarty))
            {
                const auto& dmg = *reinterpret_cast<const TPacketGCDamageInfoMarty*>(payload.data());
                if (pSink)
                {
                    pSink->OnDamageInfo({
                        .dwVictimVID = dmg.dwVID,
                        .dwAttackerVID = 0,
                        .dwDamage = static_cast<uint64_t>(dmg.damage),
                        .byDamageFlag = dmg.flag
                    });
                }
            }
            return true;

        default:
            break;
        }

        // 3. Delegacja do dyspozytora pakietow PhaseGamePacketDispatcher z walidacja dlugosci bufora
        return UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
            bHeader, payload.data(), payload.size());
    }
}
