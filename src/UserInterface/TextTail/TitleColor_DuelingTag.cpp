#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <unordered_set>
#include <mutex>
#include <format>

namespace UserInterface::Core {
    // Zero-conflict event struct for PvP dueling state notification
    struct DuelingStateChangedEvent {
        EterBase::EntityId entityId;
        bool isDueling;
    };
}

namespace UserInterface::TextTail {

    /**
     * @class DuelingTagColorizer
     * @brief Implements ITitleNameColorizer to append special PvP duel marking.
     */
    class DuelingTagColorizer final : public ITitleNameColorizer {
    public:
        DuelingTagColorizer() = default;
        ~DuelingTagColorizer() override = default;

        /**
         * @brief Gets a specific PvP red color for alignment if the player is dueling.
         * @param alignment Character alignment value (ignored if dueling).
         * @return Color hex code.
         */
        [[nodiscard]] uint32_t GetAlignmentColor(int32_t alignment) const override {
            // Return default or specific red for dueling
            return 0xFFFF0000; // Bright Red for Dueling
        }

        [[nodiscard]] uint32_t GetEmpireColor(uint8_t empire) const override {
            return 0xFFFFFFFF; // White placeholder
        }

        [[nodiscard]] uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override {
            return 0xFFFFFFFF; // White placeholder
        }

        [[nodiscard]] std::string FormatGuildName(std::string_view guildName) const override {
            return std::string(guildName);
        }

        [[nodiscard]] std::string FormatAlignmentTitle(int32_t alignment) const override {
            // Apply dueling tag
            return "[Pojedynek]";
        }

        void Clear() override {
            std::lock_guard<std::mutex> lock(mutex_);
            duelingEntities_.clear();
            EterBase::ModernLogger::Info("DuelingTagColorizer state cleared");
        }

        /**
         * @brief Toggles dueling state for a given entity.
         * @param id Entity ID.
         * @param isDueling Target state.
         * @return Result of the operation.
         */
        std::expected<void, EterBase::EntityError> SetDuelingState(EterBase::EntityId id, bool isDueling) {
            if (!id) {
                EterBase::ModernLogger::Error("Failed to set dueling state: Invalid EntityId");
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (isDueling) {
                    duelingEntities_.insert(id.value());
                } else {
                    duelingEntities_.erase(id.value());
                }
            }

            EterBase::ModernLogger::Info("Entity {} dueling state set to {}", id.value(), isDueling);
            
            Core::EventBus::GetInstance().Publish(Core::DuelingStateChangedEvent{id, isDueling});
            
            return {};
        }

        /**
         * @brief Checks if an entity is currently dueling.
         * @param id Entity ID.
         * @return true if dueling.
         */
        [[nodiscard]] bool IsDueling(EterBase::EntityId id) const {
            std::lock_guard<std::mutex> lock(mutex_);
            return duelingEntities_.contains(id.value());
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_set<uint32_t> duelingEntities_;
    };

} // namespace UserInterface::TextTail
