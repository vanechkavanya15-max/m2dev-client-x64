#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include "../Core/StrongTypes.h"
#include "../../EterBase/Result.h"

namespace Client::Gameplay {

/**
 * @brief Struktura reprezentujaca przedmiot lezacy na ziemi.
 */
struct GroundItem {
    Client::Core::EntityVid vid;
    Client::Core::ItemVnum vnum;
    Client::Core::MapCoords coords;
    std::string ownershipLabel;
};

/**
 * @brief Wynik procesu wybierania przedmiotu.
 */
struct PickResult {
    Client::Core::EntityVid targetVid;
    float distance;
};

/**
 * @brief Blad wyboru przedmiotu
 */
enum class PickerError : uint8_t {
    NoItemsNearby,
    NoOwnership,
    AllItemsOutOfRange
};

[[nodiscard]] constexpr std::string_view ToString(PickerError err) noexcept {
    switch (err) {
        case PickerError::NoItemsNearby: return "NoItemsNearby";
        case PickerError::NoOwnership: return "NoOwnership";
        case PickerError::AllItemsOutOfRange: return "AllItemsOutOfRange";
    }
    return "UnknownPickerError";
}

/**
 * @brief Inteligentne Zbieranie Dropu - Wybiera najblizszy, dozwolony przedmiot na ziemi.
 */
class GroundItemPicker {
public:
    static constexpr float MAX_PICKUP_RANGE = 300.0f;

    /**
     * @brief Wybiera najblizszy dozwolony przedmiot z listy przedmiotow.
     * @param items Lista przedmiotow w poblizu gracza.
     * @param playerCoords Pozycja gracza.
     * @param playerName Nazwa gracza, wymagana do sprawdzenia wlasnosci dropu. (Puste oznacza brak restrykcji dla uzytkownika).
     * @return Zwraca PickResult jesli znaleziono odpowiedni przedmiot, lub PickerError.
     */
    [[nodiscard]] static EterBase::Result<PickResult, PickerError> PickNearestItem(
        std::span<const GroundItem> items, 
        const Client::Core::MapCoords& playerCoords,
        const std::string& playerName);
};

} // namespace Client::Gameplay
