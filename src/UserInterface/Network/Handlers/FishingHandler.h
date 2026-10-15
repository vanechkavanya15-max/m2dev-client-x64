#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
    /**
     * @brief Event published when fishing succeeds (fish caught)
     */
    struct FishingSuccessEvent : public UserInterface::Core::IEvent {
        bool isFish;
        std::string fishName;
        
        /**
         * @brief Construct a new Fishing Success Event object
         * 
         * @param isFish Whether the caught item is a fish
         * @param fishName Name of the caught fish/item
         */
        FishingSuccessEvent(bool isFish, std::string fishName) : isFish(isFish), fishName(std::move(fishName)) {}
    };

    /**
     * @brief Event published when fishing fails (nothing caught)
     */
    struct FishingFailureEvent : public UserInterface::Core::IEvent {
        /**
         * @brief Construct a new Fishing Failure Event object
         */
        FishingFailureEvent() = default;
    };

    /**
     * @brief Event published to notify fishing result for other players
     */
    struct FishingNotifyEvent : public UserInterface::Core::IEvent {
        bool isFish;
        std::string fishName;
        
        /**
         * @brief Construct a new Fishing Notify Event object
         * 
         * @param isFish Whether the caught item is a fish
         * @param fishName Name of the caught fish/item
         */
        FishingNotifyEvent(bool isFish, std::string fishName) : isFish(isFish), fishName(std::move(fishName)) {}
    };

    /**
     * @brief Event published when fishing notification unknown
     */
    struct FishingNotifyUnknownEvent : public UserInterface::Core::IEvent {
        /**
         * @brief Construct a new Fishing Notify Unknown Event object
         */
        FishingNotifyUnknownEvent() = default;
    };

    /**
     * @brief Handler for fishing packets.
     * 
     * @param buffer Binary span representing the incoming network packet.
     * @return true if the packet was successfully parsed and handled; otherwise false.
     */
    bool HandleFishing(std::span<const uint8_t> buffer);
}
