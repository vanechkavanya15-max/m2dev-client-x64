#include "EterLib/StdAfx.h"
#include "BeaviumProtocolDriver.h"
#include "BeaviumProtocolProfile.h"
#include "../BeaviumProtocol.h"
#include "UserInterface/Contracts/IGameEvents.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include "EterBase/EventBus.h"
#include "EterBase/Timer.h"
#include "EterLib/NetStream.h"
#include "EterLib/SodiumCryptoProvider.h"
#include <cstring>
#include <algorithm>

#pragma pack(push, 1)
/**
 * @brief Struktura proxy dla ataku Beavium (8 bajtow).
 */
struct ProxyPacketCGAttack
{
    uint8_t  header;     ///< Beavium::CG::ATTACK = 0x02
    uint8_t  attackType; ///< Typ ataku
    uint32_t targetVid;  ///< Virtual ID celu
    uint16_t sequence;   ///< Sekwencja / CRC
};
static_assert(sizeof(ProxyPacketCGAttack) == 8, "ProxyPacketCGAttack musi miec dokladnie 8 bajtow");
#pragma pack(pop)

namespace Network::Protocol::Drivers
{
    std::optional<FrameHeaderInfo> BeaviumProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagany co najmniej 1 bajt naglowka
        if (buffer.empty())
        {
            return std::nullopt;
        }

        uint8_t header = buffer[0];
        Beavium::BeaviumGCInfo info{};
        if (!Beavium::GetGCInfo(header, info))
        {
            return std::nullopt;
        }

        if (!info.isDynamic)
        {
            // Pakiet o stalym rozmiarze ze statycznej tablicy beavium_gc_table.inl
            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = info.size,
                .headerSize = 1
            };
        }
        else
        {
            // Pakiety dynamiczne - bezpieczna obsluga wedlug kategorii strukturalnych Beavium
            switch (header)
            {
            case 0x0F: // TSkillCooldownPacket (6B naglowek, uint32_t total_size)
            case 0x49: // Dynamiczny bufor 32-bit
            case 0x6A: // HEADER_GC_RANKING_OPEN (6B naglowek, uint32_t total_size)
            case 0x8A: // HEADER_GC_PET_INFO (6B naglowek, uint32_t total_size)
            case 0x90: // HEADER_GC_VOICE_CHAT_DATA (6B naglowek, uint32_t total_size)
            case 0x91:
            {
                if (buffer.size() < 5)
                {
                    return std::nullopt;
                }
                uint32_t dynLength = 0;
                std::memcpy(&dynLength, &buffer[1], sizeof(uint32_t));
                if (dynLength < 5 || dynLength > 65535)
                {
                    return std::nullopt;
                }
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = dynLength,
                    .headerSize = 1
                };
            }
            case 0x53: // HEADER_GC_MINIMAP_WAYPOINTS (5B naglowek, count na offsetcie 3)
            {
                if (buffer.size() < 5)
                {
                    return std::nullopt;
                }
                uint16_t count = static_cast<uint16_t>(buffer[3] | (static_cast<uint16_t>(buffer[4]) << 8));
                uint32_t totalLength = 5 + static_cast<uint32_t>(count) * 13;
                if (totalLength > 65535)
                {
                    return std::nullopt;
                }
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = totalLength,
                    .headerSize = 1
                };
            }
            case 0x4F: // HEADER_GC_CHANNEL_STATUS (4B bazowo, 12B jesli flaga==1)
            {
                if (buffer.size() < 4)
                {
                    return std::nullopt;
                }
                uint32_t totalLength = (buffer[3] == 0x01) ? 12 : 4;
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = totalLength,
                    .headerSize = 1
                };
            }
            case 0x04: // HEADER_GC_CHAT (14B bazowy naglowek, uint16_t length na offsetcie 1)
            {
                if (buffer.size() < 14)
                {
                    return std::nullopt;
                }
                uint32_t dynLength = static_cast<uint32_t>(buffer[1] | (static_cast<uint32_t>(buffer[2]) << 8));
                if (dynLength < 14 || dynLength > 65535)
                {
                    return std::nullopt;
                }
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = dynLength,
                    .headerSize = 1
                };
            }
            case 0x25: // HEADER_GC_SCRIPT (6B naglowek, uint16_t length na offsetcie 1)
            {
                if (buffer.size() < 6)
                {
                    return std::nullopt;
                }
                uint32_t dynLength = static_cast<uint32_t>(buffer[1] | (static_cast<uint32_t>(buffer[2]) << 8));
                if (dynLength < 6 || dynLength > 65535)
                {
                    return std::nullopt;
                }
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = dynLength,
                    .headerSize = 1
                };
            }
            default:
            {
                if (buffer.size() < 3)
                {
                    return std::nullopt;
                }
                uint32_t dynLength = static_cast<uint32_t>(buffer[1] | (static_cast<uint32_t>(buffer[2]) << 8));
                if (dynLength < 3 || dynLength > 65535)
                {
                    return std::nullopt;
                }
                return FrameHeaderInfo{
                    .unifiedOpcode = static_cast<uint16_t>(header),
                    .packetLength = dynLength,
                    .headerSize = 1
                };
            }
            }
        }
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        ProxyPacketCGAttack packet{};
        packet.header = Beavium::CG::ATTACK; // 0x02
        packet.attackType = cmd.attackType;
        packet.targetVid = cmd.targetVid;
        packet.sequence = cmd.sequence;

        std::vector<uint8_t> buffer(sizeof(ProxyPacketCGAttack));
        std::memcpy(buffer.data(), &packet, sizeof(ProxyPacketCGAttack));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        Beavium::TPacketCGMoveBeavium packet{};
        packet.header = Beavium::CG::MOVE; // 0x07
        packet.bFunc = cmd.func;
        packet.wArg = cmd.arg;
        packet.dwRot = Beavium::EncodeRotationMicrodegrees(cmd.rotationDegrees);
        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;
        packet.dwExtra = 0;
        packet.bFlag = 0;
        packet.dwReserved = 0xFFFFFFFF;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGMoveBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGMoveBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeItemUse(
        uint16_t cell, uint8_t windowType) const
    {
        Beavium::TPacketCGItemUseBeavium packet{};
        packet.header = Beavium::CG::ITEM_USE; // 0x0C
        packet.cell = cell;
        packet.window = windowType;
        packet.vnum = 0;
        packet.targetVid = 0;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGItemUseBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGItemUseBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeTarget(
        uint32_t targetVid) const
    {
        Beavium::TPacketCGTargetBeavium packet{};
        packet.header = Beavium::CG::TARGET; // 0x22
        packet.targetVid = targetVid;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGTargetBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGTargetBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeItemMove(
        const Domain::ItemMoveCommand& cmd) const
    {
        Beavium::TPacketCGItemMoveBeavium packet{};
        packet.header = Beavium::CG::ITEM_MOVE; // 0x0D
        packet.srcWindow = cmd.srcWindow;
        packet.srcCell = cmd.srcCell;
        packet.dstWindow = cmd.dstWindow;
        packet.dstCell = cmd.dstCell;
        packet.count = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGItemMoveBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGItemMoveBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeItemDrop(
        const Domain::ItemDropCommand& cmd) const
    {
        Beavium::TPacketCGItemDrop2Beavium packet{};
        packet.header = 20; // HEADER_CG_ITEM_DROP2 legacy
        packet.window = cmd.window;
        packet.cell = cmd.cell;
        packet.gold = cmd.count;
        packet.count = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGItemDrop2Beavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGItemDrop2Beavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeItemPickUp(
        const Domain::ItemPickUpCommand& cmd) const
    {
        Beavium::TPacketCGItemPickupBeavium packet{};
        packet.header = Beavium::CG::ITEM_PICKUP; // 0x0E
        packet.itemVid = cmd.vid;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGItemPickupBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGItemPickupBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeSelectEmpire(
        const Domain::EmpireSelectCommand& cmd) const
    {
        Beavium::TPacketCGSelectEmpireBeavium packet{};
        packet.header = Beavium::CG::SELECT_EMPIRE; // 0x35
        packet.empireId = cmd.empireId;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGSelectEmpireBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGSelectEmpireBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeSelectCharacter(
        const Domain::CharacterSelectCommand& cmd) const
    {
        Beavium::TPacketCGCharSelectBeavium packet{};
        packet.header = Beavium::CG::CHAR_SELECT; // 0x06
        packet.slot = cmd.slotIndex;
        packet.charId = 0;
        packet.securityToken = 0;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGCharSelectBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGCharSelectBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeCreateCharacter(
        const Domain::CharacterCreateCommand& cmd) const
    {
        Beavium::TPacketCGCharacterCreateBeavium packet{};
        packet.header = Beavium::CG::CHARACTER_CREATE; // 0x04
        packet.slot = cmd.slotIndex;
        std::memset(packet.name, 0, sizeof(packet.name));
        if (!cmd.name.empty())
        {
            size_t copyLen = std::min(cmd.name.size(), sizeof(packet.name) - 1);
            std::memcpy(packet.name, cmd.name.data(), copyLen);
        }
        packet.job = cmd.job;
        packet.shape = cmd.shape;
        packet.str = cmd.str;
        packet.dex = cmd.dex;
        packet.intelligence = cmd.int_;
        packet.con = cmd.con;

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGCharacterCreateBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGCharacterCreateBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeDestroyCharacter(
        const Domain::CharacterDestroyCommand& cmd) const
    {
        Beavium::TPacketCGCharacterDestroyBeavium packet{};
        packet.header = Beavium::CG::CHARACTER_DESTROY; // 0x05
        packet.slot = cmd.slotIndex;
        std::memset(packet.privateCode, 0, sizeof(packet.privateCode));
        if (!cmd.privateCode.empty())
        {
            size_t copyLen = std::min(cmd.privateCode.size(), sizeof(packet.privateCode));
            std::memcpy(packet.privateCode, cmd.privateCode.data(), copyLen);
        }

        std::vector<uint8_t> buffer(sizeof(Beavium::TPacketCGCharacterDestroyBeavium));
        std::memcpy(buffer.data(), &packet, sizeof(Beavium::TPacketCGCharacterDestroyBeavium));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeEnterGame() const
    {
        uint8_t header = Beavium::CG::ENTERGAME; // 0x0A (1B)
        return std::vector<uint8_t>{ header };
    }

    bool BeaviumProtocolDriver::IsHandshakeOpcode(uint16_t opcode) const noexcept
    {
        return opcode == Beavium::GC::TIME_SYNC ||            // 0xFF (13B greeting)
               opcode == Beavium::GC::KEY_AGREEMENT_OFFER ||   // 0xFB (137B offer)
               opcode == Beavium::GC::KEY_AGREEMENT_ACK;       // 0xFA (4B ack)
    }

    bool BeaviumProtocolDriver::HandleHandshakePacket(
        uint16_t header,
        std::span<const uint8_t> payload,
        CNetworkStream& stream)
    {
        switch (header)
        {
        case Beavium::GC::TIME_SYNC: // 0xFF (13B greeting)
        {
            if (payload.size() < sizeof(Beavium::TPacketGCHandshakeBeavium))
                return false;

            const auto& hs = *reinterpret_cast<const Beavium::TPacketGCHandshakeBeavium*>(payload.data());
            ELTimer_SetServerMSec(hs.srvTime);
            CTimer::Instance().SetBaseTime();

            Beavium::TPacketCGHandshakeAckBeavium ack{};
            ack.header = Beavium::CG::HANDSHAKE_ACK; // 0xFF
            ack.token = hs.token;
            Beavium::CalculateHandshakeSignatures(hs.token, hs.srvTime, ack.sig1, ack.sig2);

            if (!stream.Send(sizeof(ack), &ack))
                return false;

            return true;
        }

        case Beavium::GC::KEY_AGREEMENT_OFFER: // 0xFB (137B offer)
        {
            if (payload.size() < 137)
                return false;

            const uint8_t* serverPub = &payload[1];

            auto sodiumProvider = std::make_shared<SodiumCryptoProvider>();
            if (!sodiumProvider->GetCipher().Initialize())
                return false;

            uint8_t clientPub[32]{};
            sodiumProvider->GetCipher().GetPublicKey(clientPub);

            if (!sodiumProvider->GetCipher().ComputeClientKeys(serverPub))
                return false;

            uint8_t ackBuf[137]{};
            ackBuf[0] = Beavium::CG::KEY_AGREEMENT; // 0xFA
            std::memcpy(&ackBuf[1], clientPub, 32);
            std::memcpy(&ackBuf[33], &payload[33], 32);

            if (!stream.Send(sizeof(ackBuf), ackBuf))
                return false;

            stream.SetCryptoProvider(sodiumProvider);
            return true;
        }

        case Beavium::GC::KEY_AGREEMENT_ACK: // 0xFA (4B ack)
        {
            auto crypto = stream.GetCryptoProvider();
            if (crypto)
            {
                crypto->SetActivated(true);
                stream.DecryptPendingRecvData();
            }
            return true;
        }

        default:
            break;
        }

        return false;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeLogin(
        const std::string& username,
        uint32_t loginKey) const
    {
        Beavium::TPacketCGLogin2Beavium pkt{};
        pkt.header = Beavium::CG::LOGIN2; // 0x68
        strncpy_s(pkt.username, sizeof(pkt.username), username.c_str(), _TRUNCATE);
        pkt.loginKey = loginKey;
        std::memcpy(pkt.clientKey, Beavium::DefaultClientKey, 16);

        std::vector<uint8_t> out(sizeof(pkt));
        std::memcpy(out.data(), &pkt, sizeof(pkt));
        return out;
    }

    EterBase::PacketResult<std::vector<uint8_t>> BeaviumProtocolDriver::EncodeAuthLogin(
        const std::string& username,
        const std::string& password,
        const std::string& /*pin*/) const
    {
        TPacketCGLogin3 pkt{};
        pkt.header = 0x6F; // CG::LOGIN3
        pkt.length = sizeof(pkt);
        strncpy_s(pkt.name, sizeof(pkt.name), username.c_str(), _TRUNCATE);
        strncpy_s(pkt.pwd, sizeof(pkt.pwd), password.c_str(), _TRUNCATE);

        std::vector<uint8_t> out(sizeof(pkt));
        std::memcpy(out.data(), &pkt, sizeof(pkt));
        return out;
    }

    bool BeaviumProtocolDriver::DispatchInbound(
        uint16_t unifiedOpcode,
        std::span<const uint8_t> payload,
        UserInterface::Contracts::IGameEventSink* pSink)
    {
        if (payload.empty())
        {
            return false;
        }

        uint8_t bHeader = static_cast<uint8_t>(unifiedOpcode & 0xFF);

        // 1. Publikacja ogolnego zdarzenia odbioru ramki w magistrali EventBus
        ::EterBase::EventBus::GetInstance().Publish(
            ::EterBase::NetworkPacketReceivedEvent(bHeader, payload));

        // 2. Mapowanie kodow GC Beavium na dedykowane zdarzenia domenowe
        static BeaviumProtocolProfile s_beaviumProfile;
        if (UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().GetProtocolProfile() == nullptr)
        {
            UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().SetProtocolProfile(&s_beaviumProfile);
        }

        switch (bHeader)
        {
        case Beavium::GC::ATTACK: // 0x0C
            if (payload.size() >= sizeof(Beavium::TPacketGCAttackBeavium))
            {
                const auto& atk = *reinterpret_cast<const Beavium::TPacketGCAttackBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnAttackExecuted({
                        .dwAttackerVID = atk.dwVID,
                        .dwVictimVID = atk.dwVictimVID,
                        .byMotionType = atk.bType
                    });
                }
            }
            return true;

        case Beavium::GC::DEAD: // 0x0E
            if (payload.size() >= sizeof(Beavium::TPacketGCDeadBeavium))
            {
                const auto& dead = *reinterpret_cast<const Beavium::TPacketGCDeadBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorDead({
                        .dwVID = dead.dwVID
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::ActorDeadEvent(dead.dwVID));
            }
            return true;

        case Beavium::GC::CHARACTER_MOVE: // 0x03
            if (payload.size() >= sizeof(Beavium::TPacketGCCharacterMoveBeavium))
            {
                const auto& mv = *reinterpret_cast<const Beavium::TPacketGCCharacterMoveBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorMoved({
                        .dwVID = mv.dwVID,
                        .lX = mv.lX,
                        .lY = mv.lY,
                        .fRot = static_cast<float>(static_cast<double>(mv.dwRot) / 1000000.0),
                        .dwTime = mv.dwTime
                    });
                }
            }
            return true;

        case Beavium::GC::TARGET_HP: // 0x6C
            if (payload.size() >= sizeof(Beavium::TPacketGCTargetHPBeavium))
            {
                const auto& hp = *reinterpret_cast<const Beavium::TPacketGCTargetHPBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnTargetChanged({
                        .dwVID = hp.dwTargetVID
                    });
                }
            }
            return true;

        case 0x10: // GC::ITEM_SET / CHARACTER_UPDATE (82B)
            if (payload.size() >= sizeof(Beavium::TPacketGCItemSetBeavium))
            {
                const auto& item = *reinterpret_cast<const Beavium::TPacketGCItemSetBeavium*>(payload.data());
                if (pSink)
                {
                    pSink->OnItemReceived({
                        .dwCell = item.Cell,
                        .dwVnum = item.vnum,
                        .dwCount = item.count
                    });
                }
            }
            return true;

        default:
            break;
        }

        // 3. Delegacja do dyspozytora pakietow PhaseGamePacketDispatcher z walidacja dlugosci bufora
        return UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
            bHeader, payload.data(), payload.size());
    }
}
