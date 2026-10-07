#include "VFSManager.h"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <array>
#include <mutex>

namespace Client::Platform {

// CRC32 table initialization
namespace {
    std::array<uint32_t, 256> GenerateCrc32Table() {
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1)
                    c = 0xEDB88320 ^ (c >> 1);
                else
                    c >>= 1;
            }
            table[i] = c;
        }
        return table;
    }

    const std::array<uint32_t, 256> crc32Table = GenerateCrc32Table();
}

VFSManager::VFSManager() = default;
VFSManager::~VFSManager() = default;

VFSManager& VFSManager::Instance() {
    static VFSManager instance;
    return instance;
}

uint32_t VFSManager::CalculateCrc32(std::span<const uint8_t> data) const {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = crc32Table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

void VFSManager::RegisterLooseFileDirectory(std::string_view physicalPath) {
    std::unique_lock lock(m_looseMutex);
    m_looseDirs.push_back(std::string(physicalPath));
}

EterBase::Result<void, VFSError> VFSManager::MountPack(std::string_view packPath) {
    std::unique_lock lock(m_packMutex);

    std::string indexPath = std::string(packPath) + ".idx";
    std::ifstream idxFile(indexPath, std::ios::binary);
    
    if (!idxFile.is_open()) {
        return EterBase::MakeError(VFSError::PackIndexInvalid);
    }

    // Dummy index format for testing:
    // uint32_t numEntries
    // For each entry:
    //   uint16_t pathLen
    //   char path[pathLen]
    //   uint32_t offset
    //   uint32_t size
    //   uint32_t crc32
    uint32_t numEntries = 0;
    if (!idxFile.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries))) {
         return EterBase::MakeError(VFSError::PackIndexInvalid);
    }

    for (uint32_t i = 0; i < numEntries; ++i) {
        uint16_t pathLen = 0;
        if (!idxFile.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) {
            return EterBase::MakeError(VFSError::PackIndexInvalid);
        }

        std::string virtualPath(pathLen, '\0');
        if (!idxFile.read(virtualPath.data(), pathLen)) {
            return EterBase::MakeError(VFSError::PackIndexInvalid);
        }

        PackEntry entry;
        entry.packPath = std::string(packPath);
        
        if (!idxFile.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset))) {
            return EterBase::MakeError(VFSError::PackIndexInvalid);
        }
        
        if (!idxFile.read(reinterpret_cast<char*>(&entry.size), sizeof(entry.size))) {
             return EterBase::MakeError(VFSError::PackIndexInvalid);
        }
        
        if (!idxFile.read(reinterpret_cast<char*>(&entry.crc32), sizeof(entry.crc32))) {
             return EterBase::MakeError(VFSError::PackIndexInvalid);
        }

        m_packEntries[virtualPath] = entry;
    }
    
    return {};
}

EterBase::Result<std::vector<uint8_t>, VFSError> VFSManager::ReadFile(std::string_view virtualPath) {
    std::string vPathStr(virtualPath);

    // 1. Try loose files first
    {
        std::shared_lock lock(m_looseMutex);
        for (const auto& dir : m_looseDirs) {
            std::filesystem::path physicalPath = std::filesystem::path(dir) / virtualPath;
            if (std::filesystem::exists(physicalPath) && std::filesystem::is_regular_file(physicalPath)) {
                std::ifstream file(physicalPath, std::ios::binary | std::ios::ate);
                if (file.is_open()) {
                    std::streamsize size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    
                    std::vector<uint8_t> buffer(size);
                    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                        return buffer;
                    }
                }
            }
        }
    }

    // 2. Try packs
    {
        std::shared_lock lock(m_packMutex);
        auto it = m_packEntries.find(vPathStr);
        if (it != m_packEntries.end()) {
            const PackEntry& entry = it->second;
            std::ifstream packFile(entry.packPath, std::ios::binary);
            if (!packFile.is_open()) {
                 return EterBase::MakeError(VFSError::AccessDenied);
            }
            
            packFile.seekg(entry.offset, std::ios::beg);
            
            std::vector<uint8_t> buffer(entry.size);
            if (!packFile.read(reinterpret_cast<char*>(buffer.data()), entry.size)) {
                return EterBase::MakeError(VFSError::CorruptedData);
            }

            uint32_t calcCrc = CalculateCrc32(buffer);
            if (calcCrc != entry.crc32) {
                return EterBase::MakeError(VFSError::CrcMismatch);
            }

            return buffer;
        }
    }

    return EterBase::MakeError(VFSError::FileNotFound);
}

EterBase::Result<std::span<const uint8_t>, VFSError> VFSManager::GetCachedFile(std::string_view virtualPath) {
    std::string vPathStr(virtualPath);

    // Try to find in cache first
    {
        std::shared_lock readLock(m_cacheMutex);
        auto it = m_cache.find(vPathStr);
        if (it != m_cache.end()) {
            return std::span<const uint8_t>(it->second->data);
        }
    }

    // Not in cache, load it
    auto readResult = ReadFile(virtualPath);
    if (!readResult) {
        return EterBase::MakeError(readResult.error());
    }

    // Write to cache
    std::unique_lock writeLock(m_cacheMutex);
    
    // Check again to avoid double insertion if another thread loaded it
    auto it = m_cache.find(vPathStr);
    if (it != m_cache.end()) {
        return std::span<const uint8_t>(it->second->data);
    }
    
    auto cachedFile = std::make_shared<CachedFile>();
    cachedFile->data = std::move(readResult.value());
    
    auto inserted = m_cache.insert({vPathStr, cachedFile});
    return std::span<const uint8_t>(inserted.first->second->data);
}

bool VFSManager::Exists(std::string_view virtualPath) const {
    std::string vPathStr(virtualPath);
    
    {
        std::shared_lock lock(m_looseMutex);
        for (const auto& dir : m_looseDirs) {
            std::filesystem::path physicalPath = std::filesystem::path(dir) / virtualPath;
            if (std::filesystem::exists(physicalPath) && std::filesystem::is_regular_file(physicalPath)) {
                return true;
            }
        }
    }
    
    {
        std::shared_lock lock(m_packMutex);
        if (m_packEntries.contains(vPathStr)) {
            return true;
        }
    }
    
    return false;
}

void VFSManager::ClearCache() {
    std::unique_lock lock(m_cacheMutex);
    m_cache.clear();
}

} // namespace Client::Platform
