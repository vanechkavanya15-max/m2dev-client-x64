#include "StdAfx.h"
#include "SyncPositionHandler.h"
#include <cstring>

namespace Network::Handlers {

SyncPositionHandler::SyncPositionHandler(OnSyncPositionCallback callback)
    : onSyncPosition(std::move(callback)) {}

bool SyncPositionHandler::HandlePacket(std::span<const uint8_t> payload) const {
    if (payload.size() < sizeof(SyncPositionPacket)) {
        return false;
    }

    const auto* packetHeader = reinterpret_cast<const SyncPositionPacket*>(payload.data());
    
    // Validate the length field matches the payload size
    if (packetHeader->length != payload.size()) {
        return false;
    }
    
    // Calculate how many elements are in this packet
    const size_t headerSize = sizeof(SyncPositionPacket);
    const size_t elementsDataSize = payload.size() - headerSize;
    
    // Check if the remaining size is a multiple of SyncPositionElement
    if (elementsDataSize % sizeof(SyncPositionElement) != 0) {
        return false;
    }
    
    const size_t elementCount = elementsDataSize / sizeof(SyncPositionElement);
    const auto* elements = reinterpret_cast<const SyncPositionElement*>(payload.data() + headerSize);
    
    for (size_t i = 0; i < elementCount; ++i) {
        const auto& element = elements[i];
        if (onSyncPosition) {
            onSyncPosition(element.targetId, element.x, element.y);
        }
    }
    
    return true;
}

} // namespace Network::Handlers
