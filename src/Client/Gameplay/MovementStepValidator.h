#pragma once
#include "EterBase/StdAfx.h"

#include "../../EterBase/Result.h"
#include "../Core/DomainCommands.h"
#include "../Core/StrongTypes.h"

namespace Client::Gameplay {

class MovementStepValidator {
public:
    [[nodiscard]] static EterBase::Result<bool, Client::Core::CommandError> ValidateStep(
        const Client::Core::MapCoords& current, 
        const Client::Core::MapCoords& next, 
        float dt, 
        float speed) noexcept;
};

} // namespace Client::Gameplay
