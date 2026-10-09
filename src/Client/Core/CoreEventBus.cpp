#include "CoreEventBus.h"

namespace Client::Core {

CoreEventBus& CoreEventBus::Instance() noexcept {
    static CoreEventBus instance;
    return instance;
}

void CoreEventBus::RegisterChannel(IEventChannel* channel) noexcept {
    std::lock_guard lock(busMutex_);
    if (channelCount_ < MaxChannels) {
        channels_[channelCount_++] = channel;
    }
}

void CoreEventBus::ClearAll() noexcept {
    std::lock_guard lock(busMutex_);
    for (size_t i = 0; i < channelCount_; ++i) {
        if (channels_[i]) {
            channels_[i]->Clear();
        }
    }
}

} // namespace Client::Core
