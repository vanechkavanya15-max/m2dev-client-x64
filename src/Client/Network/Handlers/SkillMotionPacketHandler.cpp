#include "SkillMotionPacketHandler.h"
#include "EterBase/LogModern.h"

namespace Client::Network::Handlers {

EterBase::PacketResult<void> SkillMotionPacketHandler::HandlePacket(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(TPacketGCMotion))
    {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Error, 
            "SkillMotionPacketHandler: Buffer underflow. Expected {}, got {}", 
            sizeof(TPacketGCMotion), buffer.size());
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCMotion*>(buffer.data());

    EterBase::EntityId vid(packet->vid);
    EterBase::EntityId victimVid(packet->victim_vid);
    uint16_t motionId = packet->motion;

    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, 
        "SkillMotionPacketHandler: Odebrano animacje umiejetnosci. Aktor: {}, Cel: {}, Ruch: {}", 
        vid.value(), victimVid.value(), motionId);

    // Publikacja zdarzenia przez EventBus dla systemow zaleznych (np. UI, swiat)
    Client::Core::EventBus::GetInstance().Publish(SkillMotionReceivedEvent{vid, victimVid, motionId});

    return {};
}

} // namespace Client::Network::Handlers
