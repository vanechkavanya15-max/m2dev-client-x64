#pragma once

#include "../../EterBase/Result.h"
#include <memory>
#include <cstdint>
#include <string_view>

namespace Client::Network {

/**
 * @brief Interface for stream cryptography processing.
 */
class ICryptoProvider {
public:
    virtual ~ICryptoProvider() = default;

    /**
     * @brief Processes (encrypts/decrypts) a data stream in-place.
     * @param data Pointer to the buffer.
     * @param length Number of bytes to process.
     * @return Success or error string.
     */
    virtual EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* data, size_t length) = 0;
};

/**
 * @brief Factory function for creating an AES-CTR cryptography provider.
 */
std::unique_ptr<ICryptoProvider> CreateAesCryptoProvider();

} // namespace Client::Network
