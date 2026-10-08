#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <expected>
#include <span>
#include <array>
#include <functional>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "Client/Core/DomainErrors.h"
#include "Client/Core/DomainEvents.h"
#include "Client/Core/StrongTypes.h"

namespace Client::Gameplay {

using SlotIndex = Client::Core::SlotIndex;

/**
 * @brief Silnie typowany enum klasowy reprezentujacy okna ekwipunku, calkowicie odciety od UI.
 */
enum class InventoryWindow : uint8_t {
    Reserved = 0,
    Inventory = 1,
    Equipment = 2,
    SafeBox = 3,
    Mall = 4,
    DragonSoul = 5,
    Ground = 6,
    Belt = 7
};

/**
 * @brief Represents the dimensions of an item in the inventory grid.
 */
struct ItemSize {
    uint8_t width{1};
    uint8_t height{1};

    constexpr bool operator==(const ItemSize& other) const = default;
};

/**
 * @brief Pojedynczy atrybut bonusowy przedmiotu.
 */
struct ItemAttribute {
    uint8_t type{0};
    int16_t value{0};

    constexpr bool operator==(const ItemAttribute& other) const = default;
};

/**
 * @brief Core data structure representing an item in the combined inventory domain.
 * Rozszerzona o 4 gniazda kamieni dusz (sockets) oraz 7 atrybutow bonusowych.
 */
struct ItemData {
    EterBase::ItemVnum vnum{0};
    uint32_t count{1};
    ItemSize size{1, 1}; // For anti-overflow validation
    std::array<uint32_t, 4> sockets{};
    std::array<ItemAttribute, 7> attributes{};
    uint32_t flags{0};
    uint32_t anti_flags{0};

    constexpr bool operator==(const ItemData& other) const = default;
};

/**
 * @brief Event triggered when an inventory slot changes (decoupled from UI IEvent).
 */
struct InventorySlotUpdatedEvent {
    InventoryWindow windowType;
    EterBase::ItemSlot slotIndex;

    explicit InventorySlotUpdatedEvent(InventoryWindow windowType, EterBase::ItemSlot slotIndex)
        : windowType(windowType), slotIndex(slotIndex) {}

    explicit InventorySlotUpdatedEvent(uint8_t windowType, EterBase::ItemSlot slotIndex)
        : windowType(static_cast<InventoryWindow>(windowType)), slotIndex(slotIndex) {}

    explicit InventorySlotUpdatedEvent(InventoryWindow windowType, SlotIndex slotIndex)
        : windowType(windowType), slotIndex(EterBase::ItemSlot(slotIndex.value())) {}

    explicit InventorySlotUpdatedEvent(uint8_t windowType, SlotIndex slotIndex)
        : windowType(static_cast<InventoryWindow>(windowType)), slotIndex(EterBase::ItemSlot(slotIndex.value())) {}

    [[nodiscard]] SlotIndex GetSlotIndex() const noexcept {
        return SlotIndex(slotIndex.value());
    }
};

using SlotUpdateCallback = std::function<void(const InventorySlotUpdatedEvent&)>;

/**
 * @brief Centralized Domain Model for managing the player's complete inventory state.
 * 
 * Handles Main Inventory (pages 1-4), Belt, Equipment, Dragon Soul, and SafeBox.
 * Provides validation for item size (1x1, 1x2, 1x3) and prevents overlapping.
 */
class InventoryDomain {
public:
    // Standard Metin2 inventory dimensions
    static constexpr uint16_t INVENTORY_PAGE_WIDTH = 5;
    static constexpr uint16_t INVENTORY_PAGE_HEIGHT = 9;
    static constexpr uint16_t INVENTORY_PAGE_SIZE = INVENTORY_PAGE_WIDTH * INVENTORY_PAGE_HEIGHT;
    static constexpr uint16_t INVENTORY_MAX_PAGES = 4;
    static constexpr uint16_t INVENTORY_MAX_NUM = INVENTORY_PAGE_SIZE * INVENTORY_MAX_PAGES;
    
    // Other window sizes
    static constexpr uint16_t BELT_INVENTORY_MAX_NUM = 16;
    static constexpr uint16_t EQUIPMENT_MAX_NUM = 32;
    static constexpr uint16_t DRAGON_SOUL_INVENTORY_MAX_NUM = 32 * 6 * 2;
    static constexpr uint16_t SAFEBOX_MAX_NUM = 135;

    InventoryDomain();
    ~InventoryDomain() = default;

    /**
     * @brief Rejestruje delegat powiadamiajacy o aktualizacji slotu w domenie ekwipunku.
     */
    void SetSlotUpdateCallback(SlotUpdateCallback callback) {
        m_slotUpdateCallback = std::move(callback);
    }

    /**
     * @brief Znajduje pierwsza wolna komorke w inwentarzu glownym dla przedmiotu 1x1.
     * @return Indeks slotu lub -1 w przypadku braku miejsca.
     */
    [[nodiscard]] int32_t FindEmptyCell() const;

    /**
     * @brief Znajduje pierwsza wolna komorke w inwentarzu glownym dla zadanego rozmiaru przedmiotu.
     * @return Indeks slotu lub -1 w przypadku braku miejsca.
     */
    [[nodiscard]] int32_t FindEmptyCell(ItemSize size) const;

    /**
     * @brief Znajduje pierwsza wolna komorke w wybranym oknie ekwipunku dla zadanego rozmiaru.
     */
    [[nodiscard]] int32_t FindEmptyCell(InventoryWindow window, ItemSize size = {1, 1}) const;

    /**
     * @brief Czyste zresetowanie wszystkich slotow we wszystkich oknach inwentarza.
     */
    void Clear();

    /**
     * @brief Resetuje sloty w konkretnym oknie ekwipunku.
     */
    void ClearWindow(InventoryWindow window);

    [[nodiscard]] std::optional<SlotIndex> FindEmptySlot() const {
        int32_t cell = FindEmptyCell();
        return cell >= 0 ? std::optional<SlotIndex>(SlotIndex(static_cast<uint16_t>(cell))) : std::nullopt;
    }
    [[nodiscard]] std::optional<SlotIndex> FindEmptySlot(ItemSize size) const {
        int32_t cell = FindEmptyCell(size);
        return cell >= 0 ? std::optional<SlotIndex>(SlotIndex(static_cast<uint16_t>(cell))) : std::nullopt;
    }
    [[nodiscard]] std::optional<SlotIndex> FindEmptySlot(InventoryWindow window, ItemSize size = {1, 1}) const {
        int32_t cell = FindEmptyCell(window, size);
        return cell >= 0 ? std::optional<SlotIndex>(SlotIndex(static_cast<uint16_t>(cell))) : std::nullopt;
    }

    /**
     * @brief Sets an item at the specified slot, validating overlap and bounds.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetItem(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetItem(uint8_t windowType, EterBase::ItemSlot slot, const ItemData& item) {
        return SetItem(static_cast<InventoryWindow>(windowType), slot, item);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetItem(InventoryWindow windowType, SlotIndex slot, const ItemData& item) {
        return SetItem(windowType, EterBase::ItemSlot(slot.get()), item);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SetItem(uint8_t windowType, SlotIndex slot, const ItemData& item) {
        return SetItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()), item);
    }

    /**
     * @brief Removes an item from the specified slot.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> RemoveItem(InventoryWindow windowType, EterBase::ItemSlot slot);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> RemoveItem(uint8_t windowType, EterBase::ItemSlot slot) {
        return RemoveItem(static_cast<InventoryWindow>(windowType), slot);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> RemoveItem(InventoryWindow windowType, SlotIndex slot) {
        return RemoveItem(windowType, EterBase::ItemSlot(slot.get()));
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> RemoveItem(uint8_t windowType, SlotIndex slot) {
        return RemoveItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()));
    }

    /**
     * @brief Swaps the items between two slots, validating target slots and sizes.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapItem(InventoryWindow windowType, EterBase::ItemSlot srcSlot, InventoryWindow dstWindowType, EterBase::ItemSlot dstSlot);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapItem(uint8_t windowType, EterBase::ItemSlot srcSlot, uint8_t dstWindowType, EterBase::ItemSlot dstSlot) {
        return SwapItem(static_cast<InventoryWindow>(windowType), srcSlot, static_cast<InventoryWindow>(dstWindowType), dstSlot);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapItem(InventoryWindow windowType, SlotIndex srcSlot, InventoryWindow dstWindowType, SlotIndex dstSlot) {
        return SwapItem(windowType, EterBase::ItemSlot(srcSlot.get()), dstWindowType, EterBase::ItemSlot(dstSlot.get()));
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SwapItem(uint8_t windowType, SlotIndex srcSlot, uint8_t dstWindowType, SlotIndex dstSlot) {
        return SwapItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(srcSlot.get()), static_cast<InventoryWindow>(dstWindowType), EterBase::ItemSlot(dstSlot.get()));
    }

    /**
     * @brief Splits an item stack, placing the split amount into an empty target slot.
     */
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SplitItem(InventoryWindow windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount);
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SplitItem(uint8_t windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount) {
        return SplitItem(static_cast<InventoryWindow>(windowType), srcSlot, dstSlot, splitCount);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SplitItem(InventoryWindow windowType, SlotIndex srcSlot, SlotIndex dstSlot, uint32_t splitCount) {
        return SplitItem(windowType, EterBase::ItemSlot(srcSlot.get()), EterBase::ItemSlot(dstSlot.get()), splitCount);
    }
    [[nodiscard]] std::expected<void, EterBase::InventoryError> SplitItem(uint8_t windowType, SlotIndex srcSlot, SlotIndex dstSlot, uint32_t splitCount) {
        return SplitItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(srcSlot.get()), EterBase::ItemSlot(dstSlot.get()), splitCount);
    }

    /**
     * @brief Retrieves the item at the specified slot.
     */
    [[nodiscard]] std::expected<ItemData, EterBase::InventoryError> GetItem(InventoryWindow windowType, EterBase::ItemSlot slot) const;
    [[nodiscard]] std::expected<ItemData, EterBase::InventoryError> GetItem(uint8_t windowType, EterBase::ItemSlot slot) const {
        return GetItem(static_cast<InventoryWindow>(windowType), slot);
    }
    [[nodiscard]] std::expected<ItemData, EterBase::InventoryError> GetItem(InventoryWindow windowType, SlotIndex slot) const {
        return GetItem(windowType, EterBase::ItemSlot(slot.get()));
    }
    [[nodiscard]] std::expected<ItemData, EterBase::InventoryError> GetItem(uint8_t windowType, SlotIndex slot) const {
        return GetItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()));
    }

    using InventoryError = Client::Core::InventoryError;
    template <typename T, typename E = Client::Core::InventoryError>
    using Result = Client::Core::Result<T, E>;

    /**
     * @brief Nowoczesne metody domenowe zwracajace Result<void, InventoryError>.
     */
    [[nodiscard]] Result<void, InventoryError> AddItem(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item);
    [[nodiscard]] Result<void, InventoryError> AddItem(uint8_t windowType, EterBase::ItemSlot slot, const ItemData& item) {
        return AddItem(static_cast<InventoryWindow>(windowType), slot, item);
    }
    [[nodiscard]] Result<void, InventoryError> AddItem(InventoryWindow windowType, SlotIndex slot, const ItemData& item) {
        return AddItem(windowType, EterBase::ItemSlot(slot.get()), item);
    }
    [[nodiscard]] Result<void, InventoryError> AddItem(uint8_t windowType, SlotIndex slot, const ItemData& item) {
        return AddItem(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()), item);
    }
    [[nodiscard]] Result<void, InventoryError> AddItem(InventoryWindow windowType, const ItemData& item);
    [[nodiscard]] Result<void, InventoryError> AddItem(const ItemData& item);
    [[nodiscard]] Result<void, InventoryError> AddItem(uint16_t slot, const ItemData& item);

    [[nodiscard]] Result<void, InventoryError> RemoveItem(uint16_t slot);
    [[nodiscard]] Result<void, InventoryError> RemoveItem(InventoryWindow windowType, uint16_t slot);
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(InventoryWindow windowType, EterBase::ItemSlot slot);
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(uint8_t windowType, EterBase::ItemSlot slot) {
        return RemoveItemResult(static_cast<InventoryWindow>(windowType), slot);
    }
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(InventoryWindow windowType, SlotIndex slot) {
        return RemoveItemResult(windowType, EterBase::ItemSlot(slot.get()));
    }
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(uint8_t windowType, SlotIndex slot) {
        return RemoveItemResult(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()));
    }
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(EterBase::ItemSlot slot);
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(SlotIndex slot) {
        return RemoveItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(slot.get()));
    }
    [[nodiscard]] Result<void, InventoryError> RemoveItemResult(uint16_t slot);

    [[nodiscard]] Result<void, InventoryError> SetItemResult(InventoryWindow windowType, EterBase::ItemSlot slot, const ItemData& item);
    [[nodiscard]] Result<void, InventoryError> SetItemResult(uint8_t windowType, EterBase::ItemSlot slot, const ItemData& item) {
        return SetItemResult(static_cast<InventoryWindow>(windowType), slot, item);
    }
    [[nodiscard]] Result<void, InventoryError> SetItemResult(InventoryWindow windowType, SlotIndex slot, const ItemData& item) {
        return SetItemResult(windowType, EterBase::ItemSlot(slot.get()), item);
    }
    [[nodiscard]] Result<void, InventoryError> SetItemResult(uint8_t windowType, SlotIndex slot, const ItemData& item) {
        return SetItemResult(static_cast<InventoryWindow>(windowType), EterBase::ItemSlot(slot.get()), item);
    }

    [[nodiscard]] Result<void, InventoryError> SwapItemResult(InventoryWindow windowType, EterBase::ItemSlot srcSlot, InventoryWindow dstWindowType, EterBase::ItemSlot dstSlot);
    [[nodiscard]] Result<void, InventoryError> SwapItemResult(InventoryWindow windowType, SlotIndex srcSlot, InventoryWindow dstWindowType, SlotIndex dstSlot) {
        return SwapItemResult(windowType, EterBase::ItemSlot(srcSlot.get()), dstWindowType, EterBase::ItemSlot(dstSlot.get()));
    }
    [[nodiscard]] Result<void, InventoryError> SplitItemResult(InventoryWindow windowType, EterBase::ItemSlot srcSlot, EterBase::ItemSlot dstSlot, uint32_t splitCount);
    [[nodiscard]] Result<void, InventoryError> SplitItemResult(InventoryWindow windowType, SlotIndex srcSlot, SlotIndex dstSlot, uint32_t splitCount) {
        return SplitItemResult(windowType, EterBase::ItemSlot(srcSlot.get()), EterBase::ItemSlot(dstSlot.get()), splitCount);
    }
    [[nodiscard]] Result<ItemData, InventoryError> GetItemResult(InventoryWindow windowType, EterBase::ItemSlot slot) const;
    [[nodiscard]] Result<ItemData, InventoryError> GetItemResult(InventoryWindow windowType, SlotIndex slot) const {
        return GetItemResult(windowType, EterBase::ItemSlot(slot.get()));
    }


private:
    std::expected<std::reference_wrapper<std::vector<std::optional<ItemData>>>, EterBase::InventoryError> GetWindowSlots(InventoryWindow windowType);
    std::expected<std::reference_wrapper<const std::vector<std::optional<ItemData>>>, EterBase::InventoryError> GetWindowSlots(InventoryWindow windowType) const;

    bool IsValidCell(InventoryWindow windowType, EterBase::ItemSlot slot, ItemSize size) const;
    bool IsEmpty(InventoryWindow windowType, EterBase::ItemSlot slot, ItemSize size, std::optional<EterBase::ItemSlot> ignoreSlot = std::nullopt) const;

    void NotifySlotUpdated(InventoryWindow windowType, EterBase::ItemSlot slot);

    std::vector<std::optional<ItemData>> m_mainInventory;
    std::vector<std::optional<ItemData>> m_beltInventory;
    std::vector<std::optional<ItemData>> m_equipment;
    std::vector<std::optional<ItemData>> m_dragonSoulInventory;
    std::vector<std::optional<ItemData>> m_safeBox;

    SlotUpdateCallback m_slotUpdateCallback;
};

} // namespace Client::Gameplay
