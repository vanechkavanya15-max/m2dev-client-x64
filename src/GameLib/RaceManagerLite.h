#pragma once

#include <cstdint>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <optional>

/**
 * @brief Represents the fundamental category of a race (VNUM).
 */
enum class RaceType : uint8_t
{
    Unknown = 0,
    Player = 1,
    Monster = 2,
    Metin = 3,
    Boss = 4,
    Npc = 5
};

/**
 * @brief A lightweight, modern C++20 manager for looking up race types by VNUM.
 * 
 * This class replaces the older map lookups with a thread-safe unordered_map
 * and std::shared_mutex for fast concurrent reads. It uses modern C++ types 
 * (no Hungarian notation) and focuses entirely on the data logic decoupled from GUI.
 */
class RaceManagerLite
{
public:
    /**
     * @brief Gets the singleton instance of RaceManagerLite.
     * 
     * @return RaceManagerLite& The singleton instance.
     */
    static RaceManagerLite& GetInstance()
    {
        static RaceManagerLite instance;
        return instance;
    }

    RaceManagerLite(const RaceManagerLite&) = delete;
    RaceManagerLite& operator=(const RaceManagerLite&) = delete;

    /**
     * @brief Registers a new race type for a given VNUM.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @param type The fundamental category of the race.
     */
    void RegisterRaceType(uint32_t vnum, RaceType type)
    {
        std::unique_lock lock(mutex_);
        raceTypes_[vnum] = type;
    }

    /**
     * @brief Retrieves the race type for a given VNUM.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return RaceType The category of the race, or RaceType::Unknown if not found.
     */
    RaceType GetRaceType(uint32_t vnum) const
    {
        std::shared_lock lock(mutex_);
        if (auto it = raceTypes_.find(vnum); it != raceTypes_.end())
        {
            return it->second;
        }
        return RaceType::Unknown;
    }

    /**
     * @brief Checks if a given VNUM corresponds to a Metin stone.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return true If the race is a Metin stone.
     * @return false Otherwise.
     */
    bool IsMetin(uint32_t vnum) const
    {
        return GetRaceType(vnum) == RaceType::Metin;
    }

    /**
     * @brief Checks if a given VNUM corresponds to a regular Monster.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return true If the race is a Monster.
     * @return false Otherwise.
     */
    bool IsMonster(uint32_t vnum) const
    {
        return GetRaceType(vnum) == RaceType::Monster;
    }

    /**
     * @brief Checks if a given VNUM corresponds to a Boss.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return true If the race is a Boss.
     * @return false Otherwise.
     */
    bool IsBoss(uint32_t vnum) const
    {
        return GetRaceType(vnum) == RaceType::Boss;
    }

    /**
     * @brief Checks if a given VNUM corresponds to an NPC.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return true If the race is an NPC.
     * @return false Otherwise.
     */
    bool IsNpc(uint32_t vnum) const
    {
        return GetRaceType(vnum) == RaceType::Npc;
    }

    /**
     * @brief Checks if a given VNUM corresponds to a Player.
     * 
     * @param vnum The unique race identifier (VNUM).
     * @return true If the race is a Player.
     * @return false Otherwise.
     */
    bool IsPlayer(uint32_t vnum) const
    {
        return GetRaceType(vnum) == RaceType::Player;
    }

    /**
     * @brief Clears all registered race types.
     */
    void Clear()
    {
        std::unique_lock lock(mutex_);
        raceTypes_.clear();
    }

private:
    RaceManagerLite() = default;
    ~RaceManagerLite() = default;

    mutable std::shared_mutex mutex_;
    std::unordered_map<uint32_t, RaceType> raceTypes_;
};
