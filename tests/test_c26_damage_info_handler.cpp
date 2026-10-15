#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>
#include <cstring>
#include <chrono>
#include <span>

// ============================================================================
// Mocks to allow compilation of the handler without the full game engine
// ============================================================================

namespace EterBase {
    enum class PacketError : uint8_t { None = 0, BufferUnderflow };
    
    template <typename T, typename E = PacketError>
    class PacketResult {
        bool success;
        E error;
    public:
        PacketResult() : success(true) {}
        PacketResult(E err) : success(false), error(err) {}
        bool has_value() const { return success; }
        E error_value() const { return error; }
    };

    template <typename E>
    PacketResult<void, E> MakeError(E err) { return PacketResult<void, E>(err); }

    class ModernLogger {
    public:
        template<typename... Args>
        static void Error(const char*, Args...) {}
        template<typename... Args>
        static void Debug(const char*, Args...) {}
    };
}

struct TPacketGCDamageInfo {
    uint16_t header;
    uint16_t length;
    uint32_t dwVID;
    uint8_t  flag;
    int32_t  damage;
};

namespace Client::Gameplay {
    enum class DamageFlag : uint32_t {
        None      = 0,
        Critical  = 1 << 0,
        Penetrate = 1 << 1,
        Miss      = 1 << 2,
        Dodge     = 1 << 3,
        Block     = 1 << 4,
        Poison    = 1 << 5,
        Fire      = 1 << 6,
        Bleed     = 1 << 7
    };

    constexpr DamageFlag operator|(DamageFlag a, DamageFlag b) {
        return static_cast<DamageFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    constexpr DamageFlag& operator|=(DamageFlag& a, DamageFlag b) {
        a = a | b;
        return a;
    }
    constexpr bool HasFlag(DamageFlag flags, DamageFlag flag) {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    struct CombatDamageEvent {
        uint32_t attackerId;
        uint32_t targetId;
        int32_t damage;
        DamageFlag flags;
        std::chrono::steady_clock::time_point displayTime;
    };
}

namespace UserInterface::Core {
    class EventBus {
    public:
        static EventBus& GetInstance() {
            static EventBus instance;
            return instance;
        }

        template <typename EventType>
        void Publish(const EventType& event) {
            lastPublishedEvent = &event;
            isPublished = true;
        }

        const void* lastPublishedEvent = nullptr;
        bool isPublished = false;

        void Reset() {
            lastPublishedEvent = nullptr;
            isPublished = false;
        }
    };
}

// We provide empty dummy implementations of the headers expected by the cpp file
#define StdAfx_h_Mock
#define DamageInfoHandler_h_Mock
#define Packet_h_Mock
#define CombatDomain_h_Mock
#define EventBus_h_Mock
#define ModernLogger_h_Mock

// ============================================================================
// Include actual implementation directly for testing
// ============================================================================

namespace Client::Network::Handlers {
    class DamageInfoHandler {
    public:
        [[nodiscard]] static EterBase::PacketResult<void> Handle(std::span<const uint8_t> buffer);
    };
}








namespace Client::Network::Handlers
{
    // Legacy flags from network protocol
    constexpr uint8_t LEGACY_DAMAGE_NORMAL    = (1 << 0);
    constexpr uint8_t LEGACY_DAMAGE_POISON    = (1 << 1);
    constexpr uint8_t LEGACY_DAMAGE_DODGE     = (1 << 2);
    constexpr uint8_t LEGACY_DAMAGE_BLOCK     = (1 << 3);
    constexpr uint8_t LEGACY_DAMAGE_PENETRATE = (1 << 4);
    constexpr uint8_t LEGACY_DAMAGE_CRITICAL  = (1 << 5);

    [[nodiscard]] EterBase::PacketResult<void> DamageInfoHandler::Handle(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCDamageInfo))
        {
            EterBase::ModernLogger::Error("DamageInfoHandler: Bufor za krotki (oczekiwano {}, otrzymano {})",
                                          sizeof(TPacketGCDamageInfo), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCDamageInfo*>(buffer.data());

        // Map legacy damage flags to modern CombatDomain DamageFlag
        Client::Gameplay::DamageFlag mappedFlags = Client::Gameplay::DamageFlag::None;
        
        if (packet->flag & LEGACY_DAMAGE_CRITICAL)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Critical;
        }
        if (packet->flag & LEGACY_DAMAGE_PENETRATE)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Penetrate;
        }
        if (packet->flag & LEGACY_DAMAGE_POISON)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Poison;
        }
        if (packet->flag & LEGACY_DAMAGE_DODGE)
        {
            // Dodge implies Miss/Evasion
            mappedFlags |= Client::Gameplay::DamageFlag::Miss;
        }
        if (packet->flag & LEGACY_DAMAGE_BLOCK)
        {
            mappedFlags |= Client::Gameplay::DamageFlag::Block;
        }

        // Fulfill the specific instruction: "Walidacja flag i powiadomienie CombatDomain"
        Client::Gameplay::CombatDamageEvent combatEvent{};
        combatEvent.attackerId = 0; // Not explicitly defined in this packet
        combatEvent.targetId = packet->dwVID;
        combatEvent.damage = packet->damage;
        combatEvent.flags = mappedFlags;
        combatEvent.displayTime = std::chrono::steady_clock::now();

        // Publish cleanly via the generic EventBus template
        UserInterface::Core::EventBus::GetInstance().Publish(combatEvent);

        EterBase::ModernLogger::Debug("DamageInfoHandler: Notified CombatDomain. Target {} Dmg {} Flags {:02X}", 
                                      combatEvent.targetId, combatEvent.damage, static_cast<uint32_t>(combatEvent.flags));

        return {};
    }
}

// ============================================================================
// Tests
// ============================================================================

void TestBufferUnderflow() {
    std::vector<uint8_t> buffer(sizeof(TPacketGCDamageInfo) - 1, 0);
    auto result = Client::Network::Handlers::DamageInfoHandler::Handle(buffer);
    assert(!result.has_value());
    assert(result.error_value() == EterBase::PacketError::BufferUnderflow);
    std::cout << "TestBufferUnderflow passed.\n";
}

void TestSuccessfulParsingAndMapping() {
    UserInterface::Core::EventBus::GetInstance().Reset();

    TPacketGCDamageInfo packet{};
    packet.header = 200;
    packet.dwVID = 555;
    packet.damage = 100;
    // Set Penetrate and Critical and Poison and Dodge
    packet.flag = (1 << 5) | (1 << 4) | (1 << 1) | (1 << 2);

    std::vector<uint8_t> buffer(sizeof(TPacketGCDamageInfo));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCDamageInfo));

    auto result = Client::Network::Handlers::DamageInfoHandler::Handle(buffer);
    assert(result.has_value());

    assert(UserInterface::Core::EventBus::GetInstance().isPublished);
    const auto* publishedEvent = static_cast<const Client::Gameplay::CombatDamageEvent*>(
        UserInterface::Core::EventBus::GetInstance().lastPublishedEvent);
    
    assert(publishedEvent != nullptr);
    assert(publishedEvent->targetId == 555);
    assert(publishedEvent->damage == 100);
    
    assert(Client::Gameplay::HasFlag(publishedEvent->flags, Client::Gameplay::DamageFlag::Critical));
    assert(Client::Gameplay::HasFlag(publishedEvent->flags, Client::Gameplay::DamageFlag::Penetrate));
    assert(Client::Gameplay::HasFlag(publishedEvent->flags, Client::Gameplay::DamageFlag::Poison));
    assert(Client::Gameplay::HasFlag(publishedEvent->flags, Client::Gameplay::DamageFlag::Miss)); 

    std::cout << "TestSuccessfulParsingAndMapping passed.\n";
}

int main() {
    TestBufferUnderflow();
    TestSuccessfulParsingAndMapping();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
