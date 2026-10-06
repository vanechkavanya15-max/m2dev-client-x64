#pragma once

#include <cstdint>
#include <span>

bool HandleQuickSlotDelPacket(std::span<const uint8_t> packet);
