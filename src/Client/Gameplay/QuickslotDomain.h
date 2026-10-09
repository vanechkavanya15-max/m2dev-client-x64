#pragma once

#include <cstdint>
#include <array>
#include <span>
#include <optional>
#include <expected>
#include <shared_mutex>
#include <mutex>
#include "../../EterBase/Result.h"
#include "../Core/StrongTypes.h"

namespace Client::Gameplay {

using SlotIndex = Client::Core::SlotIndex;
using ItemSlot = Client::Core::ItemSlot;

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
    [[nodiscard]] int32_t GetPage() const noexcept;

    /**
     * @brief Ustawia indeks strony z bezpiecznym zawijaniem (modulo / offset).
     */
    void SetPage(int32_t pageIndex) noexcept;

    /**
     * @brief Konwertuje lokalny indeks slotu (0..7) na globalny indeks w tablicy (0..35).
     */
    [[nodiscard]] SlotIndex LocalToGlobalIndex(SlotIndex localSlotIndex) const noexcept;
    [[nodiscard]] uint32_t LocalToGlobalIndex(uint32_t localSlotIndex) const noexcept {
        return LocalToGlobalIndex(SlotIndex(static_cast<uint16_t>(localSlotIndex))).get();
    }

    /**
     * @brief Pobiera dane slotu na podstawie globalnego indeksu.
     */
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetSlot(SlotIndex globalSlotIndex) const;
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetSlot(uint32_t globalSlotIndex) const {
        return GetSlot(SlotIndex(static_cast<uint16_t>(globalSlotIndex)));
    }

    /**
     * @brief Ustawia zawartosc slotu na podstawie globalnego indeksu.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetSlot(SlotIndex globalSlotIndex, const QuickslotItem& item);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetSlot(uint32_t globalSlotIndex, const QuickslotItem& item) {
        return SetSlot(SlotIndex(static_cast<uint16_t>(globalSlotIndex)), item);
    }

    /**
     * @brief Czysci zawartosc slotu (ustawia type=0, position=0).
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> ClearSlot(SlotIndex globalSlotIndex);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> ClearSlot(uint32_t globalSlotIndex) {
        return ClearSlot(SlotIndex(static_cast<uint16_t>(globalSlotIndex)));
    }

    /**
     * @brief Zamienia miejscami dwa sloty globalne.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapSlots(SlotIndex slotA, SlotIndex slotB);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapSlots(uint32_t slotA, uint32_t slotB) {
        return SwapSlots(SlotIndex(static_cast<uint16_t>(slotA)), SlotIndex(static_cast<uint16_t>(slotB)));
    }

    /**
     * @brief Pobiera dane lokalnego slotu biezacej strony.
     */
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetLocalSlot(SlotIndex localSlotIndex) const;
    [[nodiscard]] std::expected<QuickslotItem, EterBase::InventoryError> GetLocalSlot(uint32_t localSlotIndex) const {
        return GetLocalSlot(SlotIndex(static_cast<uint16_t>(localSlotIndex)));
    }

    /**
     * @brief Ustawia dane lokalnego slotu biezacej strony.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetLocalSlot(SlotIndex localSlotIndex, const QuickslotItem& item);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetLocalSlot(uint32_t localSlotIndex, const QuickslotItem& item) {
        return SetLocalSlot(SlotIndex(static_cast<uint16_t>(localSlotIndex)), item);
    }

    /**
     * @brief Zwraca widok wszystkich slotow.
     */
    [[nodiscard]] std::span<const QuickslotItem> GetAllSlots() const noexcept {
        std::shared_lock lock(m_mutex);
        return m_slots;
    }
    [[nodiscard]] std::span<QuickslotItem> GetAllSlots() noexcept {
        std::unique_lock lock(m_mutex);
        return m_slots;
    }

private:
    SlotIndex LocalToGlobalIndexUnlocked(SlotIndex localSlotIndex) const noexcept;
    std::expected<QuickslotItem, EterBase::InventoryError> GetSlotUnlocked(SlotIndex globalSlotIndex) const;
    std::expected<void, EterBase::InventoryError> SetSlotUnlocked(SlotIndex globalSlotIndex, const QuickslotItem& item);

    mutable std::shared_mutex m_mutex;
    std::array<QuickslotItem, QUICKSLOT_MAX_NUM> m_slots{};
    int32_t m_pageIndex{0};
};

} // namespace Client::Gameplay
