#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../PacketDispatcher.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/MovementEvents.h"
#include "../../Core/EventBus.h"
#include <span>

namespace UserInterface::Network::Actors
{
    namespace
    {
        /**
         * @brief Handler for teleportation (warp) packets in the NetworkActors domain.
         * 
         * Parses the TPacketGCWarp packet, strictly decoupled from GUI logic.
         * Publishes a PlayerPositionUpdated event to the EventBus.
         * 
         * @param buffer Span over the incoming network packet.
         * @return EterBase::PacketResult<void> Success or packet error.
         */
        EterBase::PacketResult<void> ProcessActorWarp(std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(TPacketGCWarp))
            {
                EterBase::ModernLogger::Error("ProcessActorWarp: Buffer underflow. Expected {}, got {}", sizeof(TPacketGCWarp), buffer.size());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCWarp*>(buffer.data());

            EterBase::ModernLogger::Info("ProcessActorWarp: Warping actor to ({}, {}), Server IP: {}, Port: {}", 
                packet->lX, packet->lY, packet->lAddr, packet->wPort);

            // Publish a globally accessible event. EntityId(0) typically refers to the main character for warp requests.
            UserInterface::Core::EventBus::GetInstance().Publish(
                UserInterface::Core::Events::PlayerPositionUpdated(
                    EterBase::EntityId(0), packet->lX, packet->lY, packet->lX, packet->lY
                )
            );

            return {};
        }

        /**
         * @brief Automatic registration of the warp dispatcher for the actors subsystem.
         */
        struct ActorWarpDispatcherRegistration
        {
            ActorWarpDispatcherRegistration()
            {
                ::Network::PacketDispatcher::Instance().RegisterModernHandler(GC::WARP, ProcessActorWarp);
            }
        } g_actorWarpDispatcherRegistration;
    }
}
