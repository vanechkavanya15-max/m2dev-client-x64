#include "ProtocolAdapter.h"
#include "Protocol/Protocol.h"

namespace Client::Network {

ProtocolAdapter::ProtocolAdapter(ServerProfile profile)
    : m_profile(profile) {
}

EterBase::PacketResult<uint16_t> ProtocolAdapter::TranslateClientToServer(uint16_t clientOpcode) const {
    if (m_profile == ServerProfile::ClassicYmir) {
        return clientOpcode;
    } else if (m_profile == ServerProfile::Pandora) {
        switch (clientOpcode) {
            case CG::ATTACK: return 0x02;
            case CG::USE_SKILL: return 0x36;
            case CG::ITEM_PICKUP: return 0x0F;
            case CG::ON_CLICK: return 0x1A;
            case CG::SHOP: return 0x32;
            case CG::SCRIPT_ANSWER: return 0x1D;
            case CG::LOGIN2: 
            case CG::LOGIN3: return 0x01;
            case CG::MOVE: return 0x07;
            default: return clientOpcode; 
        }
    }
    
    return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
}

EterBase::PacketResult<uint16_t> ProtocolAdapter::TranslateServerToClient(uint16_t serverOpcode) const {
    if (m_profile == ServerProfile::ClassicYmir) {
        return serverOpcode;
    } else if (m_profile == ServerProfile::Pandora) {
        switch (serverOpcode) {
            case 0x02: return CG::ATTACK;
            case 0x36: return CG::USE_SKILL;
            case 0x0F: return CG::ITEM_PICKUP;
            case 0x1A: return CG::ON_CLICK;
            case 0x32: return CG::SHOP;
            case 0x1D: return CG::SCRIPT_ANSWER;
            case 0x01: return CG::LOGIN2; 
            case 0x07: return CG::MOVE;
            default: return serverOpcode;
        }
    }

    return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
}

} // namespace Client::Network
