#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <cstring>

namespace metin2::combat
{

/**
 * @brief Defines the type of target available in the combat system.
 */
enum class TargetType : uint8_t
{
    None = 0,
    Player = 1,
    Monster = 2,
    Npc = 3,
    Metin = 4
};

#pragma pack(push, 1)
/**
 * @brief Represents the raw network packet structure for target updates.
 * 
 * Strictly aligned for network serialization.
 */
struct TargetUpdatePacket
{
    uint8_t header;         ///< The packet header identifying the network command.
    uint32_t targetId;      ///< The unique identifier of the target.
    uint8_t targetType;     ///< The raw type of the target.
    float distance;         ///< The current distance to the target.
    char name[32];          ///< Null-terminated or fixed-size string for the target name.
};
#pragma pack(pop)

/**
 * @brief Clean class representing the current target, its type, and distance.
 *
 * It decouples the target state from the Python GUI bindings, maintaining 
 * clean domain state which can be queried safely.
 */
class TargetModel
{
public:
    /**
     * @brief Constructs a new TargetModel with an empty state.
     */
    TargetModel() = default;

    /**
     * @brief Retrieves the unique identifier of the target.
     * @return The target's unique ID.
     */
    [[nodiscard]] uint32_t GetId() const noexcept
    {
        return id;
    }

    /**
     * @brief Retrieves the type of the target.
     * @return The current TargetType.
     */
    [[nodiscard]] TargetType GetType() const noexcept
    {
        return type;
    }

    /**
     * @brief Retrieves the distance to the target.
     * @return The distance in floating-point format.
     */
    [[nodiscard]] float GetDistance() const noexcept
    {
        return distance;
    }

    /**
     * @brief Retrieves the name of the target.
     * @return A std::string_view representing the target's name.
     */
    [[nodiscard]] std::string_view GetName() const noexcept
    {
        return name;
    }

    /**
     * @brief Sets the target's unique identifier.
     * @param newId The new target ID.
     */
    void SetId(uint32_t newId) noexcept
    {
        id = newId;
    }

    /**
     * @brief Sets the target's type.
     * @param newType The new target type.
     */
    void SetType(TargetType newType) noexcept
    {
        type = newType;
    }

    /**
     * @brief Sets the distance to the target.
     * @param newDistance The new distance.
     */
    void SetDistance(float newDistance) noexcept
    {
        distance = newDistance;
    }

    /**
     * @brief Sets the target's name.
     * @param newName A std::string_view representing the new name.
     */
    void SetName(std::string_view newName)
    {
        name = std::string{newName};
    }

    /**
     * @brief Updates the target state from a network buffer.
     * 
     * Parses the given binary buffer interpreting it as a TargetUpdatePacket.
     * @param buffer A span of constant uint8_t representing the raw payload.
     * @return True if the update was successful, false otherwise.
     */
    bool UpdateFromBuffer(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TargetUpdatePacket))
        {
            return false;
        }

        const auto* packet = reinterpret_cast<const TargetUpdatePacket*>(buffer.data());

        SetId(packet->targetId);
        SetType(static_cast<TargetType>(packet->targetType));
        SetDistance(packet->distance);

        // Safely extract the name from a fixed-size char array
        size_t nameLength = strnlen(packet->name, sizeof(packet->name));
        SetName(std::string_view(packet->name, nameLength));

        return true;
    }

    /**
     * @brief Clears the current target state.
     */
    void Clear() noexcept
    {
        id = 0;
        type = TargetType::None;
        distance = 0.0f;
        name.clear();
    }

private:
    uint32_t id{0};                         ///< The target's unique identifier.
    TargetType type{TargetType::None};      ///< The target's entity type.
    float distance{0.0f};                   ///< The distance to the target.
    std::string name;                       ///< The target's name.
};

} // namespace metin2::combat
