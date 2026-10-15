#pragma once

#include <string>

namespace Client::Core {
    struct WorldContext;
}

namespace Client::IPC {

class IPCQueryHandler {
public:
    static std::string HandleQueryPlayerState(Client::Core::WorldContext& ctx);
    static std::string HandleQueryInventory(Client::Core::WorldContext& ctx);
    static std::string HandleQuerySurroundings(Client::Core::WorldContext& ctx, float radius);
};

} // namespace Client::IPC
