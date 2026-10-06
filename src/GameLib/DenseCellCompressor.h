#pragma once

#include <vector>
#include <cstdint>
#include <span>
#include <optional>
#include <format>
#include <string_view>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"

// Note: Ensure the actual EventBus header path matches the repository structure.
#include "../UserInterface/Core/EventBus.h"

namespace GameLib {

/**
 * @brief Event triggered when terrain collision data is compressed.
 */
struct TerrainCollisionCompressedEvent : public UserInterface::Core::IEvent {
    EterBase::MapIndex mapIndex;
    size_t originalSize;
    size_t compressedSize;

    TerrainCollisionCompressedEvent(EterBase::MapIndex mapIdx, size_t origSize, size_t compSize)
        : mapIndex(mapIdx), originalSize(origSize), compressedSize(compSize) {}
};

/**
 * @brief Event triggered when terrain collision data is decompressed.
 */
struct TerrainCollisionDecompressedEvent : public UserInterface::Core::IEvent {
    EterBase::MapIndex mapIndex;
    size_t compressedSize;
    size_t decompressedSize;

    TerrainCollisionDecompressedEvent(EterBase::MapIndex mapIdx, size_t compSize, size_t decompSize)
        : mapIndex(mapIdx), compressedSize(compSize), decompressedSize(decompSize) {}
};

/**
 * @brief Utility class for Run-Length Encoding (RLE) compression of terrain collision attribute data.
 * 
 * Uses modern C++23 features like std::expected (EterBase::Result), std::span, and std::optional.
 * Does not use output parameters or Hungarian notation. Updates state only in C++ memory
 * and emits events to decouple from the GUI.
 */
class DenseCellCompressor {
public:
    /**
     * @brief Error types specific to DenseCellCompressor operations.
     */
    enum class Error : uint8_t {
        None = 0,
        EmptyInput,
        BufferOverflow,
        InvalidData,
        DecompressionBomb
    };

    /**
     * @brief Converts an error code to a string view.
     * @param err The error code to convert.
     * @return std::string_view representing the error.
     */
    static constexpr std::string_view ToString(Error err) noexcept {
        switch (err) {
            case Error::None: return "None";
            case Error::EmptyInput: return "EmptyInput";
            case Error::BufferOverflow: return "BufferOverflow";
            case Error::InvalidData: return "InvalidData";
            case Error::DecompressionBomb: return "DecompressionBomb";
        }
        return "Unknown";
    }

    /**
     * @brief Result type alias for DenseCellCompressor.
     */
    template <typename T>
    using CompressorResult = EterBase::Result<T, Error>;

    /**
     * @brief Compresses raw terrain collision attribute data using RLE.
     * 
     * @param mapIndex The MapIndex associated with the data.
     * @param input Span of raw uncompressed bytes.
     * @return CompressorResult<std::vector<uint8_t>> Containing compressed data on success, or an Error.
     */
    static CompressorResult<std::vector<uint8_t>> Compress(EterBase::MapIndex mapIndex, std::span<const uint8_t> input) {
        auto valid_input_opt = ValidateInput(input);
        if (!valid_input_opt) {
            return std::unexpected(Error::EmptyInput);
        }

        auto result = valid_input_opt
            .and_then([&](std::span<const uint8_t> valid_input) -> std::optional<std::vector<uint8_t>> {
                std::vector<uint8_t> compressedData;
                // Pre-allocate for worst case scenario (size * 2)
                compressedData.reserve(valid_input.size() * 2);

                size_t n = valid_input.size();
                for (size_t i = 0; i < n; ) {
                    uint8_t count = 1;
                    while (i + count < n && valid_input[i] == valid_input[i + count] && count < 255) {
                        count++;
                    }
                    compressedData.push_back(count);
                    compressedData.push_back(valid_input[i]);
                    i += count;
                }
                return compressedData;
            })
            .transform([&](std::vector<uint8_t> data) {
                UserInterface::Core::EventBus::GetInstance().Publish(TerrainCollisionCompressedEvent(
                    mapIndex,
                    input.size(),
                    data.size()
                ));
                EterBase::ModernLogger::Debug("Compressed map {} from {} to {} bytes", 
                                             mapIndex.value(), input.size(), data.size());
                return data;
            });

        if (result) {
            return *result;
        }
        return std::unexpected(Error::InvalidData);
    }

    /**
     * @brief Decompresses RLE compressed terrain collision attribute data.
     * 
     * @param mapIndex The MapIndex associated with the data.
     * @param input Span of RLE compressed bytes.
     * @param expectedOutputSize The expected size of the decompressed data.
     * @return CompressorResult<std::vector<uint8_t>> Containing decompressed data on success, or an Error.
     */
    static CompressorResult<std::vector<uint8_t>> Decompress(EterBase::MapIndex mapIndex, std::span<const uint8_t> input, size_t expectedOutputSize) {
        auto valid_input_opt = ValidateInput(input);
        if (!valid_input_opt) {
            return std::unexpected(Error::EmptyInput);
        }

        if (valid_input_opt->size() % 2 != 0) {
            return std::unexpected(Error::InvalidData);
        }

        auto valid_input = *valid_input_opt;
        std::vector<uint8_t> decompressedData;
        decompressedData.reserve(expectedOutputSize);

        for (size_t i = 0; i < valid_input.size(); i += 2) {
            uint8_t count = valid_input[i];
            uint8_t value = valid_input[i + 1];
            
            // Defend against decompression bombs
            if (decompressedData.size() + count > expectedOutputSize) {
                return std::unexpected(Error::DecompressionBomb);
            }
            
            decompressedData.insert(decompressedData.end(), count, value);
        }

        if (decompressedData.size() != expectedOutputSize) {
            return std::unexpected(Error::InvalidData);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(TerrainCollisionDecompressedEvent(
            mapIndex,
            input.size(),
            decompressedData.size()
        ));
        EterBase::ModernLogger::Debug("Decompressed map {} from {} to {} bytes", 
                                     mapIndex.value(), input.size(), decompressedData.size());
        
        return decompressedData;
    }

private:
    /**
     * @brief Validates the input span using std::optional.
     * 
     * @param input The input span to validate.
     * @return std::optional<std::span<const uint8_t>> The valid span, or nullopt if empty.
     */
    static std::optional<std::span<const uint8_t>> ValidateInput(std::span<const uint8_t> input) {
        if (input.empty()) {
            return std::nullopt;
        }
        return input;
    }
};

} // namespace GameLib
