#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <span>
#include <expected>
#include <string_view>
#include <format>
#include <cstring>

namespace Client::Core {

/**
 * @brief Enum reprezentujacy bledy mechanizmu rejestrowania zdarzen.
 */
enum class RecorderError : uint8_t {
    None = 0,
    BufferOverflow,
    TypeNotRegistered,
    DeserializationFailed,
    InvalidHeader
};

template <typename T, typename E = RecorderError>
using RecorderResult = std::expected<T, E>;

[[nodiscard]] constexpr std::string_view to_string(RecorderError error) noexcept {
    switch (error) {
        case RecorderError::None: return "RecorderError::None";
        case RecorderError::BufferOverflow: return "RecorderError::BufferOverflow - Brak miejsca w buforze na nowe zdarzenie";
        case RecorderError::TypeNotRegistered: return "RecorderError::TypeNotRegistered - Typ zdarzenia nie zostal zarejestrowany";
        case RecorderError::DeserializationFailed: return "RecorderError::DeserializationFailed - Blad odczytu danych zdarzenia";
        case RecorderError::InvalidHeader: return "RecorderError::InvalidHeader - Uszkodzony naglowek zdarzenia w buforze";
        default: return "RecorderError::Unknown";
    }
}

/**
 * @brief Klasa sluzaca do serializacji i deserializacji zdarzen domenowych w pamieci.
 * Umozliwia nagrywanie przebiegu dzialania systemu i jego odtwarzanie (Replay).
 */
class EventRecorder {
public:
    using EventTypeId = uint32_t;
    using Serializer = std::function<RecorderResult<void>(const void* eventData, std::span<uint8_t> buffer, size_t& offset)>;
    using Deserializer = std::function<RecorderResult<void>(std::span<const uint8_t> buffer, size_t& offset, const std::function<void(const void*)>& callback)>;

    explicit EventRecorder(size_t maxBufferSize) : maxBufferSize_(maxBufferSize) {
        buffer_.reserve(maxBufferSize);
    }

    /**
     * @brief Rejestruje funkcje serializujace dla podanego typu zdarzenia.
     */
    template <typename T>
    void RegisterEventType(EventTypeId typeId) {
        static_assert(std::is_trivially_copyable_v<T>, "EventRecorder moze serializowac tylko trywialnie kopiowalne typy");
        static_assert(std::is_default_constructible_v<T>, "EventRecorder wymaga aby typ mial domyslny konstruktor");

        serializers_[typeid(T)] = [typeId](const void* eventData, std::span<uint8_t> buffer, size_t& offset) -> RecorderResult<void> {
            const T* typedEvent = static_cast<const T*>(eventData);
            
            size_t eventSize = sizeof(T);
            size_t totalSize = sizeof(EventTypeId) + sizeof(size_t) + eventSize;
            
            if (offset + totalSize > buffer.size()) {
                return std::unexpected(RecorderError::BufferOverflow);
            }
            
            std::memcpy(buffer.data() + offset, &typeId, sizeof(EventTypeId));
            offset += sizeof(EventTypeId);
            
            std::memcpy(buffer.data() + offset, &eventSize, sizeof(size_t));
            offset += sizeof(size_t);
            
            std::memcpy(buffer.data() + offset, typedEvent, eventSize);
            offset += eventSize;
            
            return {};
        };
        
        deserializers_[typeId] = [](std::span<const uint8_t> buffer, size_t& offset, const std::function<void(const void*)>& callback) -> RecorderResult<void> {
            if (offset + sizeof(size_t) > buffer.size()) {
                return std::unexpected(RecorderError::DeserializationFailed);
            }
            
            size_t eventSize = 0;
            std::memcpy(&eventSize, buffer.data() + offset, sizeof(size_t));
            offset += sizeof(size_t);
            
            if (eventSize != sizeof(T) || offset + eventSize > buffer.size()) {
                return std::unexpected(RecorderError::DeserializationFailed);
            }
            
            T typedEvent;
            std::memcpy(&typedEvent, buffer.data() + offset, eventSize);
            offset += eventSize;
            
            callback(&typedEvent);
            return {};
        };
    }

    /**
     * @brief Zapisuje zdarzenie do wewnetrznego bufora.
     */
    template <typename T>
    RecorderResult<void> RecordEvent(const T& event) {
        auto it = serializers_.find(typeid(T));
        if (it == serializers_.end()) {
            return std::unexpected(RecorderError::TypeNotRegistered);
        }
        
        size_t requiredExtra = sizeof(EventTypeId) + sizeof(size_t) + sizeof(T);
        if (buffer_.size() + requiredExtra > maxBufferSize_) {
            return std::unexpected(RecorderError::BufferOverflow);
        }
        
        size_t originalSize = buffer_.size();
        buffer_.resize(originalSize + requiredExtra); 
        
        std::span<uint8_t> bufferSpan(buffer_);
        size_t currentOffset = originalSize;
        
        auto result = it->second(&event, bufferSpan, currentOffset);
        
        if (!result) {
            buffer_.resize(originalSize);
        } else {
            buffer_.resize(currentOffset);
        }
        
        return result;
    }

    /**
     * @brief Odtwarza wszystkie zapisane zdarzenia i przekazuje je do callbacku.
     */
    RecorderResult<void> ReplayAll(const std::function<void(EventTypeId, const void*)>& onEvent) const {
        std::span<const uint8_t> bufferSpan(buffer_);
        size_t offset = 0;
        
        while (offset < bufferSpan.size()) {
            if (offset + sizeof(EventTypeId) > bufferSpan.size()) {
                return std::unexpected(RecorderError::InvalidHeader);
            }
            
            EventTypeId typeId;
            std::memcpy(&typeId, bufferSpan.data() + offset, sizeof(EventTypeId));
            offset += sizeof(EventTypeId);
            
            auto it = deserializers_.find(typeId);
            if (it == deserializers_.end()) {
                return std::unexpected(RecorderError::TypeNotRegistered);
            }
            
            auto result = it->second(bufferSpan, offset, [&onEvent, typeId](const void* eventPtr) {
                onEvent(typeId, eventPtr);
            });
            
            if (!result) {
                return result;
            }
        }
        
        return {};
    }

    /**
     * @brief Czysci zapisane zdarzenia.
     */
    void Clear() noexcept {
        buffer_.clear();
    }

    /**
     * @brief Zwraca widok na zserializowane dane zdarzen.
     */
    [[nodiscard]] std::span<const uint8_t> GetBuffer() const noexcept {
        return buffer_;
    }

private:
    size_t maxBufferSize_;
    std::vector<uint8_t> buffer_;
    std::unordered_map<std::type_index, Serializer> serializers_;
    std::unordered_map<EventTypeId, Deserializer> deserializers_;
};

} // namespace Client::Core

template <>
struct std::formatter<Client::Core::RecorderError> : std::formatter<std::string_view> {
    auto format(Client::Core::RecorderError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Core::to_string(err), ctx);
    }
};
