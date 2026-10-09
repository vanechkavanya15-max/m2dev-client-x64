#pragma once

#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <shared_mutex>
#include <expected>
#include <string_view>
#include <format>
#include "../Core/Result.h"

namespace Client::Gameplay {

/**
 * @brief Enum of buff types tracked by this system.
 * Wymagane scisle sledzenie: Aura Miecza, Silne Cialo, Berek, Ochrona.
 */
enum class TrackedBuffType : uint8_t {
    None = 0,
    AuraMiecza = 1,
    SilneCialo = 2,
    Berek = 3,
    Ochrona = 4
};

/**
 * @brief Domain errors for buff tracking operations.
 */
enum class BuffTrackerError : uint8_t {
    None = 0,
    NotFound,
    AlreadyActive,
    InvalidDuration,
    InvalidBuffType
};

[[nodiscard]] constexpr std::string_view to_string(BuffTrackerError error) noexcept {
    switch (error) {
        case BuffTrackerError::None: return "BuffTrackerError::None";
        case BuffTrackerError::NotFound: return "BuffTrackerError::NotFound - Buff is not currently active";
        case BuffTrackerError::AlreadyActive: return "BuffTrackerError::AlreadyActive - Buff is already active";
        case BuffTrackerError::InvalidDuration: return "BuffTrackerError::InvalidDuration - Provided duration is zero or negative";
        case BuffTrackerError::InvalidBuffType: return "BuffTrackerError::InvalidBuffType - Provided buff type is invalid (None)";
        default: return "BuffTrackerError::Unknown";
    }
}

template <typename T>
using BuffResult = std::expected<T, BuffTrackerError>;
using VoidBuffResult = std::expected<void, BuffTrackerError>;

/**
 * @brief Precise duration tracker for critical player buffs.
 * Zapewnia precyzyjne odliczanie czasu i bezpieczenstwo w srodowisku wielowatkowym.
 */
class BuffDurationTracker {
public:
    struct BuffEntry {
        uint64_t startTimestampMs{0};
        uint32_t durationMs{0};
    };

    /**
     * @brief Starts tracking a buff for a given duration.
     * @param buffType Type of the buff.
     * @param durationMs Duration of the buff in milliseconds.
     * @return Success or domain error if validation fails.
     */
    VoidBuffResult StartBuff(TrackedBuffType buffType, uint32_t durationMs);
    
    /**
     * @brief Starts tracking a buff using std::chrono.
     * @param buffType Type of the buff.
     * @param duration Duration of the buff.
     * @return Success or domain error if validation fails.
     */
    VoidBuffResult StartBuff(TrackedBuffType buffType, std::chrono::milliseconds duration);

    /**
     * @brief Checks if a specific buff is currently active (duration not expired).
     * @param buffType Type of the buff.
     * @return True if active, false otherwise.
     */
    bool IsBuffActive(TrackedBuffType buffType) const;
    
    /**
     * @brief Checks if a specific buff is active at a specific timestamp.
     * @param buffType Type of the buff.
     * @param currentTimestampMs Timestamp to check against.
     * @return True if active, false otherwise.
     */
    bool IsBuffActive(TrackedBuffType buffType, uint64_t currentTimestampMs) const;

    /**
     * @brief Gets the remaining time for a buff in milliseconds.
     * @param buffType Type of the buff.
     * @return Remaining milliseconds or error if not found.
     */
    BuffResult<uint32_t> GetRemainingTimeMs(TrackedBuffType buffType) const;
    
    /**
     * @brief Gets the remaining time for a buff in milliseconds at a given timestamp.
     * @param buffType Type of the buff.
     * @param currentTimestampMs Timestamp to check against.
     * @return Remaining milliseconds or error if not found.
     */
    BuffResult<uint32_t> GetRemainingTimeMs(TrackedBuffType buffType, uint64_t currentTimestampMs) const;

    /**
     * @brief Gets the remaining time for a buff as std::chrono::milliseconds.
     * @param buffType Type of the buff.
     * @return Remaining time as std::chrono::milliseconds or error if not found.
     */
    BuffResult<std::chrono::milliseconds> GetRemainingTime(TrackedBuffType buffType) const;

    /**
     * @brief Refreshes an active buff with a new duration.
     * @param buffType Type of the buff.
     * @param durationMs New duration in milliseconds.
     * @return Success or error if not found or invalid duration.
     */
    VoidBuffResult RefreshBuff(TrackedBuffType buffType, uint32_t durationMs);

    /**
     * @brief Ends an active buff prematurely.
     * @param buffType Type of the buff.
     * @return Success or error if not found.
     */
    VoidBuffResult RemoveBuff(TrackedBuffType buffType);

    /**
     * @brief Clears all currently tracked buffs.
     */
    void ClearAll();

    /**
     * @brief Utility method to get the current timestamp in milliseconds.
     */
    static uint64_t GetCurrentTimeMs() noexcept {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    }

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<TrackedBuffType, BuffEntry> m_buffs;
};

} // namespace Client::Gameplay

template <>
struct std::formatter<Client::Gameplay::BuffTrackerError> : std::formatter<std::string_view> {
    auto format(Client::Gameplay::BuffTrackerError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Gameplay::to_string(err), ctx);
    }
};
