#pragma once

#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

namespace UserInterface::Core {
    /**
     * @brief Event triggered to request a refresh of the safebox money UI.
     */
    struct SafeBoxMoneyRefreshEvent : public IEvent {
        int32_t money;

        /**
         * @brief Constructs the safebox money refresh event.
         * @param money The current amount of money in the safebox.
         */
        explicit SafeBoxMoneyRefreshEvent(int32_t money) : money(money) {}
    };
}

namespace Network::Handlers {

	/**
	 * @brief Handles the incoming packet for setting the safebox money.
	 * 
	 * @param buffer The binary buffer containing the TPacketGCSafeboxMoneyChange packet.
	 * @return A PacketResult indicating success or an error code.
	 */
	EterBase::PacketResult<void> HandleSafeBoxMoney(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
