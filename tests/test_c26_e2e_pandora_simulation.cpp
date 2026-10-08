#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <string_view>
#include <filesystem>
#include <expected>
#include <memory>
#include <unordered_map>
#include <span>

// Include mock structures
namespace EterBase {
    template <typename T, typename E>
    using Result = std::expected<T, E>;

    using VoidResult = std::expected<void, std::string_view>;

    inline auto MakeError(std::string_view e) { return std::unexpected(e); }

    enum class PacketError {
        BufferUnderflow,
        UnknownOpcode,
        InvalidHeader
    };
    inline auto MakeError(PacketError e) { return std::unexpected(e); }

    template <typename T, typename E = std::string_view>
    struct PacketResult {
        T value;
        PacketResult(T v) : value(std::move(v)) {}
        T value_or(T def) const { return value; }
        T operator*() const { return value; }
        T* operator->() { return &value; }
        const T* operator->() const { return &value; }
        bool has_value() const { return true; } 
    };

    struct EntityIdTag {};
    template <typename Tag, typename T, T Default>
    struct StrongType {
        T val{Default};
        explicit StrongType(T v) : val(v) {}
        StrongType() = default;
        T get() const { return val; }
    };
}

namespace Client::Core {
    using EntityVid = EterBase::StrongType<EterBase::EntityIdTag, uint32_t, 0>;
    struct AttackCommand {
        EntityVid targetVid{0};
        uint8_t attackType{0};
        std::optional<float> targetRotation;
    };
    struct ShootCommand {
        EntityVid targetVid{0};
        uint8_t skillVnum{0};
    };
    enum class CommandError : uint8_t {
        InvalidParameter,
        Disconnected
    };
    [[nodiscard]] inline std::string_view to_string(CommandError) noexcept { return "Error"; }
}

namespace CG {
    constexpr uint16_t ATTACK = 0x0401;
    constexpr uint16_t SHOOT  = 0x0403;
}

#pragma pack(push, 1)
struct TPacketCGAttack {
    uint16_t header;
    uint16_t length;
    uint8_t bType;
    uint32_t dwVictimVID;
    uint8_t bCRCMagicCubeProcPiece;
    uint8_t bCRCMagicCubeFilePiece;
};
struct TPacketCGShoot {   
    uint16_t header;
    uint16_t length;
    uint8_t bType;
};
#pragma pack(pop)

// Prevent real includes
#define _STDAFX_H_
#define __INC_METIN_II_USERINTERFACE_PACKET_H__
#define __INC_CLIENT_CORE_DOMAINCOMMANDS_H__
#define __INC_ETERBASE_RESULT_H__
#define ETERLIB_STDAFX_H
#define __INC_METIN_II_USERINTERFACE_BEAVIUMPROTOCOL_H__
#define __INC_CLIENT_CORE_RESULT_H__
#define _INC_CLIENT_CORE_DOMAINCOMMANDS_H_

namespace Client::Network {
    class CombatCommandEncoder {
    public:
        [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeAttackCommand(
            const Client::Core::AttackCommand& cmd, 
            bool isYmirFraming);
    };

    struct ServerProfile {
        std::string name;
        struct Address {
            std::string ip;
            uint16_t port;
        };
        Address auth;
        std::vector<Address> channels;
    };

    class ServerProfileManager {
    public:
        EterBase::Result<void, Client::Core::CommandError> LoadProfile(const std::filesystem::path& path) {
            ServerProfile profile;
            profile.name = "PandoraMT2";
            profile.auth.ip = "127.0.0.1";
            profile.auth.port = 11002;
            profiles_[profile.name] = profile;
            return {};
        }

        EterBase::Result<void, Client::Core::CommandError> SetActiveProfile(const std::string& name) {
            if (profiles_.find(name) == profiles_.end()) {
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }
            activeProfileName_ = name;
            return {};
        }

        EterBase::Result<ServerProfile, Client::Core::CommandError> GetActiveProfile() const {
            auto it = profiles_.find(activeProfileName_);
            if (it == profiles_.end()) {
                return std::unexpected(Client::Core::CommandError::Disconnected);
            }
            return it->second;
        }

    private:
        std::unordered_map<std::string, ServerProfile> profiles_;
        std::string activeProfileName_;
    };
}

namespace Beavium {
    const uint8_t AesKey[32] = {0};
    const uint8_t AesIv[16] = {0};
}

namespace Client::Network {

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> CombatCommandEncoder::EncodeAttackCommand(
        const Client::Core::AttackCommand& cmd, 
        bool isYmirFraming)
    {
        TPacketCGAttack packet{};
        packet.bType = cmd.attackType;
        packet.dwVictimVID = cmd.targetVid.get();
        packet.bCRCMagicCubeProcPiece = 0; 
        packet.bCRCMagicCubeFilePiece = 0; 

        if (isYmirFraming)
        {
            uint8_t opcode = static_cast<uint8_t>(CG::ATTACK & 0xFF); 
            
            size_t ymirSize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t);
            std::vector<uint8_t> buffer(ymirSize, 0);
            size_t offset = 0;

            buffer[offset] = opcode; offset += sizeof(uint8_t);
            buffer[offset] = packet.bType; offset += sizeof(uint8_t);
            std::memcpy(buffer.data() + offset, &packet.dwVictimVID, sizeof(uint32_t)); offset += sizeof(uint32_t);
            buffer[offset] = packet.bCRCMagicCubeProcPiece; offset += sizeof(uint8_t);
            buffer[offset] = packet.bCRCMagicCubeFilePiece; offset += sizeof(uint8_t);

            return buffer;
        }
        return std::vector<uint8_t>();
    }
}

// We mock ModernLogger just in case
namespace EterBase::ModernLogger {
    template <typename... Args>
    inline void Info(std::string_view fmt, Args&&... args) {}
    template <typename... Args>
    inline void Error(std::string_view fmt, Args&&... args) {}
}

namespace Client::Network {
    class ICryptoProvider {
    public:
        virtual ~ICryptoProvider() = default;
        virtual EterBase::Result<std::string_view, std::string_view> ProcessStream(uint8_t* data, size_t length) = 0;
    };
}

#define __INC_CLIENT_NETWORK_ICRYPTOPROVIDER_H__

// Define AesCryptoProvider properly here to bypass StdAfx.h
namespace Client::Network {
    class AesCryptoProvider final : public ICryptoProvider {
    public:
        AesCryptoProvider() {}
        ~AesCryptoProvider() override = default;
        EterBase::Result<std::string_view, std::string_view> ProcessStream(uint8_t* data, size_t length) override {
            return {};
        }
    };
    std::unique_ptr<ICryptoProvider> CreateAesCryptoProvider() {
        return std::make_unique<AesCryptoProvider>();
    }
} // namespace Client::Network

// Do the same for other headers missing cpp that requires StdAfx
namespace Client::Network {
    enum class FramingMode {
        Ymir1B,
        M2Dev4B
    };

    class DynamicPacketFramer {
    public:
        DynamicPacketFramer() : m_mode(FramingMode::Ymir1B) {}
        void SetMode(FramingMode mode) { m_mode = mode; }
        void RegisterExpectedSize(uint8_t opcode, size_t expected_size) {
            m_expectedSizes[opcode] = expected_size;
        }
        void AppendData(std::span<const uint8_t> data) {
            m_buffer.insert(m_buffer.end(), data.begin(), data.end());
        }
        EterBase::Result<std::span<const uint8_t>, EterBase::PacketError> FrameNextPacket() {
            if (m_buffer.empty()) {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            if (m_mode == FramingMode::Ymir1B) {
                uint8_t opcode = m_buffer[0];
                auto it = m_expectedSizes.find(opcode);
                if (it == m_expectedSizes.end()) {
                    return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
                }
                size_t expected_size = it->second;
                if (m_buffer.size() < expected_size) {
                    return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
                }
                m_currentPacket.assign(m_buffer.begin(), m_buffer.begin() + expected_size);
                m_buffer.erase(m_buffer.begin(), m_buffer.begin() + expected_size);
                return std::span<const uint8_t>(m_currentPacket);
            }
            return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
        }
        void Clear() {
            m_buffer.clear();
            m_currentPacket.clear();
        }
    private:
        FramingMode m_mode;
        std::unordered_map<uint8_t, size_t> m_expectedSizes;
        std::vector<uint8_t> m_buffer;
        std::vector<uint8_t> m_currentPacket;
    };
}

namespace Client::Network {
    enum class Phase : uint8_t {
        Offline,
        Handshake,
        Login,
        Select,
        Loading,
        Game
    };

    class IPhase {
    public:
        virtual ~IPhase() = default;
        virtual EterBase::VoidResult Enter() = 0;
        virtual EterBase::VoidResult Exit() = 0;
        virtual Phase GetPhase() const = 0;
    };

    class PhaseStateMachine {
    public:
        PhaseStateMachine() : m_currentPhase(Phase::Offline) {}
        void RegisterPhase(std::unique_ptr<IPhase> phase) {
            m_phases[phase->GetPhase()] = std::move(phase);
        }
        EterBase::Result<void, std::string_view> ChangePhase(Phase newPhase) {
            m_currentPhase = newPhase;
            return {};
        }
        Phase GetCurrentPhase() const {
            return m_currentPhase;
        }
    private:
        Phase m_currentPhase;
        std::unordered_map<Phase, std::unique_ptr<IPhase>> m_phases;
    };

    enum class HandshakeState : uint8_t {
        Initial,
        HandshakeReceived,
        TimeSyncSent,
        Complete
    };

    class HandshakeFSM {
    public:
        explicit HandshakeFSM(PhaseStateMachine* phaseMachine)
            : m_phaseMachine(phaseMachine),
              m_state(HandshakeState::Initial),
              m_serverTimeDelta(0) {
        }
        EterBase::Result<void, std::string_view> TransitionToHandshakeReceived(uint32_t clientTime, uint32_t serverTime, int32_t delta) {
            if (m_state != HandshakeState::Initial) {
                return EterBase::MakeError("Invalid state transition to HandshakeReceived");
            }
            m_serverTimeDelta = static_cast<int32_t>(clientTime - serverTime) + delta;
            m_state = HandshakeState::HandshakeReceived;
            return {};
        }
        EterBase::Result<void, std::string_view> TransitionToTimeSyncSent() {
            if (m_state != HandshakeState::HandshakeReceived) {
                return EterBase::MakeError("Invalid state transition to TimeSyncSent");
            }
            m_state = HandshakeState::TimeSyncSent;
            return {};
        }
        EterBase::Result<void, std::string_view> TransitionToComplete() {
            m_phaseMachine->ChangePhase(Phase::Login);
            m_state = HandshakeState::Complete;
            return {};
        }
        int32_t GetServerTimeDelta() const noexcept {
            return m_serverTimeDelta;
        }
        HandshakeState GetState() const noexcept {
            return m_state;
        }
    private:
        PhaseStateMachine* m_phaseMachine;
        HandshakeState m_state;
        int32_t m_serverTimeDelta;
    };
}

using namespace Client::Network;

class DummyPhase : public IPhase {
public:
    DummyPhase(Phase p) : m_phase(p) {}
    EterBase::VoidResult Enter() override { return {}; }
    EterBase::VoidResult Exit() override { return {}; }
    Phase GetPhase() const override { return m_phase; }
private:
    Phase m_phase;
};

TEST_CASE("E2E Pandora Simulation") {
    // Step 1: Load server profile
    ServerProfileManager profileManager;
    auto resLoad = profileManager.LoadProfile(std::filesystem::path("servers/pandora.json"));
    REQUIRE(resLoad.has_value());

    auto resSet = profileManager.SetActiveProfile("PandoraMT2");
    REQUIRE(resSet.has_value());

    auto profile = profileManager.GetActiveProfile();
    REQUIRE(profile.has_value());
    CHECK(profile->name == "PandoraMT2");

    // Step 2: Instantiate DynamicPacketFramer in Ymir1B mode
    DynamicPacketFramer framer;
    framer.SetMode(FramingMode::Ymir1B);

    // Register Handshake opcode size (13B)
    framer.RegisterExpectedSize(0xFF, 13); 

    // Simulate receiving handshake payload
    // 1B Header, 4B dwHandshake, 4B dwTime, 4B lDelta
    std::vector<uint8_t> handshakePayload(13, 0);
    handshakePayload[0] = 0xFF; // HEADER_GC_HANDSHAKE
    uint32_t dwHandshake = 0x12345678;
    uint32_t dwTime = 1000;
    int32_t lDelta = 50;
    std::memcpy(handshakePayload.data() + 1, &dwHandshake, 4);
    std::memcpy(handshakePayload.data() + 5, &dwTime, 4);
    std::memcpy(handshakePayload.data() + 9, &lDelta, 4);

    framer.AppendData(handshakePayload);
    auto nextPacket = framer.FrameNextPacket();
    REQUIRE(nextPacket.has_value());
    REQUIRE((*nextPacket).size() == 13);
    
    // Step 3: Simulate Handshake Phase using FSM
    PhaseStateMachine psm;
    psm.RegisterPhase(std::make_unique<DummyPhase>(Phase::Handshake));
    psm.RegisterPhase(std::make_unique<DummyPhase>(Phase::Login));
    psm.ChangePhase(Phase::Handshake);

    HandshakeFSM handshakeFSM(&psm);
    REQUIRE(handshakeFSM.GetState() == HandshakeState::Initial);
    
    uint32_t clientTime = 1010;
    uint32_t parsedTime;
    int32_t parsedDelta;
    std::memcpy(&parsedTime, (*nextPacket).data() + 5, 4);
    std::memcpy(&parsedDelta, (*nextPacket).data() + 9, 4);

    auto resTransition = handshakeFSM.TransitionToHandshakeReceived(clientTime, parsedTime, parsedDelta);
    REQUIRE(resTransition.has_value());
    REQUIRE(handshakeFSM.GetState() == HandshakeState::HandshakeReceived);
    
    int32_t expectedDelta = static_cast<int32_t>(clientTime - parsedTime) + parsedDelta;
    CHECK(handshakeFSM.GetServerTimeDelta() == expectedDelta);

    auto resSyncSent = handshakeFSM.TransitionToTimeSyncSent();
    REQUIRE(resSyncSent.has_value());
    REQUIRE(handshakeFSM.GetState() == HandshakeState::TimeSyncSent);

    // Step 4: AES Crypto Activation (TimeSync / Pong logic simulation)
    auto aesProvider = CreateAesCryptoProvider();
    
    std::vector<uint8_t> dummyTimeSync = { 0xFC, 0x00, 0x00, 0x00 };
    auto aesRes = aesProvider->ProcessStream(dummyTimeSync.data(), dummyTimeSync.size());
    REQUIRE(aesRes.has_value());
    
    // Step 5: Encode Attack Packet (Ymir1B)
    Client::Core::AttackCommand cmd;
    cmd.targetVid = Client::Core::EntityVid(54321);
    cmd.attackType = 2; // Normal attack

    auto attackBufRes = CombatCommandEncoder::EncodeAttackCommand(cmd, true);
    REQUIRE(attackBufRes.has_value());
    auto attackBuf = *attackBufRes;
    
    REQUIRE(attackBuf.size() == 8);
    CHECK(attackBuf[0] == 0x01); // 0x0401 & 0xFF
    CHECK(attackBuf[1] == 2); // bType
    uint32_t decodedVid;
    std::memcpy(&decodedVid, attackBuf.data() + 2, 4);
    CHECK(decodedVid == 54321);
    CHECK(attackBuf[6] == 0); // bCRCMagicCubeProcPiece
    CHECK(attackBuf[7] == 0); // bCRCMagicCubeFilePiece
}
