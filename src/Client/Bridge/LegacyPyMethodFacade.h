#pragma once

#include <cstdint>
#include <string_view>
#include <format>
#include "Client/Core/Result.h"

namespace Client::Bridge {

// ============================================================================
// Bledy wykonania dla fasady pythona
// ============================================================================
enum class PyBridgeError : uint8_t {
    InvalidEntityId,
    EmptyString,
    SystemNotInitialized
};

[[nodiscard]] constexpr std::string_view to_string(PyBridgeError error) noexcept {
    switch (error) {
        case PyBridgeError::InvalidEntityId: return "InvalidEntityId";
        case PyBridgeError::EmptyString: return "EmptyString";
        case PyBridgeError::SystemNotInitialized: return "SystemNotInitialized";
        default: return "UnknownPyBridgeError";
    }
}

/**
 * @brief Singleton fasady dla modulow Pythona ulatwiajacy eliminacje string coupling 
 * poprzez przekazywanie zdarzen bezposrednio na EventBus C++23.
 */
class LegacyPyMethodFacade {
public:
    static LegacyPyMethodFacade& Instance() noexcept;

    // Usuniecie mozliwosci kopiowania/przenoszenia
    LegacyPyMethodFacade(const LegacyPyMethodFacade&) = delete;
    LegacyPyMethodFacade& operator=(const LegacyPyMethodFacade&) = delete;
    LegacyPyMethodFacade(LegacyPyMethodFacade&&) = delete;
    LegacyPyMethodFacade& operator=(LegacyPyMethodFacade&&) = delete;

    [[nodiscard]] Core::Result<void, PyBridgeError> NotifyAnimHitFrame(uint32_t entityId, uint32_t motionKey, uint8_t hitIndex) noexcept;
    [[nodiscard]] Core::Result<void, PyBridgeError> NotifyTextTailVisibilityChanged(uint32_t entityId, bool isVisible) noexcept;
    [[nodiscard]] Core::Result<void, PyBridgeError> NotifyCustomTitleChanged(std::string_view title, uint32_t color) noexcept;
    [[nodiscard]] Core::Result<void, PyBridgeError> NotifyAnimFinished(uint32_t entityId, uint32_t motionKey) noexcept;
    [[nodiscard]] Core::Result<void, PyBridgeError> NotifyActorDead(uint32_t entityId) noexcept;

private:
    LegacyPyMethodFacade() = default;
    ~LegacyPyMethodFacade() = default;
};

} // namespace Client::Bridge

// Specjalizacja std::formatter do obslugi Result<T, E> dla PyBridgeError
template <>
struct std::formatter<Client::Bridge::PyBridgeError> : std::formatter<std::string_view> {
    auto format(Client::Bridge::PyBridgeError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Bridge::to_string(err), ctx);
    }
};
