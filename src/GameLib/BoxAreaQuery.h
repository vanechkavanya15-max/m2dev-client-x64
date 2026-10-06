#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <span>

class CActorInstance;

/**
 * @brief Represents a rectangular area query for detecting actor instances within specific boundaries.
 * 
 * This class provides a 2D bounding box query to filter and store actor instances that fall within
 * the defined coordinates. It is designed to be completely decoupled from GUI logic and follows
 * modern C++20 standards.
 */
class BoxAreaQuery
{
public:
    /**
     * @brief Constructs a BoxAreaQuery with specified minimum and maximum coordinates.
     * 
     * The constructor normalizes the coordinates to ensure min <= max.
     * 
     * @param minX The minimum X coordinate of the query box.
     * @param minY The minimum Y coordinate of the query box.
     * @param maxX The maximum X coordinate of the query box.
     * @param maxY The maximum Y coordinate of the query box.
     */
    BoxAreaQuery(float minX, float minY, float maxX, float maxY)
        : m_minX(std::min(minX, maxX)),
          m_minY(std::min(minY, maxY)),
          m_maxX(std::max(minX, maxX)),
          m_maxY(std::max(minY, maxY))
    {
    }

    /**
     * @brief Destructor.
     */
    ~BoxAreaQuery() = default;

    /**
     * @brief Checks if a given point is inside the bounding box.
     * 
     * @param x The X coordinate to check.
     * @param y The Y coordinate to check.
     * @return true if the point is inside or on the edge of the bounding box, false otherwise.
     */
    [[nodiscard]] bool Contains(float x, float y) const
    {
        return (x >= m_minX && x <= m_maxX && y >= m_minY && y <= m_maxY);
    }

    /**
     * @brief Adds an actor instance to the results if it is valid.
     * 
     * @param instance Pointer to the actor instance to be added.
     */
    void AddResult(CActorInstance* instance)
    {
        if (instance)
        {
            m_results.push_back(instance);
        }
    }

    /**
     * @brief Clears the current query results.
     */
    void ClearResults()
    {
        m_results.clear();
    }

    /**
     * @brief Gets a view of the query results.
     * 
     * @return A std::span representing the contained actor instances.
     */
    [[nodiscard]] std::span<CActorInstance*> GetResults()
    {
        return m_results;
    }

    /**
     * @brief Gets a read-only view of the query results.
     * 
     * @return A std::span representing the contained actor instances.
     */
    [[nodiscard]] std::span<CActorInstance* const> GetResults() const
    {
        return m_results;
    }

    /**
     * @brief Retrieves the number of actor instances found.
     * 
     * @return The number of instances in the results vector.
     */
    [[nodiscard]] size_t GetResultCount() const
    {
        return m_results.size();
    }

private:
    float m_minX;
    float m_minY;
    float m_maxX;
    float m_maxY;
    
    std::vector<CActorInstance*> m_results;
};
