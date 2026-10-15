#include "StdAfx.h"
/**
 * @file SafeBoxMoneyHandler.cpp
 * @brief Implementation of the SafeBox Money Change network packet handler.
 */
#include "SafeBoxMoneyHandler.hpp"
#include "../../Packet.h"
#include "../../PythonSafeBox.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"

namespace Network::Handlers {

	/**
	 * @brief Handles the incoming packet for setting the safebox money.
	 * 
	 * @param buffer The binary buffer containing the TPacketGCSafeboxMoneyChange packet.
	 * @return A PacketResult indicating success or an error code.
	 */
	EterBase::PacketResult<void> HandleSafeBoxMoney(std::span<const uint8_t> buffer) {
		if (buffer.size() < sizeof(TPacketGCSafeboxMoneyChange)) {
			EterBase::ModernLogger::Error("HandleSafeBoxMoney: Buffer underflow. Expected {}, got {}", 
				sizeof(TPacketGCSafeboxMoneyChange), buffer.size());
			return std::unexpected(EterBase::PacketError::BufferUnderflow);
		}

		const auto* packet = reinterpret_cast<const TPacketGCSafeboxMoneyChange*>(buffer.data());

		// Update state in CPythonSafeBox
		CPythonSafeBox::Instance().SetMoney(packet->lMoney);

		// Disconnect from GUI, instead publish an event to let the Python bindings or UI handle the rendering update.
        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::SafeBoxMoneyRefreshEvent{packet->lMoney});

		return {};
	}

} // namespace Network::Handlers
