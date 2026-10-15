#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <cstddef>
#include "EterBase/StrongTypes.h"

namespace Client::Network::Protocol
{
    constexpr uint16_t PACKET_HEADER_SIZE = 4;
    typedef uint16_t TPacketHeader;

    constexpr size_t ITEM_SOCKET_SLOT_MAX_NUM = 3;
    constexpr size_t ITEM_ATTRIBUTE_SLOT_MAX_NUM = 7;
    constexpr size_t DS_REFINE_WINDOW_MAX_NUM = 15;
    constexpr size_t CHARACTER_NAME_MAX_LEN = 64;
    constexpr size_t POINT_MAX_NUM = 255;
    constexpr uint16_t INVALID_CELL = 0xFFFF;

#ifndef WORD_MAX
#define WORD_MAX 0xffff
#endif

    enum EWindows : uint8_t
    {
        RESERVED_WINDOW = 0,
        INVENTORY = 1,
        EQUIPMENT = 2,
        SAFEBOX = 3,
        MALL = 4,
        DRAGON_SOUL_INVENTORY = 5,
        BELT_INVENTORY = 6,
        GROUND = 7,
    };

#pragma pack(push, 1)

    /**
     * @brief Podstawowa pozycja przedmiotu w oknie ekwipunku/magazynu.
     */
    struct SItemPos
    {
        uint8_t window_type{0};
        uint16_t cell{0};

        constexpr SItemPos() = default;
        constexpr SItemPos(uint8_t w, uint16_t c) : window_type(w), cell(c) {}

        constexpr bool operator==(const SItemPos& rhs) const noexcept
        {
            return (window_type == rhs.window_type) && (cell == rhs.cell);
        }

        constexpr bool operator<(const SItemPos& rhs) const noexcept
        {
            return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
        }

        constexpr bool IsValidItemPosition() const noexcept
        {
            return window_type != 0 || cell != 0xFFFF;
        }

        bool IsValidCell() const noexcept
        {
            return cell != 0xFFFF;
        }
    };
    typedef SItemPos TItemPos;

    /**
     * @brief Atrybut pojedynczego bonusa przedmiotu.
     */
    struct TPlayerItemAttribute
    {
        uint8_t  bType{0};
        int16_t  sValue{0};

        constexpr TPlayerItemAttribute() = default;
        constexpr TPlayerItemAttribute(uint8_t type, int16_t val) : bType(type), sValue(val) {}
    };

    /**
     * @brief Slot szybkiego dostepu (pasek zadan).
     */
    struct SQuickSlot
    {
        uint8_t Type{0};
        uint8_t Position{0};

        constexpr SQuickSlot() = default;
        constexpr SQuickSlot(uint8_t t, uint8_t p) : Type(t), Position(p) {}
    };
    typedef SQuickSlot TQuickSlot;

    /**
     * @brief Wspolrzedne wektora pozycji w swiecie gry.
     */
    struct SPixelPosition
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        constexpr SPixelPosition() = default;
        constexpr SPixelPosition(float x, float y, float z = 0.0f) : x(x), y(y), z(z) {}
    };
    typedef SPixelPosition TPixelPosition;

#pragma pack(pop)

} // namespace Client::Network::Protocol

// Globalne aliasy i struktury kompatybilnosci wstecznej
#ifndef __METIN2_PROTOCOL_TYPES_DEFINED__
#define __METIN2_PROTOCOL_TYPES_DEFINED__

constexpr uint16_t PACKET_HEADER_SIZE = Client::Network::Protocol::PACKET_HEADER_SIZE;
using TPacketHeader = Client::Network::Protocol::TPacketHeader;

#if !defined(__METIN2_GAME_TYPE_H__) && !defined(_INC_METIN_II_GAME_TYPE_H__) && !defined(__INC_METIN_II_GAME_TYPE_H__)

using TItemPos = Client::Network::Protocol::TItemPos;
using SItemPos = Client::Network::Protocol::SItemPos;
using TPlayerItemAttribute = Client::Network::Protocol::TPlayerItemAttribute;
using SQuickSlot = Client::Network::Protocol::SQuickSlot;
using TQuickSlot = Client::Network::Protocol::TQuickSlot;
using SPixelPosition = Client::Network::Protocol::SPixelPosition;
using TPixelPosition = Client::Network::Protocol::TPixelPosition;

enum
{
    ITEM_SOCKET_SLOT_MAX_NUM = 3,
    ITEM_ATTRIBUTE_SLOT_MAX_NUM = 7,
    DS_REFINE_WINDOW_MAX_NUM = 15,
};

#pragma pack(push, 1)

typedef struct packet_item
{
    uint32_t       vnum{0};
    uint8_t        count{0};
    uint32_t       flags{0};
    uint32_t       anti_flags{0};
    int32_t        alSockets[ITEM_SOCKET_SLOT_MAX_NUM]{0};
    TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM]{};
} TItemData;

typedef struct packet_shop_item
{
    uint32_t       vnum{0};
    uint32_t       price{0};
    uint8_t        count{0};
    uint8_t        display_pos{0};
    int32_t        alSockets[ITEM_SOCKET_SLOT_MAX_NUM]{0};
    TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_SLOT_MAX_NUM]{};
} TShopItemData;

#pragma pack(pop)

using Client::Network::Protocol::INVENTORY;
using Client::Network::Protocol::RESERVED_WINDOW;
using Client::Network::Protocol::EQUIPMENT;
using Client::Network::Protocol::SAFEBOX;
using Client::Network::Protocol::MALL;
using Client::Network::Protocol::DRAGON_SOUL_INVENTORY;
using Client::Network::Protocol::BELT_INVENTORY;
using Client::Network::Protocol::GROUND;

#endif // !__METIN2_GAME_TYPE_H__

#ifndef POINT_MAX_NUM_DEFINED
#define POINT_MAX_NUM_DEFINED
enum
{
    POINT_MAX_NUM = 255,
    CHARACTER_NAME_MAX_LEN = 64,
};
#endif

#if !defined(_D3DX9_H_) && !defined(__D3DX9_H__) && !defined(_D3DX9MATH_H_) && !defined(__INC_METIN_II_GAMELIB_MAP_TYPE_H__) && !defined(__METIN2_GAME_TYPE_H__) && !defined(__MapType_Header__)
using TPixelPosition = Client::Network::Protocol::TPixelPosition;
#endif

#endif // __METIN2_PROTOCOL_TYPES_DEFINED__

namespace Client::Network::Protocol
{
    using packet_item = ::packet_item;
    using TItemData = ::TItemData;
    using packet_shop_item = ::packet_shop_item;
    using TShopItemData = ::TShopItemData;
}
