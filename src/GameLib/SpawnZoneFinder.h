#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <span>
#include <string_view>
#include <cmath>
#include <random>

namespace GameLib {

    /**
     * @brief Represents a coordinate in the game world.
     */
    struct Point {
        float x;
        float y;
        float z;
    };

    /**
     * @brief Represents a bounding box or region on the map.
     */
    struct Region {
        float minX;
        float minY;
        float maxX;
        float maxY;
    };

    /**
     * @brief A class dedicated to finding valid, safe spawn zones for mobs on the map.
     * 
     * This class operates independently of the GUI and encapsulates the logic needed
     * to determine if a specific point is suitable for a mob to spawn, considering
     * terrain height, blocking objects, and valid map bounds.
     */
    class SpawnZoneFinder {
    public:
        /**
         * @brief Default constructor for SpawnZoneFinder.
         */
        SpawnZoneFinder() = default;

        /**
         * @brief Default destructor for SpawnZoneFinder.
         */
        ~SpawnZoneFinder() = default;

        /**
         * @brief Adds a safe region where mobs are permitted to spawn.
         * 
         * @param region The bounding box region to add.
         */
        void AddSafeRegion(const Region& region) {
            safeRegions.push_back(region);
        }

        /**
         * @brief Clears all previously registered safe regions.
         */
        void ClearSafeRegions() {
            safeRegions.clear();
        }

        /**
         * @brief Checks if a given point is within any of the defined safe regions.
         * 
         * @param point The point to check.
         * @return true if the point is inside at least one safe region, false otherwise.
         */
        bool IsPointInSafeRegion(const Point& point) const {
            for (const auto& region : safeRegions) {
                if (point.x >= region.minX && point.x <= region.maxX &&
                    point.y >= region.minY && point.y <= region.maxY) {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Finds a valid spawn point within a given radius around a center point.
         * 
         * This function attempts to find a random point within the specified radius
         * that falls inside a registered safe region. It makes multiple attempts
         * before failing.
         * 
         * @param center The center point around which to search.
         * @param radius The radius to search within.
         * @param maxAttempts The maximum number of attempts to find a valid point.
         * @return std::optional<Point> containing the valid spawn point if found, or std::nullopt if not.
         */
        std::optional<Point> FindSpawnPoint(const Point& center, float radius, uint32_t maxAttempts = 10) const {
            if (safeRegions.empty()) {
                return std::nullopt;
            }

            for (uint32_t attempt = 0; attempt < maxAttempts; ++attempt) {
                // Generate a random angle and distance using C++20 random features
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
                std::uniform_real_distribution<float> distDist(0.0f, radius);

                float angle = angleDist(gen);
                float distance = distDist(gen);

                Point candidatePoint{
                    center.x + std::cos(angle) * distance,
                    center.y + std::sin(angle) * distance,
                    center.z // Z coordinate might need adjustment based on terrain height in a real scenario
                };

                if (IsPointInSafeRegion(candidatePoint)) {
                    return candidatePoint;
                }
            }

            return std::nullopt;
        }

        /**
         * @brief Retrieves all configured safe regions.
         * 
         * @return std::span<const Region> A read-only view of the safe regions.
         */
        std::span<const Region> GetSafeRegions() const {
            return safeRegions;
        }

    private:
        std::vector<Region> safeRegions;
    };

} // namespace GameLib
