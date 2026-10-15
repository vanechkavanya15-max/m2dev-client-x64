#include "../../StdAfx.h"
#include "../PacketDispatcher.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <span>

namespace Network::Dispatchers
{
    /**
     * @brief Event published when a warp request is successfully processed.
     */
    struct WarpEvent : public UserInterface::Core::IEvent
    {
        int32_t x;
        int32_t y;
        int32_t ipAddress;
        uint16_t port;
        
        WarpEvent(int32_t x, int32_t y, int32_t ipAddress, uint16_t port)
            : x(x), y(y), ipAddress(ipAddress), port(port) {}
    };

    /**
     * @brief Handler for teleportation (warp) packets.
     * 
     * Parses the TPacketGCWarp packet, strictly decoupled from GUI logic.
     * Publishes a WarpEvent to the EventBus.
     * 
     * @param buffer Span over the incoming network packet.
     * @return EterBase::PacketResult<void> Success or packet error.
     */
    EterBase::PacketResult<void> ProcessWarp(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCWarp))
        {
            EterBase::ModernLogger::Error("ProcessWarp: Buffer underflow. Expected {}, got {}", sizeof(TPacketGCWarp), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCWarp*>(buffer.data());

        EterBase::ModernLogger::Info("ProcessWarp: Warping to ({}, {}), IP: {}, Port: {}", packet->lX, packet->lY, packet->lAddr, packet->wPort);

        UserInterface::Core::EventBus::GetInstance().Publish(
            WarpEvent(packet->lX, packet->lY, packet->lAddr, packet->wPort)
        );

        return {};
    }

    /**
     * @brief Automatic registration of the warp dispatcher.
     */
    struct WarpDispatcherRegistration
    {
        WarpDispatcherRegistration()
        {
            PacketDispatcher::Instance().RegisterModernHandler(GC::WARP, ProcessWarp);
        }
    } g_warpDispatcherRegistration;
}
