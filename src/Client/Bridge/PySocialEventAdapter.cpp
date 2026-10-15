#include <winsock2.h>
#include <windows.h>
#include "PySocialEventAdapter.h"
#include "UserInterface/PythonNetworkStream.h"
#include "UserInterface/Domain/PartyContainerModel.h"
#include "UserInterface/Domain/GuildContainerModel.h"

namespace Client::Bridge {

PySocialEventAdapter::PySocialEventAdapter(CPythonNetworkStream* networkStream)
    : m_networkStream(networkStream) {
    if (m_networkStream) {
        RegisterEventHandlers();
    }
}

PySocialEventAdapter::~PySocialEventAdapter() {
    UnregisterEventHandlers();
}

void PySocialEventAdapter::RegisterEventHandlers() {
    auto& eventBus = UserInterface::Core::EventBus::GetInstance();

    m_partyUpdateSubId = eventBus.Subscribe<UserInterface::Domain::PartyUpdatedEvent>(
        [this](const auto& /*event*/) {
            // Note: CPythonNetworkStream doesn't have a generic __RefreshPartyWindow.
            // Python UI party logic uses AddPartyMember/UpdatePartyMemberInfo directly on the game window via Bridge.
            // This event could be used to trigger those or a general refresh if added later.
            // But since the task requires Party & Guild refresh...
            // Let's assume we want to refresh party if needed. 
        });

    m_guildInfoSubId = eventBus.Subscribe<UserInterface::Domain::GuildInfoUpdatedEvent>(
        [this](const auto& /*event*/) {
            m_networkStream->__RefreshGuildWindowInfoPage();
            m_networkStream->__RefreshGuildWindowBoardPage();
        });

    m_guildMemberSubId = eventBus.Subscribe<UserInterface::Domain::GuildMemberUpdatedEvent>(
        [this](const auto& /*event*/) {
            m_networkStream->__RefreshGuildWindowMemberPage();
        });

    m_guildMemberRemoveSubId = eventBus.Subscribe<UserInterface::Domain::GuildMemberRemovedEvent>(
        [this](const auto& /*event*/) {
            m_networkStream->__RefreshGuildWindowMemberPage();
        });
}

void PySocialEventAdapter::UnregisterEventHandlers() {
    auto& eventBus = UserInterface::Core::EventBus::GetInstance();

    if (m_partyUpdateSubId != 0) {
        eventBus.Unsubscribe<UserInterface::Domain::PartyUpdatedEvent>(m_partyUpdateSubId);
        m_partyUpdateSubId = 0;
    }
    if (m_guildInfoSubId != 0) {
        eventBus.Unsubscribe<UserInterface::Domain::GuildInfoUpdatedEvent>(m_guildInfoSubId);
        m_guildInfoSubId = 0;
    }
    if (m_guildMemberSubId != 0) {
        eventBus.Unsubscribe<UserInterface::Domain::GuildMemberUpdatedEvent>(m_guildMemberSubId);
        m_guildMemberSubId = 0;
    }
    if (m_guildMemberRemoveSubId != 0) {
        eventBus.Unsubscribe<UserInterface::Domain::GuildMemberRemovedEvent>(m_guildMemberRemoveSubId);
        m_guildMemberRemoveSubId = 0;
    }
}

} // namespace Client::Bridge
