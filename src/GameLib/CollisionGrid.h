#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <algorithm>

/**
 * @brief A lightweight 2D bit grid representing walls and obstacles from attribute files.
 *
 * This class provides an efficient way to store and query collision data (e.g., walls, obstacles)
 * for a specific terrain or area in the game. It uses a flat vector of bits (packed into bytes) to
 * minimize memory usage.
 */
class CollisionGrid {
public:
    /**
     * @brief Constructs a new Collision Grid with the specified dimensions and optional name.
     * 
     * @param width The width of the grid in cells.
     * @param height The height of the grid in cells.
     * @param name The name of the grid (default is empty).
     * @throws std::invalid_argument If the dimensions are zero.
     */
    CollisionGrid(uint32_t width, uint32_t height, std::string_view name = "")
        : width(width), height(height), name(name) {
        if (width == 0 || height == 0) {
            throw std::invalid_argument("Grid dimensions must be greater than zero.");
        }
        
        uint32_t totalBits = width * height;
        uint32_t totalBytes = (totalBits + 7) / 8;
        data.resize(totalBytes, 0);
    }

    /**
     * @brief Default virtual destructor.
     */
    virtual ~CollisionGrid() = default;

    /**
     * @brief Gets the name of the collision grid.
     * 
     * @return std::string_view The name of the grid.
     */
    [[nodiscard]] std::string_view GetName() const noexcept {
        return name;
    }

    /**
     * @brief Sets the name of the collision grid.
     * 
     * @param newName The new name to set.
     */
    void SetName(std::string_view newName) {
        name = std::string(newName);
    }

    /**
     * @brief Gets the width of the collision grid.
     * 
     * @return uint32_t The width of the grid.
     */
    [[nodiscard]] uint32_t GetWidth() const noexcept {
        return width;
    }

    /**
     * @brief Gets the height of the collision grid.
     * 
     * @return uint32_t The height of the grid.
     */
    [[nodiscard]] uint32_t GetHeight() const noexcept {
        return height;
    }

    /**
     * @brief Checks if a specific cell is marked as an obstacle.
     * 
     * @param x The x-coordinate of the cell.
     * @param y The y-coordinate of the cell.
     * @return true If the cell is an obstacle or out of bounds.
     * @return false If the cell is free.
     */
    [[nodiscard]] bool IsObstacle(uint32_t x, uint32_t y) const noexcept {
        if (x >= width || y >= height) {
            return true; // Treat out-of-bounds as obstacles for safety.
        }
        
        uint32_t bitIndex = y * width + x;
        uint32_t byteIndex = bitIndex / 8;
        uint32_t bitOffset = bitIndex % 8;
        
        return (data[byteIndex] & (1 << bitOffset)) != 0;
    }

    /**
     * @brief Wielowymiarowy operator dostepu C++23 umozliwiajacy naturalny syntax grid[x, y].
     * @param x Wspolrzedna X komorki.
     * @param y Wspolrzedna Y komorki.
     * @return true jesli komorka jest przeszkoda, false w przeciwnym razie.
     */
    [[nodiscard]] constexpr bool operator[](uint32_t x, uint32_t y) const noexcept {
        return IsObstacle(x, y);
    }

    /**
     * @brief Sets the obstacle state of a specific cell.
     * 
     * @param x The x-coordinate of the cell.
     * @param y The y-coordinate of the cell.
     * @param isObstacle true to mark as an obstacle, false to mark as free.
     * @throws std::out_of_range If the coordinates are outside the grid boundaries.
     */
    void SetObstacle(uint32_t x, uint32_t y, bool isObstacle) {
        if (x >= width || y >= height) {
            throw std::out_of_range("Coordinates are out of bounds.");
        }
        
        uint32_t bitIndex = y * width + x;
        uint32_t byteIndex = bitIndex / 8;
        uint32_t bitOffset = bitIndex % 8;
        
        if (isObstacle) {
            data[byteIndex] |= (1 << bitOffset);
        } else {
            data[byteIndex] &= ~(1 << bitOffset);
        }
    }

    /**
     * @brief Populates the collision grid from a binary buffer.
     * 
     * @param buffer A span containing the binary attribute data.
     * @throws std::invalid_argument If the buffer size does not match the grid's required size.
     */
    void LoadFromBuffer(std::span<const uint8_t> buffer) {
        if (buffer.size() != data.size()) {
            throw std::invalid_argument("Buffer size does not match the grid size.");
        }
        std::copy(buffer.begin(), buffer.end(), data.begin());
    }
    
    /**
     * @brief Copies the grid data to an external buffer.
     * 
     * @param buffer A span that will receive the binary attribute data.
     * @throws std::invalid_argument If the buffer size does not match the grid's required size.
     */
    void SaveToBuffer(std::span<uint8_t> buffer) const {
        if (buffer.size() != data.size()) {
            throw std::invalid_argument("Buffer size does not match the grid size.");
        }
        std::copy(data.begin(), data.end(), buffer.begin());
    }

    /**
     * @brief Clears the grid, removing all obstacles.
     */
    void Clear() noexcept {
        std::fill(data.begin(), data.end(), 0);
    }

private:
    uint32_t width;
    uint32_t height;
    std::string name;
    std::vector<uint8_t> data;
};
