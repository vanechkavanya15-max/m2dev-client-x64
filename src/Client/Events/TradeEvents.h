#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace Client::Events {

// Zdarzenie wyzwalane w momencie otwarcia okna handlu z innym graczem
struct TradeWindowOpenedEvent {
    // Unikalny identyfikator (VID) celu (gracza), z ktorym otwieramy handel
    uint32_t targetVid;
    
    constexpr TradeWindowOpenedEvent(uint32_t vid) noexcept : targetVid(vid) {}
};

// Zdarzenie wyzwalane podczas dodawania zlota do okna handlu
struct GoldAddedToTradeEvent {
    // Ilosc zlota dodana do handlu
    uint64_t amount;
    
    constexpr GoldAddedToTradeEvent(uint64_t amount) noexcept : amount(amount) {}
};

// Zdarzenie wyzwalane przy zakupie przedmiotu w sklepie (NPC lub prywatnym)
struct ShopItemPurchasedEvent {
    // Unikalny identyfikator (VID) sklepu
    uint32_t shopVid;
    // Indeks slotu kupowanego przedmiotu w sklepie
    uint8_t slotIndex;
    // Cena, ktora zostala zaplacona za przedmiot
    uint64_t price;
    
    constexpr ShopItemPurchasedEvent(uint32_t shopVid, uint8_t slotIndex, uint64_t price) noexcept 
        : shopVid(shopVid), slotIndex(slotIndex), price(price) {}
};

} // namespace Client::Events
