#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <span>
#include <expected>
#include <functional>
#include <format>
#include "Result.h"

namespace Client::Core {

// Domenowe enumy bledu dla AppLifecycleCoordinator (bez polskich znakow)
enum class LifecycleError : uint8_t {
    DependencyNotFound,
    CircularDependencyDetected,
    InitializationFailed,
    AlreadyInitialized,
    NotInitialized
};

[[nodiscard]] std::string_view to_string(LifecycleError error) noexcept;

// Interfejs podsystemu (Subsystem Interface)
class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::string_view> GetDependencies() const noexcept = 0;
    
    // Metody cyklu zycia
    [[nodiscard]] virtual Result<void, LifecycleError> Initialize() noexcept = 0;
    virtual void Shutdown() noexcept = 0;
};

// Klasa koordynatora cyklu zycia aplikacji (AppLifecycleCoordinator)
class AppLifecycleCoordinator final {
public:
    AppLifecycleCoordinator() = default;
    ~AppLifecycleCoordinator();

    // Wylaczenie kopiowania/przenoszenia
    AppLifecycleCoordinator(const AppLifecycleCoordinator&) = delete;
    AppLifecycleCoordinator& operator=(const AppLifecycleCoordinator&) = delete;
    AppLifecycleCoordinator(AppLifecycleCoordinator&&) = delete;
    AppLifecycleCoordinator& operator=(AppLifecycleCoordinator&&) = delete;

    // Rejestracja podsystemu
    void RegisterSubsystem(std::unique_ptr<ISubsystem> subsystem);

    // Inicjalizacja z weryfikacja zaleznosci (sortowanie topologiczne)
    [[nodiscard]] Result<void, LifecycleError> InitializeAll() noexcept;

    // Zatrzymanie wszystkich podsystemow (w odwrotnej kolejnosci do inicjalizacji)
    void ShutdownAll() noexcept;

private:
    std::vector<std::unique_ptr<ISubsystem>> m_subsystems;
    std::vector<ISubsystem*> m_initializedOrder;
    bool m_isInitialized = false;

    // Pomocnicza metoda do sortowania topologicznego
    [[nodiscard]] Result<std::vector<ISubsystem*>, LifecycleError> ResolveDependencies() const noexcept;
};

} // namespace Client::Core

// Specjalizacje std::formatter dla formatowania C++20/23
template <>
struct std::formatter<Client::Core::LifecycleError> : std::formatter<std::string_view> {
    auto format(Client::Core::LifecycleError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
