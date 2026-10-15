#pragma once

#include <span>
#include <cstdint>
#include <optional>
#include <concepts>

namespace Client::Network {

/**
 * @class PacketDesyncRecovery
 * @brief Mechanizm odzyskiwania synchronizacji w strumieniu danych (AI-First Architecture).
 *
 * Sluzy do inteligentnego poszukiwania poprawnego naglowka pakietu po wykryciu 
 * desynchronizacji (np. zly opcode lub dlugosc). Pozwala na uratowanie polaczenia TCP
 * poprzez odrzucenie tylko uszkodzonych bajtow zamiast natychmiastowego zerwania.
 * Zapewnia brak alokacji (Zero-Allocation) oraz jest w pelni `constexpr`.
 */
class PacketDesyncRecovery {
public:
    /**
     * @brief Skanuje bufor w poszukiwaniu poprawnego naglowka dla trybu Ymir1B (1 bajt opcode).
     * 
     * Wykorzystuje Look-Ahead. `validator` powinien zwracac oczekiwana wielkosc pakietu (std::optional<size_t>)
     * dla danego opcode, lub std::nullopt jesli opcode jest nieznany.
     * Algorytm dodatkowo weryfikuje czy nastepny opcode (po uwzglednieniu dlugosci) tez jest poprawny,
     * aby wyeliminowac "false positives" jesli uszkodzony payload przypadkowo ma wartosc poprawnego opcode.
     * 
     * @param buffer Otrzymany, czesciowo uszkodzony bufor.
     * @param validator Funkcja typu: uint8_t -> std::optional<size_t>
     * @return std::optional<size_t> Zwraca ilosc bajtow do odrzucenia (offset), aby trafic na poczatek poprawnego pakietu.
     */
    template<typename Validator>
    requires std::invocable<Validator, uint8_t> && 
             std::same_as<std::invoke_result_t<Validator, uint8_t>, std::optional<size_t>>
    [[nodiscard]] static constexpr std::optional<size_t> FindSyncPointYmir1B(
        std::span<const uint8_t> buffer, 
        Validator&& validator) noexcept;

    /**
     * @brief Skanuje bufor w poszukiwaniu poprawnego naglowka dla trybu M2Dev4B.
     * 
     * Header M2Dev4B sklada sie z 2 bajtow opcode (Little-Endian) i 2 bajtow dlugosci (Little-Endian).
     * Skanuje strumien bajt po bajcie.
     * 
     * @param buffer Otrzymany, czesciowo uszkodzony bufor.
     * @param validator Funkcja typu: (uint16_t opcode, uint16_t length) -> bool
     * @return std::optional<size_t> Zwraca ilosc bajtow do odrzucenia (offset), aby trafic na poczatek poprawnego pakietu.
     */
    template<typename Validator>
    requires std::predicate<Validator, uint16_t, uint16_t>
    [[nodiscard]] static constexpr std::optional<size_t> FindSyncPointM2Dev4B(
        std::span<const uint8_t> buffer, 
        Validator&& validator) noexcept;
};

// ============================================================================
// Implementacja Ymir1B (Look-Ahead)
// ============================================================================
template<typename Validator>
requires std::invocable<Validator, uint8_t> && 
         std::same_as<std::invoke_result_t<Validator, uint8_t>, std::optional<size_t>>
constexpr std::optional<size_t> PacketDesyncRecovery::FindSyncPointYmir1B(
    std::span<const uint8_t> buffer, 
    Validator&& validator) noexcept 
{
    if (buffer.empty()) {
        return std::nullopt;
    }

    // Zaczynamy szukac od offsetu 1. Zakladamy ze offset 0 jest uszkodzony 
    // (stad wezwanie funkcji odzyskiwania).
    for (size_t i = 1; i < buffer.size(); ++i) {
        uint8_t candidate_opcode = buffer[i];
        auto expected_size = validator(candidate_opcode);
        
        if (expected_size.has_value()) {
            size_t size = expected_size.value();
            
            // Jesli bufor jest wystarczajaco dlugi, probujemy zaawansowanego Look-Ahead.
            // Sprawdzamy czy nastepny bajt po deklarowanej wielkosci pakietu 
            // tez moglby byc poprawnym opcode (jesli istnieje).
            // Uzywamy size < buffer.size() - i zeby uniknac integer overflow.
            if (size < buffer.size() - i) {
                uint8_t next_opcode = buffer[i + size];
                if (validator(next_opcode).has_value()) {
                    return i; // Wysokie prawdopodobienstwo ze to poprawny sync-point.
                }
            } else {
                // Koniec bufora nie pozwala na Look-Ahead, ale sam opcode jest poprawny.
                // Akceptujemy to jako fallback.
                return i;
            }
        }
    }

    return std::nullopt;
}

// ============================================================================
// Implementacja M2Dev4B
// ============================================================================
template<typename Validator>
requires std::predicate<Validator, uint16_t, uint16_t>
constexpr std::optional<size_t> PacketDesyncRecovery::FindSyncPointM2Dev4B(
    std::span<const uint8_t> buffer, 
    Validator&& validator) noexcept 
{
    // Minimalny rozmiar naglowka to 4 bajty (2 opcode + 2 length).
    if (buffer.size() < 4) {
        return std::nullopt;
    }

    // Przesuwamy sie co 1 bajt, szukajac od offsetu 1.
    for (size_t i = 1; i <= buffer.size() - 4; ++i) {
        uint16_t candidate_opcode = static_cast<uint16_t>(buffer[i] | (buffer[i+1] << 8));
        uint16_t candidate_length = static_cast<uint16_t>(buffer[i+2] | (buffer[i+3] << 8));
        
        if (validator(candidate_opcode, candidate_length)) {
            // Mozna by dodac Look-Ahead dla M2Dev4B w przyszlosci, 
            // ale header z 4 bajtami jest juz statystycznie silny przed "false positives".
            return i;
        }
    }

    return std::nullopt;
}

} // namespace Client::Network
