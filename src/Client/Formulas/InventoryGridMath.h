#pragma once

#include <cstdint>
#include <expected>
#include <array>
#include <cstddef>

namespace Client::Formulas {

enum class InventoryGridError {
    InvalidSlotIndex,
    ItemOutOfBounds,
    InvalidItemDimensions,
    GridDimensionsZero
};

// Struktura przechowujaca wynik zajmowanych slotow bez dynamicznej alokacji
template <std::size_t MaxCapacity = 9>
struct CoveredSlots {
    std::array<uint32_t, MaxCapacity> slots{};
    uint8_t count = 0;

    constexpr auto begin() const noexcept { return slots.begin(); }
    constexpr auto end() const noexcept { return slots.begin() + count; }
};

class InventoryGridMath {
public:
    // Sprawdza, czy podany indeks slotu znajduje sie w obrebie siatki
    static constexpr bool IsValidSlot(uint32_t slotIndex, uint32_t gridWidth, uint32_t gridHeight) noexcept {
        if (gridWidth == 0 || gridHeight == 0) {
            return false;
        }
        return slotIndex < (gridWidth * gridHeight);
    }

    struct Coordinates {
        uint32_t x;
        uint32_t y;
    };

    // Konwertuje liniowy indeks slotu na wspolrzedne dwuwymiarowe X, Y
    static constexpr std::expected<Coordinates, InventoryGridError> 
    GetCoordinates(uint32_t slotIndex, uint32_t gridWidth, uint32_t gridHeight) noexcept {
        if (gridWidth == 0 || gridHeight == 0) {
            return std::unexpected(InventoryGridError::GridDimensionsZero);
        }
        if (slotIndex >= (gridWidth * gridHeight)) {
            return std::unexpected(InventoryGridError::InvalidSlotIndex);
        }
        return Coordinates{slotIndex % gridWidth, slotIndex / gridWidth};
    }

    // Wyznacza zestaw slotow zajmowanych przez przedmiot bazujac na jego lewym gornym rogu i wymiarach
    template <std::size_t MaxCapacity = 9>
    static constexpr std::expected<CoveredSlots<MaxCapacity>, InventoryGridError> 
    GetItemCoveredSlots(uint32_t baseSlotIndex, uint8_t itemWidth, uint8_t itemHeight, uint32_t gridWidth, uint32_t gridHeight) noexcept {
        if (gridWidth == 0 || gridHeight == 0) {
            return std::unexpected(InventoryGridError::GridDimensionsZero);
        }
        if (itemWidth == 0 || itemHeight == 0 || (static_cast<std::size_t>(itemWidth) * itemHeight) > MaxCapacity) {
            return std::unexpected(InventoryGridError::InvalidItemDimensions);
        }
        
        const auto coordsResult = GetCoordinates(baseSlotIndex, gridWidth, gridHeight);
        if (!coordsResult.has_value()) {
            return std::unexpected(coordsResult.error());
        }
        
        const auto coords = coordsResult.value();
        
        // Weryfikacja czy przedmiot nie wychodzi poza granice siatki
        if (coords.x + itemWidth > gridWidth || coords.y + itemHeight > gridHeight) {
            return std::unexpected(InventoryGridError::ItemOutOfBounds);
        }
        
        CoveredSlots<MaxCapacity> result{};
        for (uint8_t dy = 0; dy < itemHeight; ++dy) {
            for (uint8_t dx = 0; dx < itemWidth; ++dx) {
                const uint32_t currentX = coords.x + dx;
                const uint32_t currentY = coords.y + dy;
                result.slots[result.count++] = currentY * gridWidth + currentX;
            }
        }
        
        return result;
    }

    // Weryfikuje czy dwa przedmioty o podanych wymiarach i pozycjach nachodza na siebie (koliduja) w siatce
    template <std::size_t MaxCapacity = 9>
    static constexpr std::expected<bool, InventoryGridError> 
    IsOverlap(uint32_t slotA, uint8_t widthA, uint8_t heightA, 
              uint32_t slotB, uint8_t widthB, uint8_t heightB, 
              uint32_t gridWidth, uint32_t gridHeight) noexcept {
        
        const auto coveredA = GetItemCoveredSlots<MaxCapacity>(slotA, widthA, heightA, gridWidth, gridHeight);
        if (!coveredA.has_value()) {
            return std::unexpected(coveredA.error());
        }
        
        const auto coveredB = GetItemCoveredSlots<MaxCapacity>(slotB, widthB, heightB, gridWidth, gridHeight);
        if (!coveredB.has_value()) {
            return std::unexpected(coveredB.error());
        }
        
        for (const auto slotIndexA : coveredA.value()) {
            for (const auto slotIndexB : coveredB.value()) {
                if (slotIndexA == slotIndexB) {
                    return true;
                }
            }
        }
        
        return false;
    }
};

} // namespace Client::Formulas
