#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <cstring>
#include <span>

namespace ItemNetDispatch {

    /**
     * @brief Event triggered when an item drop appears on the ground.
     * 
     * This event is published to the EventBus to notify other subsystems (e.g., GUI)
     * that a new item drop has occurred, maintaining strict UI decoupling.
     */
    struct ItemGroundAddEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId dropVid;
        EterBase::ItemVnum itemVnum;
        int32_t x;
        int32_t y;
        int32_t z;

        /**
         * @brief Constructs a new ItemGroundAddEvent.
         * @param dropVid The unique entity ID of the item drop.
         * @param itemVnum The virtual number of the item.
         * @param x The global X coordinate.
         * @param y The global Y coordinate.
         * @param z The global Z coordinate.
         */
        ItemGroundAddEvent(EterBase::EntityId dropVid, EterBase::ItemVnum itemVnum, int32_t x, int32_t y, int32_t z)
            : dropVid(dropVid), itemVnum(itemVnum), x(x), y(y), z(z) {}
    };

    /**
     * @brief Dispatcher class for the TPacketGCItemGroundAdd network packet.
     * 
     * Handles the item ground add network packet using modern C++23 features
     * and strictly adheres to the Single Responsibility Principle.
     */
    class ItemDispatcher_GroundAdd
    {
    public:
        /**
         * @brief Processes the incoming ground add item packet buffer.
         * 
         * @param buffer The binary span containing the network packet data.
         * @return EterBase::PacketResult<void> representing success or domain-specific error.
         */
        static EterBase::PacketResult<void> Dispatch(std::span<const uint8_t> buffer)
        {
            // Step 1: Validate buffer size using C++23 monadic operations
            auto validateSize = [](std::span<const uint8_t> buf) -> EterBase::PacketResult<std::span<const uint8_t>> {
                if (buf.size() < sizeof(TPacketGCItemGroundAdd)) {
                    EterBase::ModernLogger::Error("ItemDispatcher_GroundAdd: Buffer underflow (size: {}, expected: {})",
                                                  buf.size(), sizeof(TPacketGCItemGroundAdd));
                    return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
                }
                return buf;
            };

            // Step 2: Extract and parse packet
            auto parsePacket = [](std::span<const uint8_t> buf) -> TPacketGCItemGroundAdd {
                TPacketGCItemGroundAdd packet;
                std::memcpy(&packet, buf.data(), sizeof(TPacketGCItemGroundAdd));
                return packet;
            };

            // Step 3: Dispatch event via EventBus
            auto dispatchEvent = [](const TPacketGCItemGroundAdd& packet) -> void {
                EterBase::EntityId dropVid(packet.dwVID);
                EterBase::ItemVnum itemVnum(packet.dwVnum);
                
                int32_t x = packet.lX;
                // Metin2 legacy coordinate fix: scale Y coordinate by 100 if greater than 10
                int32_t y = (packet.lY > 10) ? (packet.lY * 100) : packet.lY;
                int32_t z = packet.lZ;

                ItemGroundAddEvent event(dropVid, itemVnum, x, y, z);
                UserInterface::Core::EventBus::GetInstance().Publish(event);

                EterBase::ModernLogger::Debug("ItemDispatcher_GroundAdd: Dispatched drop event (VID: {}, Vnum: {})",
                                              packet.dwVID, packet.dwVnum);
            };

            // Execute monadic chain
            return validateSize(buffer)
                .transform(parsePacket)
                .transform(dispatchEvent);
        }
    };

} // namespace ItemNetDispatch
