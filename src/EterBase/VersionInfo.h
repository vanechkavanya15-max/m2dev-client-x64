#pragma once

#include <cstdint>
#include <string_view>
#include <span>

/**
 * @brief Provides build and version information for the client.
 */
class VersionInfo {
public:
    /**
     * @brief Retrieves the major version number.
     * @return The major version number.
     */
    static constexpr uint32_t GetMajorVersion() noexcept {
        return 2026;
    }

    /**
     * @brief Retrieves the minor version number.
     * @return The minor version number.
     */
    static constexpr uint32_t GetMinorVersion() noexcept {
        return 1;
    }

    /**
     * @brief Retrieves the version string.
     * @return A string view representing the complete version.
     */
    static constexpr std::string_view GetVersionString() noexcept {
        return "2026.1";
    }

    /**
     * @brief Retrieves the build timestamp.
     * @return A string view of the compilation date and time.
     */
    static constexpr std::string_view GetBuildTimestamp() noexcept {
        return __DATE__ " " __TIME__;
    }

    /**
     * @brief Checks if the compilation architecture is x64.
     * @return True if compiled for x64, false otherwise.
     */
    static constexpr bool IsArchitectureX64() noexcept {
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Retrieves the compilation architecture string.
     * @return A string view representing the architecture.
     */
    static constexpr std::string_view GetArchitectureString() noexcept {
        return IsArchitectureX64() ? "x64" : "x86";
    }

    /**
     * @brief Retrieves a binary identifier for the version.
     * @return A span of bytes representing the binary version identifier.
     */
    static constexpr std::span<const uint8_t> GetBinaryIdentifier() noexcept {
        return std::span<const uint8_t>(IdentifierBytes);
    }

private:
    static constexpr uint8_t IdentifierBytes[] = {0x02, 0x00, 0x02, 0x06, 0x00, 0x01};
};
