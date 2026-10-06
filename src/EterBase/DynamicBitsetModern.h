#pragma once

#include <vector>
#include <cstdint>
#include <expected>
#include <string_view>
#include <format>
#include <bit>
#include <algorithm>

#include "Result.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @class DynamicBitsetModern
 * @brief Nowoczesny wektor bitowy ze slowami 64-bitowymi (C++23).
 *
 * Klasa zapewnia bezpieczne operacje na bitach z wykorzystaniem std::expected
 * w celu sygnalizacji bledow (np. przekroczenia zakresu). 
 * Wykorzystuje bloki 64-bitowe dla optymalnej wydajnosci i zapobiega wyciekom
 * oraz archaicznej obsludze bledow.
 */
class DynamicBitsetModern {
public:
    /**
     * @brief Konstruktor domyslny, inicjuje pusty zbior bitow.
     */
    DynamicBitsetModern() = default;

    /**
     * @brief Konstruktor inicjujacy bitset o ustalonej liczbie bitow.
     * @param numBits Liczba bitow do zaalokowania.
     */
    explicit DynamicBitsetModern(std::size_t numBits) {
        Resize(numBits);
    }

    /**
     * @brief Ustawia rozmiar bitsetu.
     * @param numBits Nowa liczba bitow.
     */
    void Resize(std::size_t numBits) {
        numBits_ = numBits;
        blocks_.resize((numBits + 63) / 64, 0ULL);
        if (numBits > 0 && numBits % 64 != 0) {
            blocks_.back() &= (1ULL << (numBits % 64)) - 1;
        }
    }

    /**
     * @brief Zwraca liczbe przechowywanych bitow.
     * @return Liczba bitow.
     */
    [[nodiscard]] std::size_t Size() const noexcept {
        return numBits_;
    }

    /**
     * @brief Sprawdza, czy bitset jest pusty.
     * @return True, jesli bitset nie zawiera zadnych bitow, w przeciwnym razie False.
     */
    [[nodiscard]] bool Empty() const noexcept {
        return numBits_ == 0;
    }

    /**
     * @brief Ustawia wybrany bit na 1 (prawda).
     * @param index Indeks bitu do ustawienia.
     * @return EterBase::VoidResult Oczekiwany rezultat bezwartosciowy lub powod bledu (np. poza zakresem).
     */
    [[nodiscard]] EterBase::VoidResult<> Set(std::size_t index) {
        if (index >= numBits_) {
            EterBase::ModernLogger::Error("DynamicBitsetModern::Set - Index {} out of bounds (Size: {})", index, numBits_);
            return EterBase::MakeError("Index out of bounds");
        }
        
        blocks_[index / 64] |= (1ULL << (index % 64));
        return {};
    }

    /**
     * @brief Resetuje (zeruje) wybrany bit na 0 (falsz).
     * @param index Indeks bitu do zresetowania.
     * @return EterBase::VoidResult Oczekiwany rezultat bezwartosciowy lub powod bledu.
     */
    [[nodiscard]] EterBase::VoidResult<> Reset(std::size_t index) {
        if (index >= numBits_) {
            EterBase::ModernLogger::Error("DynamicBitsetModern::Reset - Index {} out of bounds (Size: {})", index, numBits_);
            return EterBase::MakeError("Index out of bounds");
        }
        
        blocks_[index / 64] &= ~(1ULL << (index % 64));
        return {};
    }

    /**
     * @brief Przelacza wartosc bitu (0 -> 1, 1 -> 0).
     * @param index Indeks bitu do przelaczenia.
     * @return EterBase::VoidResult Oczekiwany rezultat bezwartosciowy lub powod bledu.
     */
    [[nodiscard]] EterBase::VoidResult<> Toggle(std::size_t index) {
        if (index >= numBits_) {
            EterBase::ModernLogger::Error("DynamicBitsetModern::Toggle - Index {} out of bounds (Size: {})", index, numBits_);
            return EterBase::MakeError("Index out of bounds");
        }
        
        blocks_[index / 64] ^= (1ULL << (index % 64));
        return {};
    }

    /**
     * @brief Sprawdza wartosc wybranego bitu.
     * @param index Indeks bitu do sprawdzenia.
     * @return EterBase::Result<bool> Zwraca true, jesli bit jest zapalony, lub blad jesli indeks jest poza zakresem.
     */
    [[nodiscard]] EterBase::Result<bool> Test(std::size_t index) const {
        if (index >= numBits_) {
            EterBase::ModernLogger::Error("DynamicBitsetModern::Test - Index {} out of bounds (Size: {})", index, numBits_);
            return EterBase::MakeError("Index out of bounds");
        }
        
        return (blocks_[index / 64] & (1ULL << (index % 64))) != 0;
    }

    /**
     * @brief Resetuje wszystkie bity w strukturze na 0.
     */
    void ClearAll() noexcept {
        std::fill(blocks_.begin(), blocks_.end(), 0ULL);
    }

    /**
     * @brief Liczy zapalone bity (popcount).
     * @return Liczba zapalonych bitow (set bit count).
     */
    [[nodiscard]] std::size_t Count() const noexcept {
        std::size_t count = 0;
        for (const auto block : blocks_) {
            count += static_cast<std::size_t>(std::popcount(block));
        }
        return count;
    }

private:
    std::vector<uint64_t> blocks_;
    std::size_t numBits_ = 0;
};

} // namespace EterBase
