#pragma once

#include <EterBase/StrongTypes.h>
#include <EterBase/Result.h>
#include "Protocol/Protocol.h"

#include <unordered_map>
#include <chrono>
#include <optional>

namespace Client::Network {

struct PendingSpawnData {
    TPacketGCCharacterAdd spawnPacket;
    std::optional<TPacketGCCharacterAdditionalInfo> additionalInfo;
    std::chrono::steady_clock::time_point timestamp;
};

class IPendingSpawnRegistry {
public:
    virtual ~IPendingSpawnRegistry() = default;

    virtual EterBase::VoidResult<EterBase::EntityError> RegisterSpawn(EterBase::EntityId vid, const TPacketGCCharacterAdd& packet) = 0;
    virtual EterBase::VoidResult<EterBase::EntityError> RegisterAdditionalInfo(EterBase::EntityId vid, const TPacketGCCharacterAdditionalInfo& packet) = 0;
    virtual EterBase::Result<PendingSpawnData, EterBase::EntityError> TakeSpawn(EterBase::EntityId vid) = 0;
    virtual void Cleanup(std::chrono::milliseconds timeout_ms) = 0;
};

class PendingSpawnRegistry : public IPendingSpawnRegistry {
public:
    PendingSpawnRegistry() = default;
    ~PendingSpawnRegistry() override = default;

    EterBase::VoidResult<EterBase::EntityError> RegisterSpawn(EterBase::EntityId vid, const TPacketGCCharacterAdd& packet) override;
    EterBase::VoidResult<EterBase::EntityError> RegisterAdditionalInfo(EterBase::EntityId vid, const TPacketGCCharacterAdditionalInfo& packet) override;
    EterBase::Result<PendingSpawnData, EterBase::EntityError> TakeSpawn(EterBase::EntityId vid) override;
    void Cleanup(std::chrono::milliseconds timeout_ms) override;

private:
    std::unordered_map<EterBase::EntityId, PendingSpawnData> m_pendingSpawns;
};

} // namespace Client::Network
