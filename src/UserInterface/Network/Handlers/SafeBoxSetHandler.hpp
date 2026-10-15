/**
 * @file SafeBoxSetHandler.hpp
 * @brief Handler for the SafeBox Item Set network packet.
 */
#pragma once

#include <cstdint>
#include <span>

namespace Network::Handlers {

	/**
	 * @brief Handles the incoming packet for setting an item in the safebox.
	 * 
	 * @param buffer The binary buffer containing the TPacketGCItemSet packet.
	 * @return true if the packet was successfully parsed and applied, false otherwise.
	 */
	bool HandleSafeBoxSet(std::span<const uint8_t> buffer);

} // namespace Network::Handlers
