#pragma once

#include <memory>
#include <cstdint>
#include "../Core/Result.h"
#include "../Core/DomainCommands.h"
#include "../Core/INetworkPort.h"

namespace Client::Gameplay {

class NpcInteractionController {
public:
    explicit NpcInteractionController(std::shared_ptr<Client::Core::INetworkPort> networkPort) noexcept;
    ~NpcInteractionController() = default;

    NpcInteractionController(const NpcInteractionController&) = delete;
    NpcInteractionController& operator=(const NpcInteractionController&) = delete;
    NpcInteractionController(NpcInteractionController&&) noexcept = default;
    NpcInteractionController& operator=(NpcInteractionController&&) noexcept = default;

    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> InteractWithNpc(const Client::Core::InteractNpcCommand& cmd) noexcept;

private:
    std::shared_ptr<Client::Core::INetworkPort> m_networkPort;
};

} // namespace Client::Gameplay
