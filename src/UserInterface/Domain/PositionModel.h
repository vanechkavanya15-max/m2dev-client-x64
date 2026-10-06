#pragma once

#include <cstdint>

namespace UserInterface::Domain
{
    /**
     * @brief Structure representing a 2D position in local map coordinates.
     */
    struct LocalPosition
    {
        int32_t x;
        int32_t y;
    };

    /**
     * @brief Structure representing a 2D position in global game coordinates.
     */
    struct GlobalPosition
    {
        int32_t x;
        int32_t y;
    };

    /**
     * @brief Model class responsible for converting between global game coordinates
     *        and local map coordinates. Follows the single responsibility principle.
     */
    class PositionModel
    {
    public:
        /**
         * @brief Default constructor initializing base offsets to zero.
         */
        PositionModel() : baseOffset_{0, 0} {}

        /**
         * @brief Updates the base offset used for coordinate conversion.
         * @param baseGlobalX The global X coordinate of the map's base point.
         * @param baseGlobalY The global Y coordinate of the map's base point.
         */
        void SetBaseOffset(int32_t baseGlobalX, int32_t baseGlobalY)
        {
            baseOffset_.x = baseGlobalX;
            baseOffset_.y = baseGlobalY;
        }

        /**
         * @brief Converts a global position to a local map position.
         * @param globalPos The global position to convert.
         * @return The calculated local position.
         */
        [[nodiscard]] LocalPosition GlobalToLocal(const GlobalPosition& globalPos) const
        {
            return LocalPosition{
                globalPos.x - baseOffset_.x,
                globalPos.y - baseOffset_.y
            };
        }

        /**
         * @brief Converts a local map position to a global position.
         * @param localPos The local position to convert.
         * @return The calculated global position.
         */
        [[nodiscard]] GlobalPosition LocalToGlobal(const LocalPosition& localPos) const
        {
            return GlobalPosition{
                localPos.x + baseOffset_.x,
                localPos.y + baseOffset_.y
            };
        }

    private:
        GlobalPosition baseOffset_;
    };
}
