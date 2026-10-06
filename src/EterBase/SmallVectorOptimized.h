#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <memory>
#include <stdexcept>
#include <expected>
#include <utility>
#include <iterator>
#include <initializer_list>
#include <string_view>

#include "Result.h"
#include "LogModern.h"

/**
 * @file SmallVectorOptimized.h
 * @brief Nowoczesny wektor ze Small Buffer Optimization (SBO) zgodny z C++23.
 * 
 * Implementuje kontener typu vector przechowujacy poczatkowa liczbe N elementow
 * w prealokowanym, lokalnym buforze, a nastepnie przechodzacy na alokacje na stercie (heap),
 * gdy przekroczy wbudowany limit. Zastepuje mniejsze pule pamieci i archaiczne
 * tablice statyczne. Zaprojektowany z naciskiem na deterministyczna obsluge 
 * bledow za pomoca std::expected z Result.h i wyeliminowanie rzutowania oraz wskaznikow 'out'.
 */

namespace EterBase {

/**
 * @class SmallVectorOptimized
 * @brief Kontener vector-like stosujacy Small Buffer Optimization (SBO).
 * 
 * @tparam T Typ elementow przechowywanych w wektorze.
 * @tparam N Ilosc elementow przechowywanych bezposrednio w buforze lokalnym bez alokacji.
 */
template <typename T, std::size_t N>
class SmallVectorOptimized {
public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    /**
     * @brief Konstruktor domyslny. Inicjalizuje pusty wektor z buforem lokalnym.
     */
    constexpr SmallVectorOptimized() noexcept = default;

    /**
     * @brief Destruktor wektora. Niszczy elementy i zwalnia pamiec na stercie, jesli byla zaalokowana.
     */
    ~SmallVectorOptimized() {
        clear();
        if (is_heap_allocated()) {
            std::allocator<T>().deallocate(m_heapData, m_capacity);
        }
    }

    /**
     * @brief Konstruktor kopiujacy.
     * @param other Inny wektor do skopiowania.
     */
    SmallVectorOptimized(const SmallVectorOptimized& other) {
        copy_from(other);
    }

    /**
     * @brief Konstruktor przenoszacy.
     * @param other Inny wektor, ktorego zawartosc zostanie przejeta.
     */
    SmallVectorOptimized(SmallVectorOptimized&& other) noexcept {
        move_from(std::move(other));
    }

    /**
     * @brief Operator przypisania kopiujacego.
     * @param other Inny wektor do przypisania.
     * @return Referencja na ten wektor.
     */
    SmallVectorOptimized& operator=(const SmallVectorOptimized& other) {
        if (this != &other) {
            clear();
            copy_from(other);
        }
        return *this;
    }

    /**
     * @brief Operator przypisania przenoszacego.
     * @param other Inny wektor, ktory bedzie przeniesiony.
     * @return Referencja na ten wektor.
     */
    SmallVectorOptimized& operator=(SmallVectorOptimized&& other) noexcept {
        if (this != &other) {
            clear();
            if (is_heap_allocated()) {
                std::allocator<T>().deallocate(m_heapData, m_capacity);
            }
            m_capacity = N;
            m_heapData = nullptr;
            move_from(std::move(other));
        }
        return *this;
    }

    /**
     * @brief Zwraca iterator na poczatek kontenera.
     * @return Iterator poczatkowy.
     */
    [[nodiscard]] constexpr iterator begin() noexcept { return data(); }

    /**
     * @brief Zwraca staly iterator na poczatek kontenera.
     * @return Staly iterator poczatkowy.
     */
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }

    /**
     * @brief Zwraca iterator na koniec kontenera (za ostatnim elementem).
     * @return Iterator koncowy.
     */
    [[nodiscard]] constexpr iterator end() noexcept { return data() + m_size; }

    /**
     * @brief Zwraca staly iterator na koniec kontenera (za ostatnim elementem).
     * @return Staly iterator koncowy.
     */
    [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + m_size; }

    /**
     * @brief Zwraca ilosc aktualnie przechowywanych elementow.
     * @return Zwraca liczba elementow.
     */
    [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }

    /**
     * @brief Zwraca pojemnosc aktualnego wektora (limit prealokowanej pamieci lokalnie lub na stercie).
     * @return Zwraca liczbe pojemnosci wektora.
     */
    [[nodiscard]] constexpr size_type capacity() const noexcept { return m_capacity; }

    /**
     * @brief Sprawdza czy wektor jest pusty.
     * @return Zwraca true jesli pusty, w przeciwnym razie false.
     */
    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }

    /**
     * @brief Uzyskuje dostep do elementu po indeksie.
     * @param index Numer indeksu.
     * @return Referencja na element.
     */
    [[nodiscard]] constexpr reference operator[](size_type index) noexcept {
        return data()[index];
    }

    /**
     * @brief Uzyskuje dostep do elementu po indeksie.
     * @param index Numer indeksu.
     * @return Const referencja na element.
     */
    [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
        return data()[index];
    }

    /**
     * @brief Bezpieczny dostep do elementu pod zadanym indeksem z uzyciem mechanizmu Monady std::expected.
     * @param index Numer elementu wektora.
     * @return Zwraca referencje na element, jesli poprawny. W przeciwnym razie error std::string_view.
     */
    [[nodiscard]] constexpr Result<std::reference_wrapper<T>> at(size_type index) noexcept {
        if (index >= m_size) {
            EterBase::ModernLogger::Warn("SmallVectorOptimized::at out of bounds. Index: {}, Size: {}", index, m_size);
            return MakeError("Index out of bounds");
        }
        return std::ref(data()[index]);
    }

    /**
     * @brief Bezpieczny dostep do elementu pod zadanym indeksem dla typu stalego.
     * @param index Numer elementu wektora.
     * @return Zwraca stala referencje, lub error jesli index poza zakresem.
     */
    [[nodiscard]] constexpr Result<std::reference_wrapper<const T>> at(size_type index) const noexcept {
        if (index >= m_size) {
            return MakeError("Index out of bounds");
        }
        return std::cref(data()[index]);
    }

    /**
     * @brief Dodaje nowy element na koniec wektora, relokujac do sterty, jesli pojemnosc limitowa zostala przekroczona.
     * @param value Element, ktory ma byc dodany.
     */
    constexpr void push_back(const T& value) {
        if (m_size == m_capacity) {
            push_back_realloc(value);
        } else {
            std::construct_at(data() + m_size, value);
            ++m_size;
        }
    }

    /**
     * @brief Dodaje nowy element na koniec wektora (konstruktor przenoszacy).
     * @param value Element, ktory ma byc dodany.
     */
    constexpr void push_back(T&& value) {
        if (m_size == m_capacity) {
            push_back_realloc(std::move(value));
        } else {
            std::construct_at(data() + m_size, std::move(value));
            ++m_size;
        }
    }

    /**
     * @brief Konstruuje element w-miejscu na koncu kontenera.
     * @param args Parametry dla konstruktora typu T.
     * @return Zwraca referencje na nowo utworzony element.
     */
    template<typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (m_size == m_capacity) {
            return emplace_back_realloc(std::forward<Args>(args)...);
        } else {
            std::construct_at(data() + m_size, std::forward<Args>(args)...);
            return data()[m_size++];
        }
    }

    /**
     * @brief Usuwa element z konca kontenera.
     * @return std::expected<void> bez bledu w razie powodzenia, z tekstem bledu, jesli wektor byl juz pusty.
     */
    constexpr VoidResult<> pop_back() noexcept {
        if (m_size == 0) {
            return MakeError("Cannot pop_back from an empty container");
        }
        --m_size;
        std::destroy_at(data() + m_size);
        return {};
    }

    /**
     * @brief Niszczy wszystkie elementy, bez redukowania limitu pamieci prealokowanej.
     */
    constexpr void clear() noexcept {
        std::destroy(begin(), end());
        m_size = 0;
    }

    /**
     * @brief Wskaznik do aktualnego bufora przechowujacego dane.
     * @return Zwraca staly wskaznik do danych.
     */
    [[nodiscard]] constexpr pointer data() noexcept {
        if consteval { return m_heapData; }
        return is_heap_allocated() ? m_heapData : reinterpret_cast<pointer>(&m_localBuffer);
    }

    /**
     * @brief Wskaznik do aktualnego bufora przechowujacego dane (Const).
     * @return Zwraca const wskaznik do danych.
     */
    [[nodiscard]] constexpr const_pointer data() const noexcept {
        if consteval { return m_heapData; }
        return is_heap_allocated() ? m_heapData : reinterpret_cast<const_pointer>(&m_localBuffer);
    }

private:
    [[nodiscard]] constexpr bool is_heap_allocated() const noexcept {
        return m_capacity > N;
    }


    constexpr void ensure_capacity(size_type new_capacity) {
        if (new_capacity <= m_capacity) {
            return;
        }
        
        size_type expanded = m_capacity * 2;
        if (expanded < new_capacity) {
            expanded = new_capacity;
        }

        pointer new_data = std::allocator<T>().allocate(expanded);
        
        for (size_type i = 0; i < m_size; ++i) {
            std::construct_at(new_data + i, std::move_if_noexcept(data()[i]));
            std::destroy_at(data() + i);
        }

        if (is_heap_allocated()) {
            std::allocator<T>().deallocate(m_heapData, m_capacity);
        }
        
        m_heapData = new_data;
        m_capacity = expanded;
    }

    constexpr void push_back_realloc(const T& value) {
        size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
        pointer new_data = std::allocator<T>().allocate(new_capacity);
        
        std::construct_at(new_data + m_size, value);
        
        for (size_type i = 0; i < m_size; ++i) {
            std::construct_at(new_data + i, std::move_if_noexcept(data()[i]));
            std::destroy_at(data() + i);
        }

        if (is_heap_allocated()) {
            std::allocator<T>().deallocate(m_heapData, m_capacity);
        }
        
        m_heapData = new_data;
        m_capacity = new_capacity;
        ++m_size;
    }

    constexpr void push_back_realloc(T&& value) {
        size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
        pointer new_data = std::allocator<T>().allocate(new_capacity);
        
        std::construct_at(new_data + m_size, std::move(value));
        
        for (size_type i = 0; i < m_size; ++i) {
            std::construct_at(new_data + i, std::move_if_noexcept(data()[i]));
            std::destroy_at(data() + i);
        }

        if (is_heap_allocated()) {
            std::allocator<T>().deallocate(m_heapData, m_capacity);
        }
        
        m_heapData = new_data;
        m_capacity = new_capacity;
        ++m_size;
    }

    template<typename... Args>
    constexpr reference emplace_back_realloc(Args&&... args) {
        size_type new_capacity = m_capacity == 0 ? 1 : m_capacity * 2;
        pointer new_data = std::allocator<T>().allocate(new_capacity);
        
        std::construct_at(new_data + m_size, std::forward<Args>(args)...);
        
        for (size_type i = 0; i < m_size; ++i) {
            std::construct_at(new_data + i, std::move_if_noexcept(data()[i]));
            std::destroy_at(data() + i);
        }

        if (is_heap_allocated()) {
            std::allocator<T>().deallocate(m_heapData, m_capacity);
        }
        
        m_heapData = new_data;
        m_capacity = new_capacity;
        ++m_size;
        return m_heapData[m_size - 1];
    }

    constexpr void copy_from(const SmallVectorOptimized& other) {
        ensure_capacity(other.m_size);
        for (size_type i = 0; i < other.m_size; ++i) {
            std::construct_at(data() + i, other.data()[i]);
        }
        m_size = other.m_size;
    }

    constexpr void move_from(SmallVectorOptimized&& other) noexcept {
        m_size = other.m_size;
        
        if (other.is_heap_allocated()) {
            m_heapData = other.m_heapData;
            m_capacity = other.m_capacity;
            
            other.m_heapData = nullptr;
            other.m_capacity = N;
            other.m_size = 0;
        } else {
            for (size_type i = 0; i < m_size; ++i) {
                std::construct_at(data() + i, std::move(other.data()[i]));
                std::destroy_at(other.data() + i);
            }
            other.m_size = 0;
        }
    }

    alignas(T) std::byte m_localBuffer[N > 0 ? N * sizeof(T) : sizeof(T)]; 
    T* m_heapData{nullptr};
    size_type m_capacity{N};
    size_type m_size{0};
};

} // namespace EterBase
