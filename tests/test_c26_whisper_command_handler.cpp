#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <span>

#include "../src/Client/Gameplay/WhisperCommandHandler.h"
#include "../src/Client/Core/INetworkPort.h"
#include "../src/Client/Core/DomainCommands.h"
#include "../src/Client/Core/Result.h"
#include "../src/EterBase/ChronoTimer.h"

// Mock for testing without requiring the full network stack
namespace Network::Senders {
    // Stub implementation to bypass SendWhisperPacket.cpp which requires StdAfx.h and TPacketCGWhisper (Windows headers)
    EterBase::PacketResult<void> SendWhisperPacket(
        std::string_view targetName,
        std::string_view message,
        const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (targetName.empty() || targetName.length() > 64) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }
        if (message.empty() || message.length() >= 255) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        std::vector<uint8_t> dummyData;
        dummyData.push_back(0x06);
        dummyData.push_back(0x02);
        if (!sendCallback(dummyData)) {
            return std::unexpected(EterBase::PacketError::SessionClosed);
        }
        return {};
    }
}

class MockNetworkPort : public Client::Core::INetworkPort {
public:
    bool m_called = false;
    uint8_t m_lastOpcode = 0;

    [[nodiscard]] Client::Core::Result<void, Client::Core::PacketError> SendRaw(uint8_t opcode, std::span<const uint8_t> payload) override {
        m_called = true;
        m_lastOpcode = opcode;
        return {};
    }
    
    [[nodiscard]] bool IsConnected() const noexcept override {
        return true;
    }
};

void TestNullNetworkPort() {
    Client::Gameplay::WhisperCommandHandler handler;
    ChronoTimer timer;
    Client::Core::WhisperCommand cmd{"Player1", "Hello"};

    auto result = handler.Execute(cmd, nullptr, timer);
    assert(!result.has_value());
    assert(result.error() == Client::Core::CommandError::Disconnected);
    std::cout << "TestNullNetworkPort passed.\n";
}

void TestEmptyParameters() {
    Client::Gameplay::WhisperCommandHandler handler;
    ChronoTimer timer;
    auto port = std::make_shared<MockNetworkPort>();

    Client::Core::WhisperCommand cmd1{"", "Hello"};
    auto res1 = handler.Execute(cmd1, port, timer);
    assert(!res1.has_value() && res1.error() == Client::Core::CommandError::InvalidParameter);

    Client::Core::WhisperCommand cmd2{"Player1", ""};
    auto res2 = handler.Execute(cmd2, port, timer);
    assert(!res2.has_value() && res2.error() == Client::Core::CommandError::InvalidParameter);

    std::cout << "TestEmptyParameters passed.\n";
}

void TestRecipientLengthLimit() {
    Client::Gameplay::WhisperCommandHandler handler;
    ChronoTimer timer;
    auto port = std::make_shared<MockNetworkPort>();

    std::string longName(25, 'A'); // Max is 24
    Client::Core::WhisperCommand cmd{longName, "Hello"};
    auto result = handler.Execute(cmd, port, timer);
    assert(!result.has_value() && result.error() == Client::Core::CommandError::InvalidParameter);

    std::string validName(24, 'A');
    Client::Core::WhisperCommand cmdValid{validName, "Hello"};
    auto resultValid = handler.Execute(cmdValid, port, timer);
    assert(resultValid.has_value());
    
    std::cout << "TestRecipientLengthLimit passed.\n";
}

void TestAntiSpam() {
    Client::Gameplay::WhisperCommandHandler handler;
    ChronoTimer timer;
    auto port = std::make_shared<MockNetworkPort>();

    Client::Core::WhisperCommand cmd{"Player1", "Hello"};
    
    // First call should succeed
    auto res1 = handler.Execute(cmd, port, timer);
    assert(res1.has_value());

    // Second call immediately should fail with RateLimited
    auto res2 = handler.Execute(cmd, port, timer);
    assert(!res2.has_value());
    assert(res2.error() == Client::Core::CommandError::RateLimited);

    std::cout << "TestAntiSpam passed.\n";
}

void TestSuccessfulExecution() {
    Client::Gameplay::WhisperCommandHandler handler;
    ChronoTimer timer;
    auto port = std::make_shared<MockNetworkPort>();

    Client::Core::WhisperCommand cmd{"Player2", "Hello World"};
    auto result = handler.Execute(cmd, port, timer);
    
    assert(result.has_value());
    assert(port->m_called == true);
    assert(port->m_lastOpcode == 2);

    std::cout << "TestSuccessfulExecution passed.\n";
}

int main() {
    TestNullNetworkPort();
    TestEmptyParameters();
    TestRecipientLengthLimit();
    TestAntiSpam();
    TestSuccessfulExecution();
    std::cout << "All tests passed!\n";
    return 0;
}
