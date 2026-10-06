#include "../StdAfx.h"
#include "IMapChunkStreamingService.h"
#include "TerrainEvents.h"
#include "../../EterBase/LzoModern.h"
#include "../../EterBase/AsyncTaskRunnerLite.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <unordered_set>
#include <mutex>
#include <vector>
#include <functional>
#include <expected>
#include <cstring>

namespace GameLib::Terrain
{
    struct ChunkCoordinateHash
    {
        std::size_t operator()(const ChunkCoordinate& coord) const noexcept
        {
            return std::hash<int32_t>()(coord.sectorX) ^ (std::hash<int32_t>()(coord.sectorY) << 1);
        }
    };

    struct ChunkCoordinateEqual
    {
        bool operator()(const ChunkCoordinate& lhs, const ChunkCoordinate& rhs) const noexcept
        {
            return lhs.sectorX == rhs.sectorX && lhs.sectorY == rhs.sectorY;
        }
    };

    class MapChunkStreamingService_Decompress final : public IMapChunkStreamingService
    {
    public:
        MapChunkStreamingService_Decompress()
            : m_taskRunner(4)
        {
            EterBase::ModernLogger::Info("MapChunkStreamingService_Decompress initialized.");
        }

        ~MapChunkStreamingService_Decompress() override
        {
            ClearAllChunks();
        }

        void RequestChunk(ChunkCoordinate coord) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_loadingChunks.contains(coord) || m_loadedChunks.contains(coord))
            {
                return;
            }

            m_loadingChunks.insert(coord);

            std::vector<uint8_t> compressedHeightData;
            std::vector<uint8_t> compressedSplatData;
            
            m_taskRunner.SubmitTask(std::nullopt, [this, coord, hData = std::move(compressedHeightData), sData = std::move(compressedSplatData)]() -> EterBase::VoidResult<> {
                return DecompressChunkAsync(coord, hData, sData);
            });
        }

        bool IsChunkLoaded(ChunkCoordinate coord) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_loadedChunks.contains(coord);
        }

        void UpdateStreaming(float playerX, float playerY) override
        {
            (void)playerX;
            (void)playerY;
        }

        void EvictDistantChunks(float playerX, float playerY, float maxRadius) override
        {
            (void)playerX;
            (void)playerY;
            (void)maxRadius;
        }

        void ClearAllChunks() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_loadedChunks.clear();
            m_loadingChunks.clear();
        }

    private:
        EterBase::VoidResult<> DecompressChunkAsync(ChunkCoordinate coord, const std::vector<uint8_t>& compressedHeight, const std::vector<uint8_t>& compressedSplat)
        {
            constexpr size_t HEIGHTMAP_RAW_XSIZE = 131; // 128 + 3
            constexpr size_t HEIGHTMAP_RAW_YSIZE = 131;
            constexpr size_t SPLATMAP_RAW_SIZE = 258 * 258; // 256 + 2 (example)

            std::vector<uint16_t> uncompressedHeight(HEIGHTMAP_RAW_XSIZE * HEIGHTMAP_RAW_YSIZE, 0);
            std::vector<uint8_t> uncompressedSplat(SPLATMAP_RAW_SIZE, 0);
            size_t writtenHeight = 0;
            size_t writtenSplat = 0;

            if (!compressedHeight.empty())
            {
                auto heightResult = EterBase::LzoDecompressor::DecompressSafe(
                    std::span<const uint8_t>(compressedHeight),
                    std::span<uint8_t>(reinterpret_cast<uint8_t*>(uncompressedHeight.data()), uncompressedHeight.size() * sizeof(uint16_t)),
                    writtenHeight
                );

                if (heightResult != EterBase::LzoDecompressor::ErrorCode::Success)
                {
                     EterBase::ModernLogger::Error("Failed to decompress heightmap chunk ({}, {}).", coord.sectorX, coord.sectorY);
                     std::lock_guard<std::mutex> lock(m_mutex);
                     m_loadingChunks.erase(coord);
                     return std::unexpected("Heightmap decompression failed");
                }
            }

            if (!compressedSplat.empty())
            {
                auto splatResult = EterBase::LzoDecompressor::DecompressSafe(
                    std::span<const uint8_t>(compressedSplat),
                    std::span<uint8_t>(uncompressedSplat),
                    writtenSplat
                );

                if (splatResult != EterBase::LzoDecompressor::ErrorCode::Success)
                {
                     EterBase::ModernLogger::Error("Failed to decompress splatmap chunk ({}, {}).", coord.sectorX, coord.sectorY);
                     std::lock_guard<std::mutex> lock(m_mutex);
                     m_loadingChunks.erase(coord);
                     return std::unexpected("Splatmap decompression failed");
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_loadingChunks.contains(coord))
                {
                    // Chunk was evicted or clear was called during decompression
                    return std::unexpected("Chunk evicted during loading");
                }
                
                m_loadingChunks.erase(coord);
                m_loadedChunks.insert(coord);
            }

            EterBase::ModernLogger::Debug("Chunk ({}, {}) decompressed successfully.", coord.sectorX, coord.sectorY);

            Events::MapChunkLoadedEvent event(coord, std::move(uncompressedHeight), std::move(uncompressedSplat));
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {}; 
        }

        EterBase::AsyncTaskRunnerLite m_taskRunner;
        
        mutable std::mutex m_mutex;
        std::unordered_set<ChunkCoordinate, ChunkCoordinateHash, ChunkCoordinateEqual> m_loadedChunks;
        std::unordered_set<ChunkCoordinate, ChunkCoordinateHash, ChunkCoordinateEqual> m_loadingChunks;
    };

    std::unique_ptr<IMapChunkStreamingService> CreateChunkDecompressor()
    {
        return std::make_unique<MapChunkStreamingService_Decompress>();
    }
} // namespace GameLib::Terrain
