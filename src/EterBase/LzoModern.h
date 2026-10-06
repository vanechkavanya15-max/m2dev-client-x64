#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <lzo/lzo1x.h>

namespace EterBase {

/**
 * @brief LZO compression block header.
 * 
 * Defines the header structure for compressed data blocks used within the client.
 * Strictly packed to 1 byte to match the network/file binary layout.
 */
#pragma pack(push, 1)
struct LzoHeader {
    uint32_t magic;             ///< Magic number / FourCC identifier (usually 'MCOZ')
    uint32_t encrypt_size;      ///< Size of the data when encrypted (0 if not encrypted)
    uint32_t compressed_size;   ///< Size of the compressed data payload
    uint32_t real_size;         ///< Original, uncompressed data size
};
#pragma pack(pop)

/**
 * @brief Modern C++20 Hermetic LZO Decompressor
 * 
 * Provides memory-safe LZO decompression utilizing std::span for boundary checking.
 * Adheres strictly to the modernization guidelines, avoiding UI coupling and
 * preventing output buffer overflows.
 */
class LzoDecompressor {
public:
    /**
     * @brief The expected Magic / FourCC value for valid LZO blocks ('MCOZ').
     */
    static constexpr uint32_t LZO_MAGIC_FOURCC = 0x5A4F434D; // 'MCOZ' in little-endian

    /**
     * @brief Result codes for decompression operations.
     */
    enum class ErrorCode : uint8_t {
        Success = 0,
        InvalidInputSize,
        InvalidMagic,
        OutputBufferTooSmall,
        LzoError
    };

    /**
     * @brief Safely decompresses an LZO block into the provided output span.
     * 
     * Reads the `LzoHeader` from the start of the input span, validates the magic
     * number, and ensures the output span has enough capacity to hold the uncompressed data.
     * 
     * @param input  A span of bytes containing the LzoHeader followed by the compressed payload.
     * @param output A span of bytes where the uncompressed data will be written.
     * @param out_written The actual number of decompressed bytes written to the output span.
     * @return ErrorCode::Success if decompression was successful, otherwise an appropriate error code.
     */
    static ErrorCode DecompressSafe(
        std::span<const uint8_t> input,
        std::span<uint8_t> output,
        size_t& out_written) noexcept 
    {
        out_written = 0;

        if (input.size() < sizeof(LzoHeader)) {
            return ErrorCode::InvalidInputSize;
        }

        const LzoHeader* header = reinterpret_cast<const LzoHeader*>(input.data());

        if (header->magic != LZO_MAGIC_FOURCC) {
            return ErrorCode::InvalidMagic;
        }

        if (output.size() < header->real_size) {
            return ErrorCode::OutputBufferTooSmall;
        }

        // Note: The Metin2 LZO payload format contains the 4-byte FourCC twice:
        // once inside the LzoHeader, and once immediately preceding the compressed
        // data. Therefore, the actual compressed data begins at an offset of
        // sizeof(LzoHeader) + sizeof(uint32_t).
        const size_t header_offset = sizeof(LzoHeader) + sizeof(uint32_t);
        if (input.size() < header_offset + header->compressed_size) {
            return ErrorCode::InvalidInputSize;
        }

        const uint8_t* compressed_data = input.data() + header_offset;
        
        lzo_uint uncompressed_len = static_cast<lzo_uint>(output.size());

        int result = lzo1x_decompress_safe(
            compressed_data,
            static_cast<lzo_uint>(header->compressed_size),
            output.data(),
            &uncompressed_len,
            nullptr
        );

        if (result != LZO_E_OK) {
            return ErrorCode::LzoError;
        }

        if (uncompressed_len != header->real_size) {
            return ErrorCode::LzoError;
        }

        out_written = uncompressed_len;
        return ErrorCode::Success;
    }

    /**
     * @brief Safely decompresses a raw payload (without header) into the provided output span.
     * 
     * Useful if the header is parsed separately and the input span only contains the
     * raw LZO-compressed bytes.
     * 
     * @param payload A span containing only the LZO-compressed data.
     * @param output A span of bytes where the uncompressed data will be written.
     * @param out_written The actual number of decompressed bytes written to the output span.
     * @return ErrorCode::Success if decompression was successful, otherwise an appropriate error code.
     */
    static ErrorCode DecompressRawSafe(
        std::span<const uint8_t> payload,
        std::span<uint8_t> output,
        size_t& out_written) noexcept 
    {
        out_written = 0;
        lzo_uint uncompressed_len = static_cast<lzo_uint>(output.size());

        int result = lzo1x_decompress_safe(
            payload.data(),
            static_cast<lzo_uint>(payload.size()),
            output.data(),
            &uncompressed_len,
            nullptr
        );

        if (result != LZO_E_OK) {
            return ErrorCode::LzoError;
        }

        out_written = uncompressed_len;
        return ErrorCode::Success;
    }
};

} // namespace EterBase
