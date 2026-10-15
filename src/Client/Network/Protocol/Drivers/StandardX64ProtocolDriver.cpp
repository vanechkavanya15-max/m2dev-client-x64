#include "EterLib/StdAfx.h"
#include "StandardX64ProtocolDriver.h"
#include "../Protocol.h"
#include "EterLib/NetStream.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Network::Protocol::Drivers
{
    std::optional<FrameHeaderInfo> StandardX64ProtocolDriver::InspectFrame(
        std::span<const uint8_t> buffer) const noexcept
    {
        // Wymagane minimum 4 bajty: [header:2][length:2]
        if (buffer.size() < 4)
        {
            return std::nullopt;
        }

        uint16_t header = static_cast<uint16_t>(buffer[0] | (static_cast<uint16_t>(buffer[1]) << 8));
        uint32_t length = static_cast<uint32_t>(buffer[2] | (static_cast<uint32_t>(buffer[3]) << 8));

        // Dlugosc musi zawierac sie w przedziale [4, 65000]
        if (length < 4 || length > 65000)
        {
            return std::nullopt;
        }

        return FrameHeaderInfo{
            .unifiedOpcode = header,
            .packetLength = length,
            .headerSize = 4
        };
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeAttack(
        const Domain::AttackCommand& cmd) const
    {
        TPacketCGAttack packet{};
        packet.header = 0x0401; // Standardowy naglowek CG::ATTACK x64
        packet.length = sizeof(TPacketCGAttack);
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid;
        packet.bCRCMagicCubeProcPiece = 0;
        packet.bCRCMagicCubeFilePiece = 0;

        std::vector<uint8_t> buffer(sizeof(TPacketCGAttack));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGAttack));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeMove(
        const Domain::MoveCommand& cmd) const
    {
        TPacketCGMove packet{};
        packet.header = 0x0301; // Standardowy naglowek CG::MOVE x64
        packet.length = sizeof(TPacketCGMove);
        packet.bFunc = cmd.func;
        packet.bArg = static_cast<uint8_t>(cmd.arg);

        float rot = std::fmod(cmd.rotationDegrees, 360.0f);
        if (rot < 0.0f)
        {
            rot += 360.0f;
        }
        packet.bRot = static_cast<uint8_t>(rot / 5.0f); // Segmenty rotacji w stopniach (0..72)

        packet.lX = cmd.x;
        packet.lY = cmd.y;
        packet.dwTime = cmd.time;

        std::vector<uint8_t> buffer(sizeof(TPacketCGMove));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGMove));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeItemUse(
        uint16_t cell, uint8_t windowType) const
    {
        TPacketCGItemUse packet{};
        packet.header = 0x0501; // Standardowy naglowek CG::ITEM_USE x64
        packet.length = sizeof(TPacketCGItemUse);
        packet.pos = TItemPos(windowType, cell);

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemUse));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemUse));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeTarget(
        uint32_t targetVid) const
    {
        TPacketCGTarget packet{};
        packet.header = 0x0A01; // Standardowy naglowek CG::TARGET x64
        packet.length = sizeof(TPacketCGTarget);
        packet.dwVID = targetVid;

        std::vector<uint8_t> buffer(sizeof(TPacketCGTarget));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGTarget));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeItemMove(
        const Domain::ItemMoveCommand& cmd) const
    {
        TPacketCGItemMove packet{};
        packet.header = Client::Network::Protocol::CG::ITEM_MOVE; // 0x0504
        packet.length = sizeof(TPacketCGItemMove);
        packet.pos = TItemPos(cmd.srcWindow, cmd.srcCell);
        packet.change_pos = TItemPos(cmd.dstWindow, cmd.dstCell);
        packet.num = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemMove));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemMove));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeItemDrop(
        const Domain::ItemDropCommand& cmd) const
    {
        TPacketCGItemDrop2 packet{};
        packet.header = Client::Network::Protocol::CG::ITEM_DROP2; // 0x0503
        packet.length = sizeof(TPacketCGItemDrop2);
        packet.pos = TItemPos(cmd.window, cmd.cell);
        packet.gold = cmd.count;
        packet.count = static_cast<uint8_t>(cmd.count);

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemDrop2));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemDrop2));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeItemPickUp(
        const Domain::ItemPickUpCommand& cmd) const
    {
        TPacketCGItemPickUp packet{};
        packet.header = Client::Network::Protocol::CG::ITEM_PICKUP; // 0x0505
        packet.length = sizeof(TPacketCGItemPickUp);
        packet.vid = cmd.vid;

        std::vector<uint8_t> buffer(sizeof(TPacketCGItemPickUp));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGItemPickUp));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeSelectEmpire(
        const Domain::EmpireSelectCommand& cmd) const
    {
        TPacketCGEmpire packet{};
        packet.header = Client::Network::Protocol::CG::EMPIRE; // 0x010A
        packet.length = sizeof(TPacketCGEmpire);
        packet.bEmpire = cmd.empireId;

        std::vector<uint8_t> buffer(sizeof(TPacketCGEmpire));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGEmpire));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeSelectCharacter(
        const Domain::CharacterSelectCommand& cmd) const
    {
        TPacketCGSelectCharacter packet{};
        packet.header = Client::Network::Protocol::CG::CHARACTER_SELECT; // 0x0203
        packet.length = sizeof(TPacketCGSelectCharacter);
        packet.player_index = cmd.slotIndex;

        std::vector<uint8_t> buffer(sizeof(TPacketCGSelectCharacter));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGSelectCharacter));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeCreateCharacter(
        const Domain::CharacterCreateCommand& cmd) const
    {
        TPacketCGCreateCharacter packet{};
        packet.header = Client::Network::Protocol::CG::CHARACTER_CREATE; // 0x0201
        packet.length = sizeof(TPacketCGCreateCharacter);
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

        std::vector<uint8_t> buffer(sizeof(TPacketCGCreateCharacter));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGCreateCharacter));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeDestroyCharacter(
        const Domain::CharacterDestroyCommand& cmd) const
    {
        TPacketCGDestroyCharacter packet{};
        packet.header = Client::Network::Protocol::CG::CHARACTER_DELETE; // 0x0202
        packet.length = sizeof(TPacketCGDestroyCharacter);
        packet.index = cmd.slotIndex;
        std::memset(packet.szPrivateCode, 0, sizeof(packet.szPrivateCode));
        if (!cmd.privateCode.empty())
        {
            size_t copyLen = std::min(cmd.privateCode.size(), sizeof(packet.szPrivateCode));
            std::memcpy(packet.szPrivateCode, cmd.privateCode.data(), copyLen);
        }

        std::vector<uint8_t> buffer(sizeof(TPacketCGDestroyCharacter));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGDestroyCharacter));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeEnterGame() const
    {
        TPacketCGEnterFrontGame packet{};
        packet.header = Client::Network::Protocol::CG::ENTERGAME; // 0x0204
        packet.length = sizeof(TPacketCGEnterFrontGame);

        std::vector<uint8_t> buffer(sizeof(TPacketCGEnterFrontGame));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGEnterFrontGame));
        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeLogin(
        const std::string& username,
        uint32_t loginKey) const
    {
        TPacketCGLogin2 pkt{};
        pkt.header = Client::Network::Protocol::CG::LOGIN2; // 0x0101
        pkt.length = sizeof(pkt);
        pkt.login_key = loginKey;
        strncpy_s(pkt.name, sizeof(pkt.name), username.c_str(), _TRUNCATE);

        std::vector<uint8_t> out(sizeof(pkt));
        std::memcpy(out.data(), &pkt, sizeof(pkt));
        return out;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeAuthLogin(
        const std::string& username,
        const std::string& password,
        const std::string& /*pin*/) const
    {
        TPacketCGLogin3 pkt{};
        pkt.header = Client::Network::Protocol::CG::LOGIN3; // 0x0102
        pkt.length = sizeof(pkt);
        strncpy_s(pkt.name, sizeof(pkt.name), username.c_str(), _TRUNCATE);
        strncpy_s(pkt.pwd, sizeof(pkt.pwd), password.c_str(), _TRUNCATE);

        std::vector<uint8_t> out(sizeof(pkt));
        std::memcpy(out.data(), &pkt, sizeof(pkt));
        return out;
    }

    EterBase::PacketResult<std::vector<uint8_t>> StandardX64ProtocolDriver::EncodeClientVersion(
        const std::string& filename) const
    {
        TPacketCGClientVersion pkt{};
        pkt.header = Client::Network::Protocol::CG::CLIENT_VERSION; // 0x000D
        pkt.length = sizeof(pkt);
        const char szDefName[] = "Metin2_Release.exe";
        const char szDefTimestamp[] = "1215955205";
        strncpy_s(pkt.filename, sizeof(pkt.filename), filename.empty() ? szDefName : filename.c_str(), _TRUNCATE);
        strncpy_s(pkt.timestamp, sizeof(pkt.timestamp), szDefTimestamp, _TRUNCATE);

        std::vector<uint8_t> out(sizeof(pkt));
        std::memcpy(out.data(), &pkt, sizeof(pkt));
        return out;
    }

    bool StandardX64ProtocolDriver::IsHandshakeOpcode(uint16_t opcode) const noexcept
    {
        return opcode == GC::KEY_CHALLENGE   // 0x000B
            || opcode == GC::KEY_COMPLETE;   // 0x000C
    }

    bool StandardX64ProtocolDriver::HandleHandshakePacket(
        uint16_t header,
        std::span<const uint8_t> payload,
        class CNetworkStream& stream)
    {
        switch (header)
        {
        case GC::KEY_CHALLENGE:
        {
            if (payload.size() < sizeof(TPacketGCKeyChallenge))
                return false;

            const auto& packet = *reinterpret_cast<const TPacketGCKeyChallenge*>(payload.data());
            SecureCipher& cipher = stream.GetSecureCipher();
            if (!cipher.Initialize())
            {
                stream.Disconnect();
                return false;
            }

            if (!cipher.ComputeClientKeys(packet.server_pk))
            {
                stream.Disconnect();
                return false;
            }

            TPacketCGKeyResponse response{};
            response.header = CG::KEY_RESPONSE;
            response.length = sizeof(response);
            cipher.GetPublicKey(response.client_pk);
            cipher.ComputeChallengeResponse(packet.challenge, response.challenge_response);

            if (!stream.Send(sizeof(response), &response))
                return false;

            return true;
        }

        case GC::KEY_COMPLETE:
        {
            if (payload.size() < sizeof(TPacketGCKeyComplete))
                return false;

            const auto& packet = *reinterpret_cast<const TPacketGCKeyComplete*>(payload.data());
            SecureCipher& cipher = stream.GetSecureCipher();

            uint8_t decrypted_token[SecureCipher::SESSION_TOKEN_SIZE];
            if (!cipher.DecryptToken(packet.encrypted_token, sizeof(packet.encrypted_token),
                                     packet.nonce, decrypted_token))
            {
                stream.Disconnect();
                return false;
            }

            cipher.SetSessionToken(decrypted_token);
            cipher.SetActivated(true);
            stream.DecryptPendingRecvData();
            return true;
        }

        default:
            break;
        }

        return false;
    }

    bool StandardX64ProtocolDriver::DispatchInbound(
        uint16_t unifiedOpcode,
        std::span<const uint8_t> payload,
        UserInterface::Contracts::IGameEventSink* /*pSink*/)
    {
        if (payload.empty())
        {
            return false;
        }

        // Delegacja do PhaseGamePacketDispatcher dla architektury x64
        return UserInterface::Network::Routers::PhaseGamePacketDispatcher::Instance().DispatchPacket(
            unifiedOpcode, payload.data(), payload.size());
    }
}
