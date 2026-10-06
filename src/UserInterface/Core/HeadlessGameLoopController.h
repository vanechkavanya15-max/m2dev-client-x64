#pragma once

#include <expected>
#include <optional>
#include <chrono>
#include <functional>
#include <format>
#include <string_view>

#include "EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Core {

/**
 * @brief Headless game loop controller, bypassing Direct3D and GUI for event-driven autonomous execution.
 * 
 * This class handles the main logic execution of the Metin2 client when running in a headless
 * context (e.g., automated testing, botting, or zero-conflict background operations).
 * It heavily relies on EterBase::StrongTypes and C++23 monadic operations.
 */
class HeadlessGameLoopController {
public:
    /**
     * @brief Constructs the HeadlessGameLoopController.
     */
    HeadlessGameLoopController() = default;

    /**
     * @brief Destroys the HeadlessGameLoopController.
     */
    ~HeadlessGameLoopController() = default;

    /**
     * @brief Initializes the headless game loop.
     * 
     * Subscribes to necessary events in the EventBus and prepares internal state.
     * 
     * @return std::expected<void, std::string_view> Returns success or an error message on failure.
     */
    [[nodiscard]] std::expected<void, std::string_view> Initialize() {
        m_packetSubscriptionId = EventBus::GetInstance().Subscribe<NetworkPacketReceivedEvent>(
            [this](const NetworkPacketReceivedEvent& event) {
                auto result = ProcessEvent(event);
                if (!result.has_value()) {
                    EterBase::ModernLogger::Error("Failed to process event: {}", result.error());
                }
            }
        );
        
        m_isRunning = true;
        EterBase::ModernLogger::Info("HeadlessGameLoopController initialized successfully.");
        return {};
    }

    /**
     * @brief Starts and maintains the headless game loop.
     * 
     * @return std::expected<void, std::string_view> Returns success upon clean exit, or an error message if the loop crashes.
     */
    [[nodiscard]] std::expected<void, std::string_view> Run() {
        if (!m_isRunning.value_or(false)) {
            return std::unexpected("Cannot run: Controller is not initialized or has been stopped.");
        }

        auto lastTick = std::chrono::steady_clock::now();

        EterBase::ModernLogger::Info("Entering headless game loop.");

        while (m_isRunning.value_or(false)) {
            auto currentTick = std::chrono::steady_clock::now();
            auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTick - lastTick);

            if (deltaTime.count() > 0) {
                auto tickResult = Tick(deltaTime);
                if (!tickResult.has_value()) {
                    EterBase::ModernLogger::Error("Tick failed: {}", tickResult.error());
                }
                lastTick = currentTick;
            }
            
            // In a real headless environment, you would yield or sleep here briefly to prevent 100% CPU usage.
            // For now, we simulate a small sleep or rely on external mechanics.
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
            // Artificial stop for testing to avoid infinite loop
            // m_isRunning = false;
        }

        return {};
    }

    /**
     * @brief Executes a single update tick for the game logic.
     * 
     * @param deltaTime The elapsed time since the last tick.
     * @return std::expected<void, std::string_view> Returns success or an error message if the tick logic fails.
     */
    [[nodiscard]] std::expected<void, std::string_view> Tick(std::chrono::milliseconds deltaTime) {
        // Monadic operation example: processing logic based on local player ID
        m_localPlayerId.and_then([deltaTime](EterBase::EntityId id) -> std::optional<EterBase::EntityId> {
            // Simulated game state update for the entity using deltaTime
            
            // Emit target board refresh event using the EventBus instance,
            // complying with decoupling and event-driven requirements.
            EventBus::GetInstance().Publish(TargetBoardRefreshEvent(id.value()));
            
            return id;
        }).value_or(EterBase::EntityId{0});

        return {};
    }

    /**
     * @brief Processes incoming events without triggering GUI rendering.
     * 
     * @param event The base event to process.
     * @return std::expected<void, std::string_view> Returns success or an error message on processing failure.
     */
    [[nodiscard]] std::expected<void, std::string_view> ProcessEvent(const IEvent& event) {
        if (const auto* packetEvent = dynamic_cast<const NetworkPacketReceivedEvent*>(&event)) {
            // EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "Headless logic processing packet header: {}", packetEvent->header);
            // Simulated event processing
            return {};
        }

        if (const auto* refreshEvent = dynamic_cast<const TargetBoardRefreshEvent*>(&event)) {
            // EterBase::ModernLogger::Log(EterBase::LogLevel::Trace, "Headless logic refreshing target ID: {}", refreshEvent->targetId);
            return {};
        }

        return std::unexpected("Unhandled event type in HeadlessGameLoopController.");
    }

    /**
     * @brief Stops the headless loop.
     */
    void Stop() {
        m_isRunning = false;
        if (m_packetSubscriptionId.has_value()) {
            EventBus::GetInstance().Unsubscribe<NetworkPacketReceivedEvent>(m_packetSubscriptionId.value());
            m_packetSubscriptionId.reset();
        }
        EterBase::ModernLogger::Info("HeadlessGameLoopController stopped.");
    }

    /**
     * @brief Sets the local player entity ID.
     * 
     * @param id The entity ID to set.
     */
    void SetLocalPlayerId(EterBase::EntityId id) {
        m_localPlayerId = id;
    }

private:
    std::optional<bool> m_isRunning{false};
    std::optional<EterBase::EntityId> m_localPlayerId;
    std::optional<uint32_t> m_packetSubscriptionId;
};

} // namespace UserInterface::Core
