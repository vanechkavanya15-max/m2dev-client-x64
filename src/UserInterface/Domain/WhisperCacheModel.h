#pragma once

#include "src/UserInterface/Core/EventBus.h"
#include "src/EterBase/StrongTypes.h"
#include "src/EterBase/Result.h"
#include "src/EterBase/LogModern.h"

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>
#include <expected>
#include <format>
#include <cstdint>

namespace UserInterface::Domain {

/**
 * @brief Represents a single line in a whisper chat.
 */
struct WhisperLine {
    uint8_t type;
    std::string text;
    uint32_t timestamp;

    /**
     * @brief Constructs a new Whisper Line object
     * 
     * @param type The type of the whisper message (e.g., normal, error).
     * @param text The content of the whisper message.
     * @param timestamp The time the message was received/sent.
     */
    WhisperLine(uint8_t type, std::string_view text, uint32_t timestamp)
        : type(type), text(text), timestamp(timestamp) {}
};

/**
 * @brief Represents a thread of conversation with a single target.
 */
struct WhisperThread {
    std::string targetName;
    std::vector<WhisperLine> messages;

    /**
     * @brief Constructs a new Whisper Thread object
     * 
     * @param targetName The name of the target character.
     */
    explicit WhisperThread(std::string_view targetName)
        : targetName(targetName) {}
};

// ============================================================================
// Events
// ============================================================================

/**
 * @brief Event emitted when a whisper thread is updated (e.g., new message added).
 */
struct WhisperThreadUpdatedEvent : public Core::IEvent {
    std::string targetName;
    WhisperLine newLine;

    WhisperThreadUpdatedEvent(std::string_view targetName, const WhisperLine& newLine)
        : targetName(targetName), newLine(newLine) {}
};

/**
 * @brief Event emitted when a whisper thread is removed (e.g., cache eviction).
 */
struct WhisperThreadRemovedEvent : public Core::IEvent {
    std::string targetName;

    explicit WhisperThreadRemovedEvent(std::string_view targetName)
        : targetName(targetName) {}
};


/**
 * @brief Event emitted when a new whisper thread is created.
 */
struct WhisperThreadCreatedEvent : public Core::IEvent {
    std::string targetName;

    explicit WhisperThreadCreatedEvent(std::string_view targetName)
        : targetName(targetName) {}
};


// ============================================================================
// Domain Errors
// ============================================================================

enum class WhisperError : uint8_t {
    None = 0,
    ThreadNotFound,
    EmptyMessage,
    InvalidTarget
};

[[nodiscard]] constexpr std::string_view ToString(WhisperError err) noexcept {
    switch (err) {
        case WhisperError::None: return "None";
        case WhisperError::ThreadNotFound: return "ThreadNotFound";
        case WhisperError::EmptyMessage: return "EmptyMessage";
        case WhisperError::InvalidTarget: return "InvalidTarget";
    }
    return "UnknownWhisperError";
}

} // namespace UserInterface::Domain

template <>
struct std::formatter<UserInterface::Domain::WhisperError> : std::formatter<std::string_view> {
    auto format(UserInterface::Domain::WhisperError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(UserInterface::Domain::ToString(err), ctx);
    }
};

namespace UserInterface::Domain {

// ============================================================================
// Model
// ============================================================================

/**
 * @brief Manages the caching of whisper conversations, storing up to 5 recent threads.
 * 
 * This class adheres to the C++23 standards, using std::expected for error handling,
 * and decoupling from GUI via EventBus. It maintains an LRU (Least Recently Used)
 * policy for managing the threads.
 */
class WhisperCacheModel {
public:
    static constexpr size_t MAX_THREADS = 5;

    /**
     * @brief Constructs a new Whisper Cache Model object.
     */
    WhisperCacheModel() = default;
    
    /**
     * @brief Appends a new message to a whisper thread.
     * 
     * If the thread does not exist, it will be created. If adding the new thread exceeds
     * MAX_THREADS, the least recently used thread will be evicted.
     * 
     * @param targetName The name of the character being whispered to/from.
     * @param type The type of the whisper message.
     * @param text The content of the whisper message.
     * @param timestamp The time the message was received/sent.
     * @return std::expected<void, WhisperError> Success or an error code.
     */
    std::expected<void, WhisperError> AppendMessage(std::string_view targetName, uint8_t type, std::string_view text, uint32_t timestamp) {
        if (targetName.empty()) {
            EterBase::ModernLogger::Warn("Failed to append whisper message: Invalid target name.");
            return std::unexpected(WhisperError::InvalidTarget);
        }
        
        if (text.empty()) {
            EterBase::ModernLogger::Warn("Failed to append whisper message: Empty message.");
            return std::unexpected(WhisperError::EmptyMessage);
        }

        UpdateLru(targetName);

        auto it = threads_.find(targetName);
        if (it != threads_.end()) {
            WhisperLine newLine(type, text, timestamp);
            it->second.messages.push_back(newLine);
            Core::EventBus::GetInstance().Publish(WhisperThreadUpdatedEvent(targetName, newLine));
        }

        return {};
    }

    /**
     * @brief Retrieves a whisper thread by target name.
     * 
     * @param targetName The name of the target character.
     * @return std::optional<std::reference_wrapper<const WhisperThread>> The thread if found, otherwise std::nullopt.
     */
    std::optional<std::reference_wrapper<const WhisperThread>> GetThread(std::string_view targetName) const {
        auto it = threads_.find(targetName);
        if (it != threads_.end()) {
            return std::cref(it->second);
        }
        return std::nullopt;
    }

    /**
     * @brief Removes a whisper thread manually.
     * 
     * @param targetName The name of the target character.
     * @return std::expected<void, WhisperError> Success or an error code.
     */
    std::expected<void, WhisperError> RemoveThread(std::string_view targetName) {
        auto it = threads_.find(targetName);
        if (it == threads_.end()) {
            return std::unexpected(WhisperError::ThreadNotFound);
        }

        threads_.erase(it);
        
        std::erase_if(lruOrder_, [&](const std::string& name) {
            return name == targetName;
        });

        Core::EventBus::GetInstance().Publish(WhisperThreadRemovedEvent(targetName));

        return {};
    }

    /**
     * @brief Gets the ordered list of recent target names (most recent first).
     * 
     * @return const std::deque<std::string>& The list of target names.
     */
    const std::deque<std::string>& GetRecentTargets() const {
        return lruOrder_;
    }

private:
    /**
     * @brief Updates the LRU cache order for the given target name.
     * 
     * If the target is not in the cache, it adds it to the front. If the cache exceeds
     * MAX_THREADS, it evicts the least recently used thread. If the target is already
     * in the cache, it moves it to the front.
     * 
     * @param targetName The name of the target character.
     */
    void UpdateLru(std::string_view targetName) {
        // Remove if it exists in LRU to move it to the front
        auto lruIt = std::find(lruOrder_.begin(), lruOrder_.end(), targetName);
        if (lruIt != lruOrder_.end()) {
            lruOrder_.erase(lruIt);
        } else {
            // New thread
            if (threads_.find(targetName) == threads_.end()) {
                 std::string targetStr(targetName);
                 threads_.emplace(targetStr, WhisperThread(targetName));
                 Core::EventBus::GetInstance().Publish(WhisperThreadCreatedEvent(targetName));
            }
        }

        lruOrder_.push_front(std::string(targetName));

        // Evict if exceeded max threads
        if (lruOrder_.size() > MAX_THREADS) {
            std::string lruTarget = lruOrder_.back();
            lruOrder_.pop_back();
            threads_.erase(lruTarget);
            Core::EventBus::GetInstance().Publish(WhisperThreadRemovedEvent(lruTarget));
        }
    }

    struct string_hash {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char *txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(std::string_view txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string &txt) const {
            return std::hash<std::string>{}(txt);
        }
    };

    std::unordered_map<std::string, WhisperThread, string_hash, std::equal_to<>> threads_;
    std::deque<std::string> lruOrder_;
};

} // namespace UserInterface::Domain
