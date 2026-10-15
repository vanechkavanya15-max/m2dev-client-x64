#pragma once

#include "../../EterBase/StdAfx.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/XXHash64Constexpr.h"

#include <string>
#include <array>
#include <cstdint>

namespace Client::Data {

struct ItemLimit {
    uint8_t limitType = 0;
    int64_t limitValue = 0;
    constexpr auto operator<=>(const ItemLimit&) const noexcept = default;
};

struct ItemApply {
    uint8_t applyType = 0;
    int64_t applyValue = 0;
    constexpr auto operator<=>(const ItemApply&) const noexcept = default;
};

struct ItemProtoRecord {
    EterBase::ItemVnum vnum{0};
    uint32_t vnumRange = 0;
    std::string name;
    std::string localeName;
    uint8_t type = 0;
    uint8_t subType = 0;
    uint8_t weight = 0;
    uint8_t size = 0;
    uint32_t antiFlags = 0;
    uint32_t flags = 0;
    uint32_t wearFlags = 0;
    uint32_t immuneFlags = 0;
    uint64_t buyPrice = 0;
    uint64_t sellPrice = 0;
    std::array<ItemLimit, 4> limits{};
    std::array<ItemApply, 5> applies{};
    std::array<int64_t, 6> values{};
    std::array<int64_t, 4> sockets{};
    EterBase::ItemVnum refinedVnum{0};
    uint32_t refineSet = 0;

    auto operator<=>(const ItemProtoRecord&) const noexcept = default;
    EterBase::VoidResult<std::string_view> Invariants() const;
};

struct MobProtoRecord {
    EterBase::EntityId vnum{0};
    std::string name;
    std::string localeName;
    uint8_t type = 0;
    uint8_t rank = 0;
    uint8_t battleType = 0;
    EterBase::PlayerLevel level{0};
    uint8_t size = 0;
    uint32_t goldMin = 0;
    uint32_t goldMax = 0;
    uint32_t exp = 0;
    uint32_t maxHp = 0;
    uint8_t regenCycle = 0;
    uint8_t regenPercent = 0;
    uint16_t defense = 0;
    
    auto operator<=>(const MobProtoRecord&) const noexcept = default;
    EterBase::VoidResult<std::string_view> Invariants() const;
};

class ProtoSchemaDefinition {
public:
    static uint64_t GetItemProtoSchemaHash() noexcept;
    static uint64_t GetMobProtoSchemaHash() noexcept;
};

} // namespace Client::Data
