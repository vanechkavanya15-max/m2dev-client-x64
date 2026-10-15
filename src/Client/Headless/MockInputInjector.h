#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>

namespace Client::Headless {

// Reprezentacja bledow wstrzykiwania wejscia
enum class InputError {
    InvalidKey,
    InvalidMouseState,
    EmptyInputSequence,
    BufferOverflow
};

// Struktura opisujaca stan klawisza
struct KeyState {
    char key;
    bool isPressed;
    uint32_t timestampMs;
};

// Struktura opisujaca stan myszy
struct MouseState {
    float x;
    float y;
    bool leftClick;
    bool rightClick;
    uint32_t timestampMs;
};

// Interfejs wstrzykiwacza dla srodowiska testowego
class MockInputInjector {
public:
    constexpr MockInputInjector() noexcept = default;
    
    // Wstrzykuje pojedyncze nacisniecie klawisza (np. W, A, S, D)
    constexpr std::expected<void, InputError> InjectKeyPress(char key, uint32_t timestampMs = 0) noexcept {
        key = ToUpper(key);
        if (!IsValidKey(key)) {
            return std::unexpected(InputError::InvalidKey);
        }
        
        if (m_keyEvents.size() >= MAX_EVENTS) {
            return std::unexpected(InputError::BufferOverflow);
        }
        
        m_keyEvents.push_back(KeyState{key, true, timestampMs});
        return {};
    }
    
    // Wstrzykuje zwolnienie klawisza
    constexpr std::expected<void, InputError> InjectKeyRelease(char key, uint32_t timestampMs = 0) noexcept {
        key = ToUpper(key);
        if (!IsValidKey(key)) {
            return std::unexpected(InputError::InvalidKey);
        }
        
        if (m_keyEvents.size() >= MAX_EVENTS) {
            return std::unexpected(InputError::BufferOverflow);
        }
        
        m_keyEvents.push_back(KeyState{key, false, timestampMs});
        return {};
    }
    
    // Wstrzykuje klikniecie myszy na danych koordynatach
    constexpr std::expected<void, InputError> InjectMouseClick(float x, float y, bool left, bool right, uint32_t timestampMs = 0) noexcept {
        // Podstawowa walidacja koordynatow - np. zapobieganie NaN/Inf (tutaj prosty check dla symulacji)
        if (x < -10000.0f || x > 10000.0f || y < -10000.0f || y > 10000.0f) {
            return std::unexpected(InputError::InvalidMouseState);
        }
        
        if (m_mouseEvents.size() >= MAX_EVENTS) {
            return std::unexpected(InputError::BufferOverflow);
        }
        
        m_mouseEvents.push_back(MouseState{x, y, left, right, timestampMs});
        return {};
    }
    
    // Pobiera zarejestrowane zdarzenia klawiatury
    constexpr std::span<const KeyState> GetKeyEvents() const noexcept {
        return {m_keyEvents.data(), m_keyEvents.size()};
    }
    
    // Pobiera zarejestrowane zdarzenia myszy
    constexpr std::span<const MouseState> GetMouseEvents() const noexcept {
        return {m_mouseEvents.data(), m_mouseEvents.size()};
    }
    
    // Czysci bufory zdarzen
    constexpr void Clear() noexcept {
        m_keyEvents.clear();
        m_mouseEvents.clear();
    }

private:
    // Limit zdarzen zeby zapobiec niekontrolowanej alokacji w trybie Headless
    static constexpr size_t MAX_EVENTS = 1024;
    
    // Bufor zdarzen - uzywamy vector z rezerwa pamieci (w constexpr C++20 vector jest dozwolony)
    std::vector<KeyState> m_keyEvents;
    std::vector<MouseState> m_mouseEvents;
    
    constexpr bool IsValidKey(char key) const noexcept {
        // Obsluga podstawowych klawiszy WASD oraz modyfikatorow (np. Q, E, Spacja itp)
        const std::array<char, 7> validKeys = {'W', 'A', 'S', 'D', 'Q', 'E', ' '};
        return std::ranges::find(validKeys, key) != validKeys.end();
    }
    
    constexpr char ToUpper(char c) const noexcept {
        if (c >= 'a' && c <= 'z') {
            return c - ('a' - 'A');
        }
        return c;
    }
};

} // namespace Client::Headless
