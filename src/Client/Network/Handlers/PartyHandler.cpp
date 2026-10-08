#include "../../../EterBase/StdAfx.h"
#include "PartyHandler.h"
#include "../../Gameplay/SocialDomain.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/StrongTypes.h"
#include "../../../UserInterface/Packet.h"

namespace Client::Network::Handlers {

    PartyHandler::PartyHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager)
        : m_socialManager(std::move(socialManager)) {
        if (!m_socialManager) {
            EterBase::ModernLogger::Error("PartyHandler initialized with null SocialManager.");
        }
    }

    EterBase::PacketResult<void> PartyHandler::HandlePartyInvite(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCPartyInvite)) {
            EterBase::ModernLogger::Error("PartyHandler::HandlePartyInvite: Buffer underflow. Expected {}, got {}",
                sizeof(TPacketGCPartyInvite), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCPartyInvite*>(payload.data());
        Client::Core::EntityVid leaderVid(packet->leader_pid);

        EterBase::ModernLogger::Info("PartyHandler: Received party invite from VID: {}", leaderVid.get());
        
        // Emitting an event or updating UI would normally happen here. 
        // For SocialDomain, invite state is typically transient or handled in another manager,
        // so we just log it as per zero-conflict design.
        
        return {};
    }

    EterBase::PacketResult<void> PartyHandler::HandlePartyAdd(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCPartyAdd)) {
            EterBase::ModernLogger::Error("PartyHandler::HandlePartyAdd: Buffer underflow. Expected {}, got {}",
                sizeof(TPacketGCPartyAdd), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCPartyAdd*>(payload.data());
        Client::Core::EntityVid memberVid(packet->pid);
        std::string memberName(packet->name, strnlen(packet->name, sizeof(packet->name)));

        EterBase::ModernLogger::Info("PartyHandler: Adding member {} (VID: {})", memberName, memberVid.get());

        if (m_socialManager) {
            m_socialManager->CreateParty();
            auto party = m_socialManager->GetParty();
            if (party) {
                // First member added is set as leader by SocialDomain logic, 
                // but we can pass false here unless we specifically know they are leader.
                Client::Gameplay::PartyMember newMember(EterBase::EntityId(memberVid.get()), memberName, false);
                auto result = party->AddMember(newMember);
                if (!result) {
                    EterBase::ModernLogger::Warning("PartyHandler: Failed to add member: {}", result.error());
                }
            }
        }

        return {};
    }

    EterBase::PacketResult<void> PartyHandler::HandlePartyUpdate(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCPartyUpdate)) {
            EterBase::ModernLogger::Error("PartyHandler::HandlePartyUpdate: Buffer underflow. Expected {}, got {}",
                sizeof(TPacketGCPartyUpdate), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCPartyUpdate*>(payload.data());
        Client::Core::EntityVid memberVid(packet->pid);

        EterBase::ModernLogger::Debug("PartyHandler: Updating member VID: {}, HP: {}%", memberVid.get(), packet->percent_hp);

        if (m_socialManager) {
            auto party = m_socialManager->GetParty();
            if (party) {
                auto result = party->UpdateMemberHP(EterBase::EntityId(memberVid.get()), packet->percent_hp);
                if (!result) {
                    EterBase::ModernLogger::Warning("PartyHandler: Failed to update member: {}", result.error());
                }
            }
        }

        return {};
    }

    EterBase::PacketResult<void> PartyHandler::HandlePartyRemove(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCPartyRemove)) {
            EterBase::ModernLogger::Error("PartyHandler::HandlePartyRemove: Buffer underflow. Expected {}, got {}",
                sizeof(TPacketGCPartyRemove), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCPartyRemove*>(payload.data());
        Client::Core::EntityVid memberVid(packet->pid);

        EterBase::ModernLogger::Info("PartyHandler: Removing member VID: {}", memberVid.get());

        if (m_socialManager) {
            auto party = m_socialManager->GetParty();
            if (party) {
                auto result = party->RemoveMember(EterBase::EntityId(memberVid.get()));
                if (!result) {
                    EterBase::ModernLogger::Warning("PartyHandler: Failed to remove member: {}", result.error());
                }
                
                if (party->IsEmpty()) {
                    EterBase::ModernLogger::Info("PartyHandler: Party is empty, leaving party.");
                    m_socialManager->LeaveParty();
                }
            }
        }

        return {};
    }

} // namespace Client::Network::Handlers
