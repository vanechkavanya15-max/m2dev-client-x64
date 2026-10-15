#pragma once

#include <cstdint>

/**
 * @brief Represents a single navigation node used in pathfinding algorithms.
 *
 * This structure holds the coordinates of the node along with the costs 
 * associated with traversing to it, which are typically used in A* or 
 * similar pathfinding algorithms.
 */
struct PathNode
{
    /**
     * @brief The X coordinate of the node in the grid or map.
     */
    int32_t x;

    /**
     * @brief The Y coordinate of the node in the grid or map.
     */
    int32_t y;

    /**
     * @brief The cost from the start node to this node.
     * 
     * gScore represents the exact cost of the path from the starting point 
     * to this node.
     */
    float gScore;

    /**
     * @brief The heuristic estimated cost from this node to the goal node.
     * 
     * hScore is an estimate of the cost to reach the destination from 
     * this node.
     */
    float hScore;

    /**
     * @brief The total expected cost of the path through this node.
     * 
     * fScore is the sum of the gScore and the hScore. 
     * fScore = gScore + hScore.
     */
    float fScore;

    /**
     * @brief A pointer to the parent node in the path.
     * 
     * This is used to retrace the path from the destination back to 
     * the starting point once the goal is reached.
     */
    PathNode* parent;

    /**
     * @brief Default constructor for PathNode.
     * 
     * Initializes all coordinates and costs to zero, and the parent 
     * pointer to nullptr.
     */
    PathNode() 
        : x(0), y(0), gScore(0.0f), hScore(0.0f), fScore(0.0f), parent(nullptr)
    {
    }

    /**
     * @brief Parameterized constructor for PathNode.
     * 
     * @param nodeX The X coordinate of the node.
     * @param nodeY The Y coordinate of the node.
     */
    PathNode(int32_t nodeX, int32_t nodeY)
        : x(nodeX), y(nodeY), gScore(0.0f), hScore(0.0f), fScore(0.0f), parent(nullptr)
    {
    }

    /**
     * @brief Calculates and updates the fScore based on the current gScore and hScore.
     * 
     * @return true if the calculation was successful, false otherwise. 
     *         For this simple struct, it always returns true.
     */
    bool UpdateCosts()
    {
        fScore = gScore + hScore;
        return true;
    }
};
