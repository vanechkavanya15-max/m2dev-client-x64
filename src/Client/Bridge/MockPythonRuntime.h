#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <span>
#include <optional>
#include <functional>
#include <format>
#include <type_traits>
#include <cstdint>
#include <expected>
#include "EterBase/Result.h"

namespace Client::Bridge {

enum class PythonRuntimeError : uint8_t {
    None = 0,
    NotInitialized,
    ModuleNotFound,
    MethodNotFound,
    ExecutionFailed,
    InvalidArgument
};

[[nodiscard]] constexpr std::string_view ToString(PythonRuntimeError err) noexcept {
    switch (err) {
        case PythonRuntimeError::None: return "None";
        case PythonRuntimeError::NotInitialized: return "NotInitialized";
        case PythonRuntimeError::ModuleNotFound: return "ModuleNotFound";
        case PythonRuntimeError::MethodNotFound: return "MethodNotFound";
        case PythonRuntimeError::ExecutionFailed: return "ExecutionFailed";
        case PythonRuntimeError::InvalidArgument: return "InvalidArgument";
    }
    return "UnknownPythonRuntimeError";
}

struct PythonCallRecord {
    std::string moduleName;
    std::string methodName;
    std::vector<std::string> args;

    [[nodiscard]] bool operator==(const PythonCallRecord& other) const noexcept = default;
};

class MockPythonRuntime final {
public:
    using CallbackType = std::function<EterBase::Result<std::string, PythonRuntimeError>(std::span<const std::string>)>;

    MockPythonRuntime() = default;
    ~MockPythonRuntime() = default;

    MockPythonRuntime(const MockPythonRuntime&) = delete;
    MockPythonRuntime& operator=(const MockPythonRuntime&) = delete;
    MockPythonRuntime(MockPythonRuntime&&) = delete;
    MockPythonRuntime& operator=(MockPythonRuntime&&) = delete;

    void Initialize() noexcept { m_isInitialized = true; }
    void Shutdown() noexcept { 
        m_isInitialized = false; 
        m_history.clear();
        m_mocks.clear();
    }

    void RegisterMock(std::string_view moduleName, std::string_view methodName, CallbackType callback) {
        m_mocks[GetMockKey(moduleName, methodName)] = std::move(callback);
    }

    EterBase::Result<std::string, PythonRuntimeError> ExecuteMethod(
        std::string_view moduleName, 
        std::string_view methodName, 
        std::span<const std::string> args = {}) 
    {
        if (!m_isInitialized) {
            return std::unexpected(PythonRuntimeError::NotInitialized);
        }

        m_history.emplace_back(PythonCallRecord{
            std::string(moduleName),
            std::string(methodName),
            std::vector<std::string>(args.begin(), args.end())
        });

        auto it = m_mocks.find(GetMockKey(moduleName, methodName));
        if (it != m_mocks.end()) {
            return it->second(args);
        }

        return std::unexpected(PythonRuntimeError::MethodNotFound);
    }

    template <typename T>
    static std::string FormatArg(T&& val) {
        using DecayedT = std::remove_cvref_t<T>;
        if constexpr (std::is_convertible_v<DecayedT, std::string_view>) {
            return std::string(std::string_view(val));
        } else if constexpr (std::is_arithmetic_v<DecayedT>) {
            return std::to_string(val);
        } else {
            return "[unsupported_type]";
        }
    }

    template <typename... Args>
    EterBase::Result<std::string, PythonRuntimeError> Invoke(
        std::string_view moduleName, 
        std::string_view methodName, 
        Args&&... args)
    {
        if (!m_isInitialized) {
            return std::unexpected(PythonRuntimeError::NotInitialized);
        }

        std::vector<std::string> stringArgs;
        if constexpr (sizeof...(Args) > 0) {
            stringArgs = { FormatArg(std::forward<Args>(args))... };
        }
        return ExecuteMethod(moduleName, methodName, stringArgs);
    }

    [[nodiscard]] std::span<const PythonCallRecord> GetHistory() const noexcept {
        return m_history;
    }

    void ClearHistory() noexcept {
        m_history.clear();
    }

private:
    [[nodiscard]] static std::string GetMockKey(std::string_view moduleName, std::string_view methodName) {
        return std::string(moduleName) + "::" + std::string(methodName);
    }

    bool m_isInitialized{false};
    std::vector<PythonCallRecord> m_history;
    std::unordered_map<std::string, CallbackType> m_mocks;
};

} // namespace Client::Bridge

template <>
struct std::formatter<Client::Bridge::PythonRuntimeError> : std::formatter<std::string_view> {
    auto format(Client::Bridge::PythonRuntimeError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Bridge::ToString(err), ctx);
    }
};

