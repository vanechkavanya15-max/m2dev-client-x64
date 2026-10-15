#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>
#include <functional>
#include "InventoryDomain.h"

namespace Client::Gameplay {

struct EquippedItemInfo {
    uint32_t vnum{0};
    std::array<ItemAttribute, 7> attributes{};
    std::array<uint32_t, 4> sockets{};
};

/**
 * @brief Kalkulator sumujacy bonusy atrybutow oraz gniazd kamieni dusz (sockets).
 */
class ItemBonusCalculator {
public:
    using SocketBonusResolver = std::function<std::vector<ItemAttribute>(uint32_t socketVnum)>;

    ItemBonusCalculator() = default;
    ~ItemBonusCalculator() = default;

    /**
     * @brief Rejestruje niestandardowy resolver kamieni dusz / gniazd.
     */
    void SetSocketBonusResolver(SocketBonusResolver resolver);

    /**
     * @brief Rejestruje bezposredni bonus dla podanego VNUM kamienia w socket.
     */
    void RegisterSocketBonus(uint32_t socketVnum, uint8_t type, int16_t value);

    /**
     * @brief Oblicza sumaryczna wartosc danego bonusu (w tym z gniazd kamieni dusz).
     */
    int32_t CalculateTotalBonus(uint8_t bonusType, const std::vector<EquippedItemInfo>& equippedItems) const;

    /**
     * @brief Agreguje wszystkie bonusy ze wszystkich przedmiotow i socketow do mapy.
     */
    std::unordered_map<uint8_t, int32_t> AggregateAllBonuses(const std::vector<EquippedItemInfo>& equippedItems) const;

    /**
     * @brief Pobiera bonusy dla danego kamienia w sockecie (z tabeli wbudowanej lub rejestru).
     */
    std::vector<ItemAttribute> GetSocketBonuses(uint32_t socketVnum) const;

private:
    SocketBonusResolver m_customResolver;
    std::unordered_map<uint32_t, std::vector<ItemAttribute>> m_registeredBonuses;
};

} // namespace Client::Gameplay
