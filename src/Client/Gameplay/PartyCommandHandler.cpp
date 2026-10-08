#include "../StdAfx.h"
#include "PartyCommandHandler.h"

namespace Client::Gameplay {

    PartyCommandHandler::PartyCommandHandler(std::shared_ptr<SocialManager> socialManager)
        : m_socialManager(std::move(socialManager)) {
    }

    EterBase::Result<void, CommandError> PartyCommandHandler::Invite(EterBase::EntityId playerVid, EterBase::EntityId targetVid) {
        if (!m_socialManager) {
            return std::unexpected(CommandError::NotInParty); // Arbitrary error for null manager
        }

        if (playerVid == targetVid) {
            return std::unexpected(CommandError::InvalidTarget);
        }

        auto party = m_socialManager->GetParty();
        
        if (party) {
            // Check if player is leader
            auto member = party->GetMember(playerVid);
            if (!member) {
                // If the player is not in the party but the party exists on the manager, it means this manager represents someone else or state is weird.
                // Assuming m_socialManager represents the current client's state, the current client should be in the party.
                return std::unexpected(CommandError::NotInParty); 
            }
            if (!member->isLeader) {
                return std::unexpected(CommandError::NotLeader);
            }
            // Check if target is already in the party
            if (party->GetMember(targetVid)) {
                return std::unexpected(CommandError::AlreadyInParty);
            }
        } else {
            // Player is not in a party, create one.
            m_socialManager->CreateParty();
            party = m_socialManager->GetParty();
            // Add the player as the leader first
            party->AddMember(PartyMember(playerVid, "Player", true));
        }

        // Send invite logic would go here. For now just add to party.
        // Usually, an invite is sent and then accepted. We simulate immediate acceptance for logic completeness or just adding.
        // Actually, the prompt says "obsługuje akcje grupy: zaproszenie po VID". 
        // Adding directly to party.
        auto res = party->AddMember(PartyMember(targetVid, "Target"));
        if (!res) {
            // Already in party should have been caught, but just in case
            return std::unexpected(CommandError::AlreadyInParty);
        }

        return {};
    }

    EterBase::Result<void, CommandError> PartyCommandHandler::Leave(EterBase::EntityId playerVid) {
        if (!m_socialManager) {
            return std::unexpected(CommandError::NotInParty);
        }

        auto party = m_socialManager->GetParty();
        if (!party) {
            return std::unexpected(CommandError::NotInParty);
        }
        
        auto member = party->GetMember(playerVid);
        if (!member) {
            return std::unexpected(CommandError::NotInParty);
        }

        auto result = party->RemoveMember(playerVid);
        if (!result) {
            return std::unexpected(CommandError::NotInParty);
        }

        if (party->IsEmpty()) {
            m_socialManager->LeaveParty();
        }

        return {};
    }

    EterBase::Result<void, CommandError> PartyCommandHandler::ChangeLeader(EterBase::EntityId playerVid, EterBase::EntityId newLeaderVid) {
        if (!m_socialManager) {
            return std::unexpected(CommandError::NotInParty);
        }

        if (playerVid == newLeaderVid) {
             return std::unexpected(CommandError::InvalidTarget);
        }

        auto party = m_socialManager->GetParty();
        if (!party) {
            return std::unexpected(CommandError::NotInParty);
        }
        
        auto member = party->GetMember(playerVid);
        if (!member) {
            return std::unexpected(CommandError::NotInParty);
        }

        if (!member->isLeader) {
            return std::unexpected(CommandError::NotLeader);
        }

        auto newLeader = party->GetMember(newLeaderVid);
        if (!newLeader) {
            return std::unexpected(CommandError::PlayerNotFound);
        }
        
        auto res = party->SetLeader(newLeaderVid);
        if (!res) {
            return std::unexpected(CommandError::PlayerNotFound); // Should not happen
        }

        return {};
    }

} // namespace Client::Gameplay
