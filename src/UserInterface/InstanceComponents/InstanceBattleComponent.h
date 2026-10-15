#pragma once

#include "InstanceCombatComponent.h"

namespace UserInterface::InstanceComponents {

/**
 * @brief Komponent odpowiedzialny za logike stanu walki, pojedynkow, celowania i rejestru PVP.
 * Fasada / alias kompozycyjny integrujacy podsystem bitewny instancji.
 */
using InstanceBattleComponent = InstanceCombatComponent;

} // namespace UserInterface::InstanceComponents
