#pragma once

#include <cstdint>
#include <span>
#include <functional>
#include <expected>
#include <string_view>
#include <string>

#include "GameLib/ItemData.h"

class ItemManagerModernLoader
{
public:
    using ItemRegistrar = std::function<void(const CItemData::TItemTable&)>;

    static std::expected<void, std::string> LoadFromModernBlob(std::span<const uint8_t> buffer, ItemRegistrar itemRegistrar);
    static std::expected<void, std::string> LoadFromFile(std::string_view filename, ItemRegistrar itemRegistrar);
};
