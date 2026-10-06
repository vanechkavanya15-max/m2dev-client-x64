#include "../../StdAfx.h"
#include "WarpHandler.h"
#include <cstring>

namespace Network
{
namespace Handlers
{

WarpHandler::WarpHandler()
    : state_{0, 0, 0, 0, false}, observer_(nullptr)
{
}

bool WarpHandler::HandlePacket(std::span<const uint8_t> buffer)
{
    state_.isValid = false;

    // Verify buffer size against the expected packet structure.
    if (buffer.size() < sizeof(WarpPacket))
    {
        return false;
    }

    WarpPacket packet{};
    // Safely copy the span buffer into the strictly aligned memory layout
    std::memcpy(&packet, buffer.data(), sizeof(WarpPacket));

    // Update the internal C++ memory state decoupled from any logic
    state_.targetX = packet.x;
    state_.targetY = packet.y;
    state_.serverAddress = packet.ipAddress;
    state_.serverPort = packet.port;
    state_.isValid = true;

    // Notify any registered observer asynchronously or via callback
    if (observer_ != nullptr)
    {
        observer_->OnWarpRequested(state_);
    }

    return true;
}

void WarpHandler::SetObserver(IWarpObserver* observer)
{
    observer_ = observer;
}

const WarpState& WarpHandler::GetState() const
{
    return state_;
}

} // namespace Handlers
} // namespace Network
