#pragma once

#include <cstdint>
#include <algorithm>
#include <span>
#include <string_view>
#include <stdexcept>

namespace domain
{

    /**
     * @brief Represents the various affect flags that can be applied to an actor.
     * 
     * This enum uses modern C++20 `uint32_t` type, replacing the legacy Win32 `DWORD` 
     * and old `AFFECT_` constants with a strongly-typed enum class.
     */
    enum class AffectFlagType : uint32_t
    {
        None = 0,
        Invisibility = 1,
        Spawn = 2,
        Poison = 3,
        Slow = 4,
        Stun = 5,
        DungeonReady = 6,
        ShowAlways = 7,
        BuildingConstructionSmall = 8,
        BuildingConstructionLarge = 9,
        BuildingUpgrade = 10,
        MovSpeedPotion = 11,
        AttSpeedPotion = 12,
        FishMind = 13,
        Jeongwi = 14,
        Geomgyeong = 15,
        Cheongeun = 16,
        Gyeonggong = 17,
        Eunhyeong = 18,
        Gwigeom = 19,
        Gongpo = 20,
        Jumagap = 21,
        Hosin = 22,
        Boho = 23,
        Kwaesok = 24,
        Heuksin = 25,
        Muyeong = 26,
        ReviveInvisibility = 27,
        Fire = 28,
        Gicheon = 29,
        Jeungryeok = 30,
        Dash = 31,
        Pabeop = 32,
        FallenCheongeun = 33,
        Polymorph = 34,
        WarFlag1 = 35,
        WarFlag2 = 36,
        WarFlag3 = 37,
        ChinaFirework = 38,
        PremiumSilver = 39,
        PremiumGold = 40,
        RamadanRing = 41
    };

    /**
     * @brief Network structure for receiving affect flags.
     * 
     * Strict 1-byte alignment is enforced to safely overlay over network buffers.
     */
#pragma pack(push, 1)
    struct AffectFlagsData
    {
        uint32_t flags[2];
    };
#pragma pack(pop)

    /**
     * @brief Helper class to query affect flags cleanly.
     * 
     * Applies Single Responsibility Principle by decoupling flag querying logic
     * from `CActorInstance` and network processing. No Hungarian notation used.
     */
    class AffectFlagsHelper
    {
    public:
        /**
         * @brief Checks if a specific flag is set in the affect flags data.
         * 
         * @param data The affect flags data.
         * @param flagType The specific flag to check for.
         * @return true If the flag is set.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool isFlagSet(const AffectFlagsData& data, AffectFlagType flagType) noexcept
        {
            const uint32_t bitIndex = static_cast<uint32_t>(flagType);
            if (bitIndex >= 64)
            {
                return false;
            }
            const uint32_t arrayIndex = bitIndex / 32;
            const uint32_t bitOffset = bitIndex % 32;
            return (data.flags[arrayIndex] & (1u << bitOffset)) != 0;
        }

        /**
         * @brief Checks if a specific flag is set using a raw binary buffer.
         * 
         * Safely interprets the buffer as an `AffectFlagsData` if large enough.
         * 
         * @param buffer C++20 span over the raw binary bytes.
         * @param flagType The specific flag to check for.
         * @return true If the flag is set.
         * @return false Otherwise or if the buffer is too small.
         */
        [[nodiscard]] static bool isFlagSet(std::span<const uint8_t> buffer, AffectFlagType flagType) noexcept
        {
            if (buffer.size() < sizeof(AffectFlagsData))
            {
                return false;
            }
            
            // Using memcpy or similar to avoid strict aliasing violation is preferred in some cases,
            // but since AffectFlagsData is trivial and packed, a reinterpret_cast is commonly used 
            // in game networking, provided the alignment is safe.
            // Using a safe local copy to prevent unaligned read issues.
            AffectFlagsData data{};
            std::copy_n(buffer.data(), sizeof(AffectFlagsData), reinterpret_cast<uint8_t*>(&data));

            return isFlagSet(data, flagType);
        }

        /**
         * @brief Checks if the actor is poisoned.
         * 
         * @param data The affect flags data.
         * @return true If poisoned.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool isPoisoned(const AffectFlagsData& data) noexcept
        {
            return isFlagSet(data, AffectFlagType::Poison);
        }

        /**
         * @brief Checks if the actor is slowed.
         * 
         * @param data The affect flags data.
         * @return true If slowed.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool isSlowed(const AffectFlagsData& data) noexcept
        {
            return isFlagSet(data, AffectFlagType::Slow);
        }

        /**
         * @brief Checks if the actor is stunned.
         * 
         * @param data The affect flags data.
         * @return true If stunned.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool isStunned(const AffectFlagsData& data) noexcept
        {
            return isFlagSet(data, AffectFlagType::Stun);
        }

        /**
         * @brief Checks if the actor is on fire.
         * 
         * @param data The affect flags data.
         * @return true If on fire.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool isOnFire(const AffectFlagsData& data) noexcept
        {
            return isFlagSet(data, AffectFlagType::Fire);
        }

        /**
         * @brief Checks if the actor has any positive buffs.
         * 
         * @param data The affect flags data.
         * @return true If any positive buff is active.
         * @return false Otherwise.
         */
        [[nodiscard]] static bool hasPositiveBuffs(const AffectFlagsData& data) noexcept
        {
            // Example of checking multiple positive buff flags.
            return isFlagSet(data, AffectFlagType::MovSpeedPotion) ||
                   isFlagSet(data, AffectFlagType::AttSpeedPotion) ||
                   isFlagSet(data, AffectFlagType::Hosin) ||
                   isFlagSet(data, AffectFlagType::Boho) ||
                   isFlagSet(data, AffectFlagType::Heuksin) ||
                   isFlagSet(data, AffectFlagType::Gicheon) ||
                   isFlagSet(data, AffectFlagType::Jeungryeok);
        }

        /**
         * @brief Gets a string representation of the given flag type.
         * 
         * @param flagType The flag type.
         * @return std::string_view A textual name of the flag.
         */
        [[nodiscard]] static std::string_view getFlagName(AffectFlagType flagType) noexcept
        {
            switch (flagType)
            {
            case AffectFlagType::None: return "None";
            case AffectFlagType::Invisibility: return "Invisibility";
            case AffectFlagType::Spawn: return "Spawn";
            case AffectFlagType::Poison: return "Poison";
            case AffectFlagType::Slow: return "Slow";
            case AffectFlagType::Stun: return "Stun";
            case AffectFlagType::DungeonReady: return "DungeonReady";
            case AffectFlagType::ShowAlways: return "ShowAlways";
            case AffectFlagType::BuildingConstructionSmall: return "BuildingConstructionSmall";
            case AffectFlagType::BuildingConstructionLarge: return "BuildingConstructionLarge";
            case AffectFlagType::BuildingUpgrade: return "BuildingUpgrade";
            case AffectFlagType::MovSpeedPotion: return "MovSpeedPotion";
            case AffectFlagType::AttSpeedPotion: return "AttSpeedPotion";
            case AffectFlagType::FishMind: return "FishMind";
            case AffectFlagType::Jeongwi: return "Jeongwi";
            case AffectFlagType::Geomgyeong: return "Geomgyeong";
            case AffectFlagType::Cheongeun: return "Cheongeun";
            case AffectFlagType::Gyeonggong: return "Gyeonggong";
            case AffectFlagType::Eunhyeong: return "Eunhyeong";
            case AffectFlagType::Gwigeom: return "Gwigeom";
            case AffectFlagType::Gongpo: return "Gongpo";
            case AffectFlagType::Jumagap: return "Jumagap";
            case AffectFlagType::Hosin: return "Hosin";
            case AffectFlagType::Boho: return "Boho";
            case AffectFlagType::Kwaesok: return "Kwaesok";
            case AffectFlagType::Heuksin: return "Heuksin";
            case AffectFlagType::Muyeong: return "Muyeong";
            case AffectFlagType::ReviveInvisibility: return "ReviveInvisibility";
            case AffectFlagType::Fire: return "Fire";
            case AffectFlagType::Gicheon: return "Gicheon";
            case AffectFlagType::Jeungryeok: return "Jeungryeok";
            case AffectFlagType::Dash: return "Dash";
            case AffectFlagType::Pabeop: return "Pabeop";
            case AffectFlagType::FallenCheongeun: return "FallenCheongeun";
            case AffectFlagType::Polymorph: return "Polymorph";
            case AffectFlagType::WarFlag1: return "WarFlag1";
            case AffectFlagType::WarFlag2: return "WarFlag2";
            case AffectFlagType::WarFlag3: return "WarFlag3";
            case AffectFlagType::ChinaFirework: return "ChinaFirework";
            case AffectFlagType::PremiumSilver: return "PremiumSilver";
            case AffectFlagType::PremiumGold: return "PremiumGold";
            case AffectFlagType::RamadanRing: return "RamadanRing";
            default: return "Unknown";
            }
        }
    };

} // namespace domain
