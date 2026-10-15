#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <format>
#include "../../EterBase/Result.h"

namespace Client::Platform {

enum class VFSError : uint8_t {
    None = 0,
    FileNotFound,
    AccessDenied,
    CorruptedData,
    CrcMismatch,
    PackIndexInvalid
};

[[nodiscard]] constexpr std::string_view ToString(VFSError err) noexcept {
    switch (err) {
        case VFSError::None: return "None";
        case VFSError::FileNotFound: return "FileNotFound";
        case VFSError::AccessDenied: return "AccessDenied";
        case VFSError::CorruptedData: return "CorruptedData";
        case VFSError::CrcMismatch: return "CrcMismatch";
        case VFSError::PackIndexInvalid: return "PackIndexInvalid";
    }
    return "UnknownVFSError";
}

struct VFSFileInfo {
    std::string virtualPath;
    uint32_t size;
    uint32_t crc32;
};

class VFSManager {
public:
    VFSManager();
    ~VFSManager();

    VFSManager(const VFSManager&) = delete;
    VFSManager& operator=(const VFSManager&) = delete;

    static VFSManager& Instance();

    void RegisterLooseFileDirectory(std::string_view physicalPath);
    EterBase::Result<void, VFSError> MountPack(std::string_view packPath);

    EterBase::Result<std::vector<uint8_t>, VFSError> ReadFile(std::string_view virtualPath);
    
    // Thread-safe access to cached buffers
    EterBase::Result<std::span<const uint8_t>, VFSError> GetCachedFile(std::string_view virtualPath);
    
    bool Exists(std::string_view virtualPath) const;
    void ClearCache();

private:
    uint32_t CalculateCrc32(std::span<const uint8_t> data) const;

    struct PackEntry {
        std::string packPath;
        uint32_t offset;
        uint32_t size;
        uint32_t crc32;
    };

    struct CachedFile {
        std::vector<uint8_t> data;
    };

    std::vector<std::string> m_looseDirs;
    std::unordered_map<std::string, PackEntry> m_packEntries;
    
    mutable std::shared_mutex m_cacheMutex;
    std::unordered_map<std::string, std::shared_ptr<CachedFile>> m_cache;
    
    mutable std::shared_mutex m_packMutex;
    mutable std::shared_mutex m_looseMutex;
};

} // namespace Client::Platform

template <>
struct std::formatter<Client::Platform::VFSError> : std::formatter<std::string_view> {
    auto format(Client::Platform::VFSError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(Client::Platform::ToString(err), ctx);
    }
};
