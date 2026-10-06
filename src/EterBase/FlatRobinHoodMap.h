#pragma once

#include <array>
#include <optional>
#include <functional>
#include <cstdint>
#include <expected>
#include <utility>

#include "Result.h"
#include "StrongTypes.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @class FlatRobinHoodMap
 * @brief Plaska tablica hashujaca uzywajaca algorytmu Robin Hood dla bezpiecznego, bezalokacyjnego przechowywania par klucz-wartosc.
 * 
 * Klasa prealokuje cala pamiec w buforze 'std::array' o wielkosci 'Capacity'. 
 * Brak alokacji na stercie w trakcie operacji (Zero-allocation).
 * W przypadku kolizji stosuje Open Addressing z Robin Hood Hashing.
 * 
 * @tparam Key Typ klucza (np. EntityId)
 * @tparam Value Typ wartosci 
 * @tparam Capacity Maksymalna pojemnosc mapy
 */
template <typename Key, typename Value, std::size_t Capacity>
class FlatRobinHoodMap {
public:
    /**
     * @struct Entry
     * @brief Struktura przechowujaca pojedynczy wpis w mapie.
     */
    struct Entry {
        Key key{};
        Value value{};
        std::size_t distance{0};
        bool isOccupied{false};
    };

    /**
     * @brief Domyslny konstruktor.
     */
    constexpr FlatRobinHoodMap() = default;

    /**
     * @brief Wstawia nowy element do mapy.
     * @param key Klucz do wstawienia.
     * @param value Wartosc powiazana z kluczem.
     * @return Zwraca std::expected<void, EntityError>. EntityError::OutOfRange w przypadku braku miejsca, EntityError::AlreadyExists dla zduplikowanego klucza.
     */
    [[nodiscard]] std::expected<void, EntityError> Insert(Key key, Value value) {
        if (size >= Capacity) {
            ModernLogger::Error("FlatRobinHoodMap: Capacity exceeded during insert attempt.");
            return std::unexpected(EntityError::OutOfRange);
        }

        std::size_t index = HashKey(key) % Capacity;
        std::size_t distance = 0;

        Entry newEntry{std::move(key), std::move(value), distance, true};

        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t currentIndex = (index + i) % Capacity;
            auto& currentEntry = entries[currentIndex];

            if (!currentEntry.isOccupied) {
                currentEntry = std::move(newEntry);
                ++size;
                return {};
            }

            if (currentEntry.key == newEntry.key) {
                ModernLogger::Warn("FlatRobinHoodMap: Attempted to insert a duplicate key.");
                return std::unexpected(EntityError::AlreadyExists);
            }

            if (currentEntry.distance < newEntry.distance) {
                std::swap(currentEntry, newEntry);
            }

            newEntry.distance++;
        }

        return std::unexpected(EntityError::OutOfRange);
    }

    /**
     * @brief Wyszukuje element po kluczu.
     * @param key Szukany klucz.
     * @return Zwraca referencje do wartosci zapakowana w std::expected, lub EntityError::NotFound jesli klucz nie istnieje.
     */
    [[nodiscard]] std::expected<std::reference_wrapper<Value>, EntityError> Find(const Key& key) {
        std::size_t index = HashKey(key) % Capacity;

        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t currentIndex = (index + i) % Capacity;
            auto& currentEntry = entries[currentIndex];

            if (!currentEntry.isOccupied || i > currentEntry.distance) {
                return std::unexpected(EntityError::NotFound);
            }

            if (currentEntry.key == key) {
                return std::ref(currentEntry.value);
            }
        }

        return std::unexpected(EntityError::NotFound);
    }

    /**
     * @brief Wyszukuje element po kluczu (wersja stala).
     * @param key Szukany klucz.
     * @return Zwraca stala referencje do wartosci zapakowana w std::expected, lub EntityError::NotFound jesli klucz nie istnieje.
     */
    [[nodiscard]] std::expected<std::reference_wrapper<const Value>, EntityError> Find(const Key& key) const {
        std::size_t index = HashKey(key) % Capacity;

        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t currentIndex = (index + i) % Capacity;
            const auto& currentEntry = entries[currentIndex];

            if (!currentEntry.isOccupied || i > currentEntry.distance) {
                return std::unexpected(EntityError::NotFound);
            }

            if (currentEntry.key == key) {
                return std::cref(currentEntry.value);
            }
        }

        return std::unexpected(EntityError::NotFound);
    }

    /**
     * @brief Usuwa element z mapy.
     * @param key Klucz elementu do usuniecia.
     * @return Zwraca std::expected<void, EntityError>. EntityError::NotFound jesli klucz nie istnieje.
     */
    [[nodiscard]] std::expected<void, EntityError> Remove(const Key& key) {
        std::size_t index = HashKey(key) % Capacity;

        for (std::size_t i = 0; i < Capacity; ++i) {
            std::size_t currentIndex = (index + i) % Capacity;
            auto& currentEntry = entries[currentIndex];

            if (!currentEntry.isOccupied || i > currentEntry.distance) {
                return std::unexpected(EntityError::NotFound);
            }

            if (currentEntry.key == key) {
                currentEntry.isOccupied = false;
                --size;
                ShiftBackward(currentIndex);
                return {};
            }
        }

        return std::unexpected(EntityError::NotFound);
    }

    /**
     * @brief Wykonuje operacje na elemencie pod warunkiem jego istnienia (Monadyczna aktualizacja).
     * @tparam Fn Typ funkcji/lambdy operujacej na wartosci.
     * @param key Klucz elementu.
     * @param func Funkcja wykonujaca operacje na elemencie jesli istnieje.
     * @return Zwraca std::expected<void, EntityError> potwierdzajacy sukces badz brak elementu.
     */
    template <typename Fn>
    [[nodiscard]] std::expected<void, EntityError> Update(const Key& key, Fn&& func) {
        return Find(key).transform([&func](std::reference_wrapper<Value> refWrap) {
            std::invoke(std::forward<Fn>(func), refWrap.get());
        });
    }

    /**
     * @brief Zwraca aktualna liczbe elementow w mapie.
     * @return Liczba elementow.
     */
    [[nodiscard]] constexpr std::size_t Size() const noexcept {
        return size;
    }

    /**
     * @brief Zwraca pojemnosc mapy.
     * @return Calkowita pojemnosc.
     */
    [[nodiscard]] constexpr std::size_t MaxSize() const noexcept {
        return Capacity;
    }

    /**
     * @brief Czysci cala mape resetujac flagi zajetosci.
     */
    void Clear() noexcept {
        for (auto& entry : entries) {
            entry.isOccupied = false;
        }
        size = 0;
    }

private:
    std::array<Entry, Capacity> entries{};
    std::size_t size{0};

    [[nodiscard]] std::size_t HashKey(const Key& key) const noexcept {
        return std::hash<Key>{}(key);
    }

    void ShiftBackward(std::size_t startIndex) noexcept {
        std::size_t currentIndex = startIndex;
        
        while (true) {
            std::size_t nextIndex = (currentIndex + 1) % Capacity;
            auto& nextEntry = entries[nextIndex];

            if (!nextEntry.isOccupied || nextEntry.distance == 0) {
                break;
            }

            auto& currentEntry = entries[currentIndex];
            currentEntry = std::move(nextEntry);
            currentEntry.distance--;

            nextEntry.isOccupied = false;
            currentIndex = nextIndex;
        }
    }
};

} // namespace EterBase
