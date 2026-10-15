/**
 * @file Packet_Shop.h
 * @brief Network packets for the NPC and Private Shop systems.
 * 
 * Defines the structures used for client-server communication regarding shops,
 * including item listing, buying, selling, and private shop creation.
 * Strict alignment is maintained using `#pragma pack(push, 1)`.
 */
#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "../GameType.h"
#include <cstring>

#pragma pack(push, 1)

/**
 * @brief Data structure representing a single item in a shop.
 * Used in NPC shops and private shops.
 */
struct TShopItemData
{
    uint32_t vnum;                                    /**< The virtual number of the item */
    uint32_t price;                                   /**< The price of the item */
    uint8_t count;                                    /**< The quantity of the item */
    uint8_t displayPos;                               /**< The position of the item in the shop grid */
    int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];      /**< Socket values (e.g. spirit stones) */
    TPlayerItemAttribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM]; /**< Attributes/Bonuses of the item */
};

/**
 * @brief Packet sent from Client to Game to initiate a shop action.
 */
struct TPacketCGShop
{
    uint16_t header;       /**< Packet header (CG::SHOP) */
    uint16_t length;       /**< Total packet length */
    uint8_t subheader;     /**< Shop subheader action (ShopSub::CG) */
};

/**
 * @brief Data structure for an item placed in a private shop.
 */
struct TShopItemTable
{
    uint32_t vnum;         /**< The virtual number of the item */
    uint8_t count;         /**< The quantity of the item */
    TItemPos pos;          /**< The position in the player's inventory */
    uint32_t price;        /**< The selling price set by the player */
    uint8_t displayPos;    /**< The position of the item in the private shop grid */
};

/**
 * @brief Packet sent from Client to Game to create a private shop.
 */
struct TPacketCGMyShop
{
    uint16_t header;                          /**< Packet header (CG::MYSHOP) */
    uint16_t length;                          /**< Total packet length */
    char sign[SHOP_SIGN_MAX_LEN + 1];         /**< The sign/title of the private shop */
    uint8_t count;                            /**< The number of items in the shop (max 39) */

    /**
     * @brief Gets the shop sign as a modern string_view.
     * @return std::string_view representing the shop sign text.
     */
    [[nodiscard]] std::string_view GetSign() const noexcept
    {
        return std::string_view(sign, strnlen(sign, sizeof(sign)));
    }
};

/**
 * @brief Packet sent from Game to Client to start viewing a shop (standard).
 */
struct TPacketGCShopStart
{
    TShopItemData items[SHOP_HOST_ITEM_MAX_NUM]; /**< Array of items available in the shop */
};

/**
 * @brief Packet sent from Game to Client to start viewing an extended shop (multiple tabs).
 * Note: Followed by an array of `TSubPacketShopTab` objects.
 */
struct TPacketGCShopStartEx
{
    /**
     * @brief Data structure for a single shop tab in an extended shop.
     */
    struct TSubPacketShopTab 
    {
        char name[SHOP_TAB_NAME_MAX];                 /**< The name of the tab */
        uint8_t coinType;                             /**< The currency type used in this tab */
        TShopItemData items[SHOP_HOST_ITEM_MAX_NUM];  /**< Array of items in this tab */
        
        /**
         * @brief Gets the tab name as a string_view.
         * @return std::string_view representing the tab name.
         */
        [[nodiscard]] std::string_view GetName() const noexcept
        {
            return std::string_view(name, strnlen(name, sizeof(name)));
        }
    };

    uint32_t ownerVid;       /**< The Virtual ID of the shop owner */
    uint8_t shopTabCount;    /**< The number of tabs in the shop */
    
    // TSubPacketShopTab shopTabs[]; // dynamically appended
};

/**
 * @brief Packet sent from Game to Client to update a specific item in the shop.
 */
struct TPacketGCShopUpdateItem
{
    uint8_t pos;             /**< The grid position of the item to update */
    TShopItemData item;      /**< The new item data */
};

/**
 * @brief Packet sent from Game to Client to update the price/amount in the shop.
 */
struct TPacketGCShopUpdatePrice
{
    int32_t elkAmount;       /**< The updated amount of Elk/Money */
};

/**
 * @brief Packet sent from Game to Client regarding shop state changes.
 */
struct TPacketGCShop
{
    uint16_t header;         /**< Packet header (GC::SHOP) */
    uint16_t length;         /**< Total packet length */
    uint8_t subheader;       /**< Shop subheader action (ShopSub::GC) */
};

/**
 * @brief Packet sent from Game to Client to display a private shop sign over a character.
 */
struct TPacketGCShopSign
{
    uint16_t header;                        /**< Packet header (GC::SHOP_SIGN) */
    uint16_t length;                        /**< Total packet length */
    uint32_t vid;                           /**< The Virtual ID of the shop owner */
    char sign[SHOP_SIGN_MAX_LEN + 1];       /**< The sign/title of the private shop */
    
    /**
     * @brief Gets the shop sign as a modern string_view.
     * @return std::string_view representing the shop sign text.
     */
    [[nodiscard]] std::string_view GetSign() const noexcept
    {
        return std::string_view(sign, strnlen(sign, sizeof(sign)));
    }
};

#pragma pack(pop)
