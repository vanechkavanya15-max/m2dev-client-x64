#include "ItemManagerModernLoader.h"
#include "PackLib/PackManager.h"
#include "EterBase/ModernLogger.h"
#include <zstd.h>
#include <fstream>
#include <vector>
#include <format>

std::expected<void, std::string> ItemManagerModernLoader::LoadFromModernBlob(std::span<const uint8_t> buffer, ItemRegistrar itemRegistrar)
{
    if (buffer.empty())
    {
        return std::unexpected("Buffer is empty.");
    }

    unsigned long long const uncompressedSize = ZSTD_getFrameContentSize(buffer.data(), buffer.size());
    if (uncompressedSize == ZSTD_CONTENTSIZE_ERROR)
    {
        return std::unexpected("Not a valid ZSTD compressed frame.");
    }
    if (uncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        return std::unexpected("Original size unknown (unsupported format).");
    }

    if (uncompressedSize % sizeof(CItemData::TItemTable) != 0)
    {
        return std::unexpected(std::format("Uncompressed size ({}) is not a multiple of TItemTable size ({}).",
            uncompressedSize, sizeof(CItemData::TItemTable)));
    }

    std::vector<uint8_t> decompressedBuffer(uncompressedSize);
    size_t const decompressResult = ZSTD_decompress(decompressedBuffer.data(), decompressedBuffer.size(), buffer.data(), buffer.size());

    if (ZSTD_isError(decompressResult))
    {
        return std::unexpected(std::format("ZSTD decompression failed: {}", ZSTD_getErrorName(decompressResult)));
    }

    if (decompressResult != uncompressedSize)
    {
        return std::unexpected(std::format("Decompressed size mismatch (expected {}, got {})", uncompressedSize, decompressResult));
    }

    size_t const itemsCount = uncompressedSize / sizeof(CItemData::TItemTable);
    const CItemData::TItemTable* items = reinterpret_cast<const CItemData::TItemTable*>(decompressedBuffer.data());

    for (size_t i = 0; i < itemsCount; ++i)
    {
        itemRegistrar(items[i]);
    }

    return {};
}

std::expected<void, std::string> ItemManagerModernLoader::LoadFromFile(std::string_view filename, ItemRegistrar itemRegistrar)
{
    TPackFile fileData;
    if (CPackManager::Instance().GetFile(filename, fileData))
    {
        EterBase::Trace("ItemManagerModernLoader: Loaded '{}' from VFS.", filename);
        return LoadFromModernBlob(std::span<const uint8_t>(fileData.data(), fileData.size()), itemRegistrar);
    }
    
    // Fallback to standard file system
    std::ifstream file(std::string{filename}, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        auto err = std::format("Failed to open file '{}' from disk (not found in VFS).", filename);
        EterBase::TraceError("ItemManagerModernLoader: {}", err);
        return std::unexpected(err);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        auto err = std::format("Failed to read file '{}' from disk.", filename);
        EterBase::TraceError("ItemManagerModernLoader: {}", err);
        return std::unexpected(err);
    }

    EterBase::Trace("ItemManagerModernLoader: Loaded '{}' from disk.", filename);
    return LoadFromModernBlob(buffer, itemRegistrar);
}
