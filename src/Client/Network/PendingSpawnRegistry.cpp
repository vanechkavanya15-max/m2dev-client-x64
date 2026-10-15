#include "PendingSpawnRegistry.h"
#include <EterBase/ModernLogger.h>

namespace Client::Network {

EterBase::VoidResult<EterBase::EntityError> PendingSpawnRegistry::RegisterSpawn(EterBase::EntityId vid, const TPacketGCCharacterAdd& packet) {
    if (m_pendingSpawns.contains(vid)) {
        return EterBase::MakeError(EterBase::EntityError::AlreadyExists);
    }

    PendingSpawnData data;
    data.spawnPacket = packet;
    data.timestamp = std::chrono::steady_clock::now();
    m_pendingSpawns.emplace(vid, std::move(data));

    return {};
}

EterBase::VoidResult<EterBase::EntityError> PendingSpawnRegistry::RegisterAdditionalInfo(EterBase::EntityId vid, const TPacketGCCharacterAdditionalInfo& packet) {
    auto it = m_pendingSpawns.find(vid);
    if (it == m_pendingSpawns.end()) {
        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    it->second.additionalInfo = packet;
    return {};
}

EterBase::Result<PendingSpawnData, EterBase::EntityError> PendingSpawnRegistry::TakeSpawn(EterBase::EntityId vid) {
    auto it = m_pendingSpawns.find(vid);
    if (it == m_pendingSpawns.end()) {
        return EterBase::MakeError(EterBase::EntityError::NotFound);
    }

    if (!it->second.additionalInfo.has_value()) {
        // According to context, spawn is incomplete without additional info.
        // Returning InvalidType to represent incomplete spawn.
        return EterBase::MakeError(EterBase::EntityError::InvalidType);
    }

    PendingSpawnData data = std::move(it->second);
    m_pendingSpawns.erase(it);
    return data;
}

void PendingSpawnRegistry::Cleanup(std::chrono::milliseconds timeout_ms) {
    auto now = std::chrono::steady_clock::now();
    
    // erase_if for map
    std::erase_if(m_pendingSpawns, [now, timeout_ms](const auto& pair) {
        return (now - pair.second.timestamp) > timeout_ms;
    });
}

} // namespace Client::Network
