#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Network/Handlers/TargetHPHandler.h"

namespace UserInterface::Domain
{

/**
 * @brief Represents the living status of a target.
 */
enum class LifeStatus : uint8_t
{
    None = 0,
    Alive,
    Dead
};

/**
 * @brief Domain model for managing the current attack target's state.
 * 
 * Tracks the current target ID, HP percentage, and life status.
 * Updates are decoupled from the GUI through the EventBus.
 */
class TargetStateModel
{
public:
    /**
     * @brief Constructs a new TargetStateModel with no active target.
     */
    TargetStateModel() = default;

    /**
     * @brief Retrieves the current target ID if one is set.
     * @return An optional containing the EntityId, or std::nullopt if no target is active.
     */
    [[nodiscard]] std::optional<EterBase::EntityId> GetTargetId() const noexcept
    {
        return targetId;
    }

    /**
     * @brief Retrieves the current HP percentage of the target.
     * @return The HP percentage (0-100).
     */
    [[nodiscard]] uint8_t GetHpPercent() const noexcept
    {
        return hpPercent;
    }

    /**
     * @brief Retrieves the current life status of the target.
     * @return The life status (None, Alive, or Dead).
     */
    [[nodiscard]] LifeStatus GetLifeStatus() const noexcept
    {
        return lifeStatus;
    }

    /**
     * @brief Sets or clears the current target.
     * @param id An optional containing the new EntityId, or std::nullopt to clear the target.
     */
    void SetTargetId(std::optional<EterBase::EntityId> id) noexcept
    {
        targetId = id;
        if (!targetId.has_value())
        {
            hpPercent = 0;
            lifeStatus = LifeStatus::None;
        }
    }

    /**
     * @brief Updates the target state from a network buffer.
     * 
     * Parses the binary payload into a PacketTargetHP struct. If valid,
     * updates the target's HP and life status, and emits a TargetBoardRefreshEvent.
     * 
     * @param buffer The binary data representing the packet.
     * @return A PacketResult indicating success or a specific packet error.
     */
    EterBase::PacketResult<void> UpdateTargetState(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(Network::Handlers::PacketTargetHP))
        {
            EterBase::ModernLogger::Warn("TargetStateModel: Buffer underflow while processing Target HP packet.");
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const Network::Handlers::PacketTargetHP*>(buffer.data());

        // Update domain state
        targetId = EterBase::EntityId{packet->targetVid};
        hpPercent = packet->hpPercentage;
        
        if (hpPercent == 0)
        {
            lifeStatus = LifeStatus::Dead;
        }
        else
        {
            lifeStatus = LifeStatus::Alive;
        }

        EterBase::ModernLogger::Debug("TargetStateModel: Updated target {} to HP {}%", 
            targetId.value().value(), hpPercent);

        // Notify GUI (Event-driven decoupling)
        UserInterface::Core::TargetBoardRefreshEvent refreshEvent{packet->targetVid};
        
        // Note: Using GetInstance() as defined in EventBus.h despite Instance() in prompt
        UserInterface::Core::EventBus::GetInstance().Publish(refreshEvent);

        return {};
    }

    /**
     * @brief Clears the current target state completely.
     */
    void Clear() noexcept
    {
        targetId = std::nullopt;
        hpPercent = 0;
        lifeStatus = LifeStatus::None;
    }

private:
    std::optional<EterBase::EntityId> targetId{std::nullopt};
    uint8_t hpPercent{0};
    LifeStatus lifeStatus{LifeStatus::None};
};

} // namespace UserInterface::Domain
