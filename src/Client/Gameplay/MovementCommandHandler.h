#pragma once

#include "../Core/DomainCommands.h"
#include "../Core/WorldContext.h"
#include "../Core/Result.h"
#include <functional>

namespace Client::Gameplay {

class MovementCommandHandler {
public:
    MovementCommandHandler(Client::Core::WorldContext& context, std::function<bool()> isStunnedProvider)
        : m_context{context}, m_isStunnedProvider{std::move(isStunnedProvider)} {}

    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> Handle(const Client::Core::MoveCommand& cmd);

private:
    Client::Core::WorldContext& m_context;
    std::function<bool()> m_isStunnedProvider;
};

} // namespace Client::Gameplay
