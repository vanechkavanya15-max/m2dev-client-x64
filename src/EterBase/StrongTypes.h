#pragma once

#include <cstdint>
#include <compare>
#include <functional>
#include <format>

/**
 * @file StrongTypes.h
 * @brief Silne typy domenowe C++23 zapobiegajace pomylkom identyfikatorow przez programistow i agentow AI.
 * 
 * Klasa szablonowa StrongType enkapsuluje typy prymitywne (np. uint32_t) za pomoca unikalnych tagow.
 * Zapobiega to przypadkowemu przekazaniu np. ItemVnum do funkcji oczekujacej EntityId.
 */

namespace EterBase {

template <typename Tag, typename Underlying = uint32_t, Underlying DefaultValue = Underlying{}>
class StrongType {
public:
    using UnderlyingType = Underlying;

    constexpr StrongType() noexcept : m_value(DefaultValue) {}
    constexpr explicit StrongType(Underlying value) noexcept : m_value(value) {}

    [[nodiscard]] constexpr Underlying value() const noexcept { return m_value; }
    [[nodiscard]] constexpr Underlying get() const noexcept { return m_value; }
    [[nodiscard]] constexpr Underlying Value() const noexcept { return m_value; }
    [[nodiscard]] constexpr Underlying Get() const noexcept { return m_value; }

    [[nodiscard]] constexpr explicit operator Underlying() const noexcept { return m_value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_value != DefaultValue; }

    constexpr auto operator<=>(const StrongType&) const noexcept = default;
    constexpr bool operator==(const StrongType&) const noexcept = default;

    constexpr StrongType& operator=(Underlying val) noexcept {
        m_value = val;
        return *this;
    }

private:
    Underlying m_value;
};

// ============================================================================
// Tagowane Typy Domenowe Metin2
// ============================================================================

struct EntityIdTag {};
struct ItemVnumTag {};
struct ItemSlotTag {};
struct SkillIdTag {};
struct GuildIdTag {};
struct DungeonIdTag {};
struct MapIndexTag {};
struct PlayerLevelTag {};

/// @brief Identyfikator unikalny instancji w swiecie gry (VID aktora, moba, gracza)
using EntityId = StrongType<EntityIdTag, uint32_t, 0>;

/// @brief Wirtualny numer typu przedmiotu (VNUM w tabeli item_proto)
using ItemVnum = StrongType<ItemVnumTag, uint32_t, 0>;

/// @brief Pozycja slotu w ekwipunku, pasie, depozycie (0..N)
using ItemSlot = StrongType<ItemSlotTag, uint16_t, 0xFFFF>;

/// @brief Identyfikator umiejetnosci (Skill VNUM)
using SkillId = StrongType<SkillIdTag, uint32_t, 0>;

/// @brief Identyfikator gildii
using GuildId = StrongType<GuildIdTag, uint32_t, 0>;

/// @brief Identyfikator instancji lochu (Dungeon ID)
using DungeonId = StrongType<DungeonIdTag, uint32_t, 0>;

/// @brief Indeks mapy swiata
using MapIndex = StrongType<MapIndexTag, int32_t, 0>;

/// @brief Poziom postaci gracza (1..120)
using PlayerLevel = StrongType<PlayerLevelTag, uint8_t, 0>;

} // namespace EterBase

// ============================================================================
// Specjalizacje std::hash dla wykorzystania w std::unordered_map
// ============================================================================

template <typename Tag, typename Underlying, Underlying DefaultValue>
struct std::hash<EterBase::StrongType<Tag, Underlying, DefaultValue>> {
    std::size_t operator()(const EterBase::StrongType<Tag, Underlying, DefaultValue>& st) const noexcept {
        return std::hash<Underlying>{}(st.value());
    }
};

// ============================================================================
// Specjalizacje std::formatter dla bezpiecznego formatowania C++23 (std::format)
// ============================================================================

template <typename Tag, typename Underlying, Underlying DefaultValue>
struct std::formatter<EterBase::StrongType<Tag, Underlying, DefaultValue>> : std::formatter<Underlying> {
    auto format(const EterBase::StrongType<Tag, Underlying, DefaultValue>& st, std::format_context& ctx) const {
        return std::formatter<Underlying>::format(st.value(), ctx);
    }
};
