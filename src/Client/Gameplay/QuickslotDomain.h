#pragma once

#include <cstdint>
#include <array>
#include <span>
#include <optional>
#include <expected>
#include "../../EterBase/Result.h"

namespace Client::Gameplay {

/**
 * @brief Pojedynczy slot paska szybkiego dostepu (Quickslot).
 */
struct QuickslotItem {
    uint8_t type{0};
    uint8_t position{0};

    constexpr bool operator==(const QuickslotItem& other) const = default;
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return type == 0; }
};

/**
 * @brief Domena paska szybkiego dostepu odcieta od UI i Pythona.
 * 
 * Zarzadza 36 slotami paska szybkiego dostepu, stronicowaniem (4 strony po 8 slotow)
 * oraz walidacja zakresow i operacjami zamiany miejsc (swap/move).
 */
class QuickslotDomain {
public:
    static constexpr uint32_t QUICKSLOT_MAX_LINE = 4;
    static constexpr uint32_t QUICKSLOT_MAX_COUNT_PER_LINE = 8;
    static constexpr uint32_t QUICKSLOT_MAX_COUNT = QUICKSLOT_MAX_LINE * QUICKSLOT_MAX_COUNT_PER_LINE; // 32
    static constexpr uint32_t QUICKSLOT_MAX_NUM = 36; // Calkowity limit serwera

    QuickslotDomain();
    ~QuickslotDomain() = default;

    /**
     * @brief Czyste zresetowanie wszystkich slotow i strony.
     */
    void Clear() noexcept;

    /**
     * @brief Pobiera aktualny indeks strony paska szybkiego dostepu.
     */
    [[nodiscard]] int32_t GetPage() const noexcept { return m_pageIndex; }

    /**
     * @brief Ustawia indeks strony z bezpiecznym zawijaniem (modulo / offset).
     */
    void SetPage(int32_t pageIndex) noexcept;

    /**
     * @brief Konwertuje lokalny indeks slotu (0..7) na globalny indeks w tablicy (0..35).
     */
    [[nodiscard]] uint32_t LocalToGlobalIndex(uint32_t localSlotIndex) const noexcept;

    /**
     * @brief Pobiera dane slotu na podstawie globalnego indeksu.
     */
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetSlot(uint32_t globalSlotIndex) const;

    /**
     * @brief Ustawia zawartosc slotu na podstawie globalnego indeksu.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetSlot(uint32_t globalSlotIndex, const QuickslotItem& item);

    /**
     * @brief Czysci zawartosc slotu (ustawia type=0, position=0).
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> ClearSlot(uint32_t globalSlotIndex);

    /**
     * @brief Zamienia miejscami dwa sloty globalne.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapSlots(uint32_t slotA, uint32_t slotB);

    /**
     * @brief Pobiera dane lokalnego slotu biezacej strony.
     */
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetLocalSlot(uint32_t localSlotIndex) const;

    /**
     * @brief Ustawia dane lokalnego slotu biezacej strony.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetLocalSlot(uint32_t localSlotIndex, const QuickslotItem& item);

    /**
     * @brief Zwraca widok wszystkich slotow.
     */
    [[nodiscard]] std::span<const QuickslotItem> GetAllSlots() const noexcept { return m_slots; }
    [[nodiscard]] std::span<QuickslotItem> GetAllSlots() noexcept { return m_slots; }

private:
    std::array<QuickslotItem, QUICKSLOT_MAX_NUM> m_slots{};
    int32_t m_pageIndex{0};
};

} // namespace Client::Gameplay
