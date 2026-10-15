#include "EterLib/StdAfx.h"
#include "LuriaProtocolDriver.h"
#include "LuriaProtocolProfile.h"
#include "../LuriaProtocol.h"
#include "UserInterface/Contracts/IGameEvents.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include "EterBase/EventBus.h"
#include "EterLib/NetStream.h"
#include "EterLib/BlowfishCryptoProvider.h"
#include <cstring>
#include <atomic>
#include <algorithm>

namespace Network::Protocol::Drivers
{
    std::optional<FrameHeaderInfo> LuriaProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagany co najmniej 1 bajt naglowka
        if (buffer.empty())
        {
            return std::nullopt;
        }

        uint8_t header = buffer[0];
        Luria::LuriaGCInfo info{};
        if (!Luria::GetGCInfo(header, info))
        {
            return std::nullopt;
        }

        if (!info.isDynamic)
        {
            // Dla pakietow stalych (isDynamic=false): zwraca info.size
            return FrameHeaderInfo{
                .unifiedOpcode = static_cast<uint16_t>(header),
                .packetLength = info.size,
                .headerSize = 1
            };
        }
        else
        {
            // Dla pakietow dynamicznych (isDynamic=true):
            if (header == 0xBE)
            {
                // Jesli 0xBE: 5 bajtow min, uint32_t dlugosc na offsetcie 1
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
            else
            {
                // Jesli pozostale (0x04 CHAT, 0x19 WHISPER, 0x25 SCRIPT itd.): 3 bajty min, uint16_t dlugosc na offsetcie 1
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

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        Luria::TPacketCGAttackLuria packet{};
        packet.header = Luria::CG::ATTACK; // 0x02
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid;

        static std::atomic<uint32_t> s_crcIndex{0};
        uint32_t idx = (cmd.sequence > 0)
            ? (static_cast<uint32_t>(cmd.sequence) * 2)
            : s_crcIndex.fetch_add(2);

        packet.bCRCMagicCubeProcPiece = Luria::CRC_XOR_TABLE[idx % 8];
        packet.bCRCMagicCubeFilePiece = Luria::CRC_XOR_TABLE[(idx + 1) % 8];

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGAttackLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGAttackLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        Luria::TPacketCGMoveLuria packet{};
        packet.header = Luria::CG::MOVE; // 0x03
        packet.bFunc = cmd.func;
        packet.bArg = static_cast<uint8_t>(cmd.arg);
        packet.fRot = cmd.rotationDegrees;
        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGMoveLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGMoveLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeItemUse(
        uint16_t cell, uint8_t windowType) const
    {
        Luria::TPacketCGItemUseLuria packet{};
        packet.header = Luria::CG::ITEM_USE; // 0x0B
        packet.window_type = windowType;
        packet.cell = cell;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGItemUseLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGItemUseLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeTarget(
        uint32_t targetVid) const
    {
        Luria::TPacketCGTargetLuria packet{};
        packet.header = Luria::CG::TARGET; // 0x3D
        packet.targetVid = targetVid;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGTargetLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGTargetLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeItemMove(
        const Domain::ItemMoveCommand& cmd) const
    {
        Luria::TPacketCGItemMoveLuria packet{};
        packet.header = Luria::CG::ITEM_MOVE; // 0x0D
        packet.srcWindow = cmd.srcWindow;
        packet.srcCell = cmd.srcCell;
        packet.dstWindow = cmd.dstWindow;
        packet.dstCell = cmd.dstCell;
        packet.count = cmd.count;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGItemMoveLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGItemMoveLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeItemDrop(
        const Domain::ItemDropCommand& cmd) const
    {
        Luria::TPacketCGItemDropLuria packet{};
        packet.header = Luria::CG::ITEM_DROP; // 0x0C
        packet.window_type = cmd.window;
        packet.cell = cmd.cell;
        packet.count = cmd.count;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGItemDropLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGItemDropLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeItemPickUp(
        const Domain::ItemPickUpCommand& cmd) const
    {
        Luria::TPacketCGItemPickUpLuria packet{};
        packet.header = Luria::CG::ITEM_PICKUP; // 0x0F
        packet.dwVID = cmd.vid;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGItemPickUpLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGItemPickUpLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeSelectEmpire(
        const Domain::EmpireSelectCommand& cmd) const
    {
        Luria::TPacketCGEmpireLuria packet{};
        packet.header = Luria::CG::EMPIRE; // 0x5A
        packet.empire = cmd.empireId;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGEmpireLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGEmpireLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeSelectCharacter(
        const Domain::CharacterSelectCommand& cmd) const
    {
        Luria::TPacketCGSelectCharacterLuria packet{};
        packet.header = Luria::CG::CHARACTER_SELECT; // 0x06
        packet.index = cmd.slotIndex;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGSelectCharacterLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGSelectCharacterLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeCreateCharacter(
        const Domain::CharacterCreateCommand& cmd) const
    {
        Luria::TPacketCGCreateCharacterLuria packet{};
        packet.header = Luria::CG::CHARACTER_CREATE; // 0x04
        packet.index = cmd.slotIndex;
        std::memset(packet.name, 0, sizeof(packet.name));
        if (!cmd.name.empty())
        {
            size_t copyLen = std::min(cmd.name.size(), sizeof(packet.name) - 1);
            std::memcpy(packet.name, cmd.name.data(), copyLen);
        }
        packet.job = cmd.job;
        packet.shape = cmd.shape;
        packet.CON = cmd.con;
        packet.INT = cmd.int_;
        packet.STR = cmd.str;
        packet.DEX = cmd.dex;

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGCreateCharacterLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGCreateCharacterLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeDestroyCharacter(
        const Domain::CharacterDestroyCommand& cmd) const
    {
        Luria::TPacketCGDestroyCharacterLuria packet{};
        packet.header = Luria::CG::CHARACTER_DELETE; // 0x05
        packet.index = cmd.slotIndex;
        std::memset(packet.szPrivateCode, 0, sizeof(packet.szPrivateCode));
        if (!cmd.privateCode.empty())
        {
            size_t copyLen = std::min(cmd.privateCode.size(), sizeof(packet.szPrivateCode) - 1);
            std::memcpy(packet.szPrivateCode, cmd.privateCode.data(), copyLen);
        }

        std::vector<uint8_t> buffer(sizeof(Luria::TPacketCGDestroyCharacterLuria));
        std::memcpy(buffer.data(), &packet, sizeof(Luria::TPacketCGDestroyCharacterLuria));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeEnterGame() const
    {
        // Luria EnterGame wymaga wyslania pakietu wersji klienta (67B) i bezposrednio po nim bajtu ENTERGAME (0x0A)
        Luria::TPacketCGClientVersionLuria ver{};
        ver.header = Luria::CG::CLIENT_VERSION; // 0xF1
        const char szDefName[] = "metin2client.exe";
        const char szDefTimestamp[] = "1215955205";
        strncpy_s(ver.szClientName, sizeof(ver.szClientName), szDefName, _TRUNCATE);
        strncpy_s(ver.szTimestamp, sizeof(ver.szTimestamp), szDefTimestamp, _TRUNCATE);

        std::vector<uint8_t> buffer(sizeof(ver) + 1);
        std::memcpy(buffer.data(), &ver, sizeof(ver));
        buffer[sizeof(ver)] = Luria::CG::ENTERGAME; // 0x0A
        return buffer;
    }

    bool LuriaProtocolDriver::IsHandshakeOpcode(uint16_t opcode) const noexcept
    {
        uint8_t bHeader = static_cast<uint8_t>(opcode & 0xFF);
        return bHeader == Luria::GC::HANDSHAKE          // 0x5B (91)
            || bHeader == Luria::GC::KEY_AGREEMENT      // 0xFC (252)
            || bHeader == Luria::GC::KEY_AGREEMENT_ACK; // 0xFA (250)
    }

    bool LuriaProtocolDriver::HandleHandshakePacket(
        uint16_t header,
        std::span<const uint8_t> payload,
        class CNetworkStream& stream)
    {
        uint8_t bHeader = static_cast<uint8_t>(header & 0xFF);
        switch (bHeader)
        {
        case Luria::GC::HANDSHAKE: // 0x5B (13B)
        {
            if (payload.size() < sizeof(Luria::TPacketCGHandshakeLuria))
                return false;

            const auto& hsPacket = *reinterpret_cast<const Luria::TPacketCGHandshakeLuria*>(payload.data());

            Luria::TPacketCGHandshakeLuria ackPacket{};
            ackPacket.header = Luria::CG::HANDSHAKE;
            ackPacket.dwHandshake = hsPacket.dwHandshake;
            ackPacket.dwTime = hsPacket.dwTime + hsPacket.lDelta * 2;
            ackPacket.lDelta = 0;

            if (!stream.Send(sizeof(ackPacket), &ackPacket))
                return false;

            return true;
        }

        case Luria::GC::KEY_AGREEMENT: // 0xFC (261B)
        {
            if (payload.size() < 261)
                return false;

            uint16_t wType = static_cast<uint16_t>(payload[1] | (static_cast<uint16_t>(payload[2]) << 8));
            uint16_t wLength = static_cast<uint16_t>(payload[3] | (static_cast<uint16_t>(payload[4]) << 8));
            if (wLength != 256)
                return false;

            const uint8_t* sPubData = &payload[5];
            Luria::LuriaDH2Keys keys{};
            if (!Luria::NegotiateDH2Keys(std::span<const uint8_t>(sPubData, 256), keys))
                return false;

            m_dh2Keys = keys;

            Luria::TPacketCGKeyAgreementLuria cgPkt{};
            cgPkt.header = Luria::CG::KEY_AGREEMENT;
            cgPkt.wType = wType;
            cgPkt.wLength = 256;
            std::memcpy(cgPkt.abData, keys.cgPubData.data(), 256);

            if (!stream.Send(sizeof(cgPkt), &cgPkt))
                return false;

            return true;
        }

        case Luria::GC::KEY_AGREEMENT_ACK: // 0xFA (4B ack)
        {
            if (payload.empty() || payload[0] != Luria::GC::KEY_AGREEMENT_ACK)
                return false;

            auto blowfish = std::make_shared<BlowfishCryptoProvider>();
            blowfish->SetKeys(m_dh2Keys.key2, m_dh2Keys.iv2,
                              m_dh2Keys.key1, m_dh2Keys.iv1);
            blowfish->SetActivated(true);
            stream.SetCryptoProvider(blowfish);
            stream.DecryptPendingRecvData();
            return true;
        }

        default:
            break;
        }

        return false;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeLogin(
        const std::string& username,
        uint32_t loginKey) const
    {
        Luria::TPacketCGLogin2Luria loginPacket{};
        loginPacket.header = Luria::CG::LOGIN2; // 0x6D
        strncpy_s(loginPacket.szLogin, sizeof(loginPacket.szLogin), username.c_str(), _TRUNCATE);
        loginPacket.dwLoginKey = loginKey;
        loginPacket.adwClientKeys[0] = 1;
        loginPacket.adwClientKeys[1] = 2;
        loginPacket.adwClientKeys[2] = 3;
        loginPacket.adwClientKeys[3] = 4;

        std::vector<uint8_t> out(sizeof(loginPacket));
        std::memcpy(out.data(), &loginPacket, sizeof(loginPacket));
        return out;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeAuthLogin(
        const std::string& username,
        const std::string& password,
        const std::string& pin) const
    {
        Luria::TPacketCGAuthLuria authPacket{};
        authPacket.header = Luria::CG::AUTH_LOGIN; // 0x6F
        strncpy_s(authPacket.szLogin, sizeof(authPacket.szLogin), username.c_str(), _TRUNCATE);
        strncpy_s(authPacket.szPassword, sizeof(authPacket.szPassword), password.c_str(), _TRUNCATE);
        if (!pin.empty())
        {
            strncpy_s(authPacket.szPin, sizeof(authPacket.szPin), pin.c_str(), _TRUNCATE);
        }
        strncpy_s(authPacket.szToken, sizeof(authPacket.szToken), "STARRS3543S22", _TRUNCATE);
        authPacket.adwClientKeys[0] = 1;
        authPacket.adwClientKeys[1] = 2;
        authPacket.adwClientKeys[2] = 3;
        authPacket.adwClientKeys[3] = 4;
        std::memcpy(authPacket.szHwidSha256, "da0cb75f0b7f841dd089e7af0e074add76c810e8f9742b892268157bbab0bc47", 64);
        authPacket.bNullTerminator = 0;

        std::vector<uint8_t> out(sizeof(authPacket));
        std::memcpy(out.data(), &authPacket, sizeof(authPacket));
        return out;
    }

    EterBase::PacketResult<std::vector<uint8_t>> LuriaProtocolDriver::EncodeClientVersion(
        const std::string& filename) const
    {
        Luria::TPacketCGClientVersionLuria kVersionPacket{};
        kVersionPacket.header = Luria::CG::CLIENT_VERSION; // 0xF1
        const char szDefName[] = "metin2client.exe";
        const char szDefTimestamp[] = "1215955205";
        strncpy_s(kVersionPacket.szClientName, sizeof(kVersionPacket.szClientName),
            filename.empty() ? szDefName : filename.c_str(), _TRUNCATE);
        strncpy_s(kVersionPacket.szTimestamp, sizeof(kVersionPacket.szTimestamp),
            szDefTimestamp, _TRUNCATE);

        std::vector<uint8_t> out(sizeof(kVersionPacket));
        std::memcpy(out.data(), &kVersionPacket, sizeof(kVersionPacket));
        return out;
    }

    bool LuriaProtocolDriver::DispatchInbound(
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

        // 2. Aktywacja profilu protokolu Luria w dyspozytorze pakietow
        static LuriaProtocolProfile s_luriaProfile;
        if (UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().GetProtocolProfile() == nullptr)
        {
            UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().SetProtocolProfile(&s_luriaProfile);
        }

        // 3. Mapowanie glownych pakietow GC na IGameEventSink oraz EventBus
        switch (bHeader)
        {
        case Luria::GC::CHARACTER_MOVE: // 0x03 (24B)
            if (payload.size() >= sizeof(Luria::TPacketGCMoveLuria))
            {
                const auto& mv = *reinterpret_cast<const Luria::TPacketGCMoveLuria*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorMoved({
                        .dwVID = mv.dwVID,
                        .lX = mv.lX,
                        .lY = mv.lY,
                        .fRot = static_cast<float>(mv.bRot * 5.0f),
                        .dwTime = mv.dwTime
                    });
                }
            }
            return true;

        case Luria::GC::DEAD: // 0x0E (5B)
            if (payload.size() >= sizeof(Luria::TPacketGCDeadLuria))
            {
                const auto& dead = *reinterpret_cast<const Luria::TPacketGCDeadLuria*>(payload.data());
                if (pSink)
                {
                    pSink->OnActorDead({
                        .dwVID = dead.dwVID
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::ActorDeadEvent(dead.dwVID));
            }
            return true;

        case Luria::GC::TARGET_HP: // 0x6C (13B lub 5B min)
        case 0x29:                 // RecvTarget (41 / 10B)
            if (payload.size() >= 5)
            {
                uint32_t targetVid = 0;
                std::memcpy(&targetVid, &payload[1], sizeof(uint32_t));
                if (pSink)
                {
                    pSink->OnTargetChanged({
                        .dwVID = targetVid
                    });
                }
                ::EterBase::EventBus::GetInstance().Publish(::EterBase::TargetBoardRefreshEvent(targetVid));
            }
            return true;

        case Luria::GC::DAMAGE_INFO: // 0x87 (10B)
            if (payload.size() >= sizeof(Luria::TPacketGCDamageInfoLuria))
            {
                const auto& dmg = *reinterpret_cast<const Luria::TPacketGCDamageInfoLuria*>(payload.data());
                if (pSink)
                {
                    pSink->OnDamageInfo({
                        .dwVictimVID = dmg.dwVID,
                        .dwAttackerVID = 0,
                        .dwDamage = static_cast<uint64_t>(dmg.damage > 0 ? dmg.damage : 0),
                        .byDamageFlag = dmg.flag
                    });
                }
            }
            return true;

        case Luria::GC::ATTACK: // 0x0C
            if (payload.size() >= 10)
            {
                uint32_t attackerVid = 0;
                uint32_t victimVid = 0;
                std::memcpy(&attackerVid, &payload[1], sizeof(uint32_t));
                std::memcpy(&victimVid, &payload[5], sizeof(uint32_t));
                uint8_t motionType = payload[9];
                if (pSink)
                {
                    pSink->OnAttackExecuted({
                        .dwAttackerVID = attackerVid,
                        .dwVictimVID = victimVid,
                        .byMotionType = motionType
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
