#pragma once

#include <unordered_map>
#include <optional>
#include <expected>
#include <cstdint>
#include <format>
#include <memory>
#include <string_view>

#include "../EterBase/StrongTypes.h"
#include "../EterBase/Result.h"
#include "../EterBase/LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Enum representing possible errors during chunk streaming operations.
 */
enum class ChunkCacheError : uint8_t {
    None = 0,
    ChunkNotFound,
    AlreadyLoaded,
    InvalidCoordinates,
    FileSystemError
};

/**
 * @brief Converts ChunkCacheError to string view for logging.
 * @param err The error to convert.
 * @return String representation of the error.
 */
[[nodiscard]] constexpr std::string_view ToString(ChunkCacheError err) noexcept {
    switch (err) {
        case ChunkCacheError::None: return "None";
        case ChunkCacheError::ChunkNotFound: return "ChunkNotFound";
        case ChunkCacheError::AlreadyLoaded: return "AlreadyLoaded";
        case ChunkCacheError::InvalidCoordinates: return "InvalidCoordinates";
        case ChunkCacheError::FileSystemError: return "FileSystemError";
    }
    return "UnknownChunkCacheError";
}

} // namespace GameLib

template <>
struct std::formatter<GameLib::ChunkCacheError> : std::formatter<std::string_view> {
    auto format(GameLib::ChunkCacheError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(GameLib::ToString(err), ctx);
    }
};

namespace GameLib {

/**
 * @brief Event triggered when a map chunk is successfully loaded into cache.
 */
struct ChunkLoadedEvent : public UserInterface::Core::IEvent {
    EterBase::MapIndex mapIndex;
    int32_t chunkX;
    int32_t chunkY;

    /**
     * @brief Constructs a ChunkLoadedEvent.
     * @param mapIndex Index of the map.
     * @param chunkX X coordinate of the chunk.
     * @param chunkY Y coordinate of the chunk.
     */
    ChunkLoadedEvent(EterBase::MapIndex mapIndex, int32_t chunkX, int32_t chunkY)
        : mapIndex(mapIndex), chunkX(chunkX), chunkY(chunkY) {}
};

/**
 * @brief Event triggered when a map chunk is unloaded from cache.
 */
struct ChunkUnloadedEvent : public UserInterface::Core::IEvent {
    EterBase::MapIndex mapIndex;
    int32_t chunkX;
    int32_t chunkY;

    /**
     * @brief Constructs a ChunkUnloadedEvent.
     * @param mapIndex Index of the map.
     * @param chunkX X coordinate of the chunk.
     * @param chunkY Y coordinate of the chunk.
     */
    ChunkUnloadedEvent(EterBase::MapIndex mapIndex, int32_t chunkX, int32_t chunkY)
        : mapIndex(mapIndex), chunkX(chunkX), chunkY(chunkY) {}
};

/**
 * @brief Represents coordinates of a chunk in the map grid.
 */
struct ChunkCoordinates {
    EterBase::MapIndex mapIndex;
    int32_t x;
    int32_t y;

    auto operator<=>(const ChunkCoordinates&) const = default;
};

} // namespace GameLib

template <>
struct std::hash<GameLib::ChunkCoordinates> {
    std::size_t operator()(const GameLib::ChunkCoordinates& coords) const noexcept {
        std::size_t h1 = std::hash<int32_t>{}(coords.mapIndex.value());
        std::size_t h2 = std::hash<int32_t>{}(coords.x);
        std::size_t h3 = std::hash<int32_t>{}(coords.y);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

namespace GameLib {

/**
 * @brief Structure holding data of a map chunk.
 */
struct ChunkData {
    bool isReady{false};
};

/**
 * @brief Modern C++23 Cache Manager for map chunks streaming.
 *
 * It utilizes std::expected for error handling, monadic std::optional,
 * and decouples UI via EventBus.
 */
class MapChunkStreamingCache {
public:
    using ChunkResult = EterBase::Result<void, ChunkCacheError>;

    /**
     * @brief Constructor for the streaming cache.
     */
    MapChunkStreamingCache() = default;

    /**
     * @brief Destructor.
     */
    ~MapChunkStreamingCache() = default;

    /**
     * @brief Requests loading of a specific chunk.
     * 
     * Uses std::expected for robust error handling. Emits ChunkLoadedEvent on success.
     * 
     * @param coords The coordinates of the chunk to load.
     * @return ChunkResult indicating success or specific ChunkCacheError.
     */
    ChunkResult LoadChunk(const ChunkCoordinates& coords) {
        if (coords.x < 0 || coords.y < 0) {
            EterBase::ModernLogger::Warn("Failed to load chunk: Invalid coordinates ({}, {})", coords.x, coords.y);
            return std::unexpected(ChunkCacheError::InvalidCoordinates);
        }

        if (cache.contains(coords)) {
            EterBase::ModernLogger::Info("Chunk already loaded: map {}, x {}, y {}", coords.mapIndex.value(), coords.x, coords.y);
            return std::unexpected(ChunkCacheError::AlreadyLoaded);
        }

        auto newChunk = std::make_shared<ChunkData>();
        newChunk->isReady = true;

        cache.emplace(coords, std::move(newChunk));
        
        EterBase::ModernLogger::Info("Chunk loaded successfully: map {}, x {}, y {}", coords.mapIndex.value(), coords.x, coords.y);
        
        UserInterface::Core::EventBus::GetInstance().Publish(
            ChunkLoadedEvent{coords.mapIndex, coords.x, coords.y}
        );

        return {};
    }

    /**
     * @brief Unloads a specific chunk from the cache.
     * 
     * @param coords The coordinates of the chunk to unload.
     * @return ChunkResult indicating success or specific ChunkCacheError.
     */
    ChunkResult UnloadChunk(const ChunkCoordinates& coords) {
        auto it = cache.find(coords);
        if (it == cache.end()) {
            EterBase::ModernLogger::Warn("Failed to unload chunk: Not found at ({}, {})", coords.x, coords.y);
            return std::unexpected(ChunkCacheError::ChunkNotFound);
        }

        cache.erase(it);
        
        EterBase::ModernLogger::Info("Chunk unloaded successfully: map {}, x {}, y {}", coords.mapIndex.value(), coords.x, coords.y);

        UserInterface::Core::EventBus::GetInstance().Publish(
            ChunkUnloadedEvent{coords.mapIndex, coords.x, coords.y}
        );

        return {};
    }

    /**
     * @brief Retrieves a chunk from the cache if it is loaded and ready.
     * 
     * Demonstrates the use of monadic std::optional operations (and_then).
     * 
     * @param coords The coordinates of the chunk to retrieve.
     * @return std::optional containing a shared pointer to ChunkData if found and ready.
     */
    [[nodiscard]] std::optional<std::shared_ptr<ChunkData>> GetChunk(const ChunkCoordinates& coords) const {
        return CheckCache(coords)
            .and_then([](const std::shared_ptr<ChunkData>& chunk) -> std::optional<std::shared_ptr<ChunkData>> {
                if (chunk && chunk->isReady) {
                    return chunk;
                }
                return std::nullopt;
            });
    }

    /**
     * @brief Checks if a chunk is ready, using std::optional::transform.
     * 
     * @param coords The coordinates of the chunk.
     * @return std::optional<bool> true if ready, false if not ready, or nullopt if chunk is missing.
     */
    [[nodiscard]] std::optional<bool> IsChunkReady(const ChunkCoordinates& coords) const {
        return CheckCache(coords)
            .transform([](const std::shared_ptr<ChunkData>& chunk) {
                return chunk->isReady;
            });
    }

    /**
     * @brief Retrieves the chunk readiness state safely, defaulting to false if missing.
     * 
     * Uses std::optional::value_or.
     * 
     * @param coords The coordinates of the chunk.
     * @return bool True if ready, false otherwise.
     */
    [[nodiscard]] bool IsChunkReadySafe(const ChunkCoordinates& coords) const {
        return IsChunkReady(coords).value_or(false);
    }

    /**
     * @brief Clears all chunks from the cache.
     */
    void Clear() {
        cache.clear();
        EterBase::ModernLogger::Info("MapChunkStreamingCache cleared.");
    }

private:
    /**
     * @brief Internal helper to check if a chunk exists in the cache.
     * 
     * @param coords The chunk coordinates.
     * @return std::optional with the chunk data pointer if it exists in the cache.
     */
    [[nodiscard]] std::optional<std::shared_ptr<ChunkData>> CheckCache(const ChunkCoordinates& coords) const {
        auto it = cache.find(coords);
        if (it != cache.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::unordered_map<ChunkCoordinates, std::shared_ptr<ChunkData>> cache;
};

} // namespace GameLib
