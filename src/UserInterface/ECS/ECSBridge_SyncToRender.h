#pragma once

#include "TransformComponentTable.h"
#include "EterBase/Result.h"

namespace UserInterface::ECS
{
    EterBase::PacketResult<void> SyncTransformToRender(const TransformComponentTable& transformTable);
}
