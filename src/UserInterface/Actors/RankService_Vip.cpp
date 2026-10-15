#include "../StdAfx.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../UserInterface/Core/EventBus.h"

#include <string_view>
#include <string>
#include <expected>
#include <mutex>
#include <unordered_map>
#include <format>

namespace UserInterface::Actors {

/**
 * @brief Event emitted when an actor's VIP/GM title or prefix is updated.
 * 
 * Satisfies the zero-conflict rule by being defined directly in the cpp file.
 */
struct RankVipTitleChangedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    std::string prefix;
    std::string title;

    RankVipTitleChangedEvent(EterBase::EntityId entityId, std::string_view prefix, std::string_view title)
        : entityId(entityId), prefix(prefix), title(title) {}
};

/**
 * @brief Service responsible for managing VIP and GM titles above characters.
 */
class RankServiceVip {
public:
    static RankServiceVip& GetInstance() {
        static RankServiceVip instance;
        return instance;
    }

    /**
     * @brief Updates the VIP/GM title for a specific entity.
     * 
     * @param entityId The ID of the actor.
     * @param isVip Whether the actor has VIP status.
     * @param isGm Whether the actor has GM status.
     * @return std::expected<void, std::string_view> Success or error message.
     */
    std::expected<void, std::string_view> UpdateActorTitle(EterBase::EntityId entityId, bool isVip, bool isGm) {
        if (!entityId) {
            EterBase::ModernLogger::Error("RankServiceVip: Invalid entityId zero passed to UpdateActorTitle.");
            return std::unexpected("Invalid entityId");
        }

        std::string_view prefix = "";
        std::string_view title = "";

        if (isGm) {
            prefix = "[GM]";
            title = "Game Master";
        } else if (isVip) {
            prefix = "[VIP]";
            title = "Premium User";
        } else {
            // Normal user
        }

        {
            std::unique_lock lock(m_mutex);
            m_entityRanks[entityId] = {std::string(prefix), std::string(title)};
        }

        EterBase::ModernLogger::Info("RankServiceVip: Title updated for EntityId {}. Prefix: {}, Title: {}", 
                                     entityId.get(), prefix, title);

        RankVipTitleChangedEvent event(entityId, prefix, title);
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    /**
     * @brief Retrieves the current prefix and title for an entity.
     * 
     * @param entityId The ID of the actor.
     * @return std::expected<std::pair<std::string, std::string>, std::string_view> Pair of (prefix, title) or error.
     */
    std::expected<std::pair<std::string, std::string>, std::string_view> GetActorTitle(EterBase::EntityId entityId) const {
        std::unique_lock lock(m_mutex);
        if (auto it = m_entityRanks.find(entityId); it != m_entityRanks.end()) {
            return it->second;
        }
        return std::unexpected("Entity not found in RankServiceVip");
    }

private:
    RankServiceVip() = default;
    ~RankServiceVip() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<EterBase::EntityId, std::pair<std::string, std::string>> m_entityRanks;
};

} // namespace UserInterface::Actors
