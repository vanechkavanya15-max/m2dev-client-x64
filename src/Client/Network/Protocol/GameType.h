#pragma once

#include "ProtocolTypes.h"

// Compatibility constants for inventory and equipment slots
constexpr uint32_t c_Name_Max_Length = 64;
constexpr uint32_t c_FileName_Max_Length = 128;
constexpr uint32_t c_Short_Name_Max_Length = 32;

constexpr uint32_t c_Inventory_Page_Size = 5 * 9; // 45
constexpr uint32_t c_Inventory_Page_Count = 2;
constexpr uint32_t c_ItemSlot_Count = c_Inventory_Page_Size * c_Inventory_Page_Count; // 90
constexpr uint32_t c_Equipment_Count = 12;

constexpr uint32_t c_Equipment_Start = c_ItemSlot_Count;
