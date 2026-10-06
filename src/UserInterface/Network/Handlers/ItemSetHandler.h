#pragma once

#include <cstdint>
#include <span>

class CPythonNetworkStream;

namespace Network
{
namespace Handlers
{

/**
 * @brief Handles the item set packet (TPacketGCItemSet) from the server.
 * @param stream The network stream instance to handle receiving data and triggering updates.
 * @return true if the packet was successfully parsed and handled, false otherwise.
 */
bool HandleItemSet(CPythonNetworkStream& stream);

} // namespace Handlers
} // namespace Network
