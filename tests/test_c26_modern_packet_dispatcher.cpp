#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include "../src/Client/Network/ModernPacketDispatcher.h"
#include "../src/Client/Bridge/StranglerNetworkFacade.h"
#include "../src/Client/Network/Protocol/ProtocolOpcodes.h"
#include "../src/Client/Network/Protocol/Protocol.h"

using namespace Client::Network;
using namespace Client::Bridge;

class DummyStaticHandler : public IPacketHandler {
public:
    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override {
        called = true;
        return {};
    }
    [[nodiscard]] uint16_t GetExpectedSize() const override {
        return 10;
    }
    [[nodiscard]] bool IsDynamicSize() const override {
        return false;
    }
    
    bool called = false;
};

class DummyDynamicHandler : public IPacketHandler {
public:
    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override {
        called = true;
        return {};
    }
    [[nodiscard]] uint16_t GetExpectedSize() const override {
        return 4; // minimum header size
    }
    [[nodiscard]] bool IsDynamicSize() const override {
        return true;
    }
    
    bool called = false;
};

class ErrorHandler : public IPacketHandler {
public:
    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override {
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }
    [[nodiscard]] uint16_t GetExpectedSize() const override {
        return 2;
    }
    [[nodiscard]] bool IsDynamicSize() const override {
        return false;
    }
};

void test_unregistered_opcode() {
    ModernPacketDispatcher dispatcher;
    std::vector<uint8_t> payload = {1, 2, 3};
    auto result = dispatcher.Dispatch(50, payload);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::UnknownOpcode);
    std::cout << "test_unregistered_opcode passed\n";
}

void test_static_size_success() {
    ModernPacketDispatcher dispatcher;
    DummyStaticHandler handler;
    dispatcher.RegisterHandler(10, &handler);
    
    std::vector<uint8_t> payload(10, 0);
    auto result = dispatcher.Dispatch(10, payload);
    
    assert(result.has_value());
    assert(handler.called);
    std::cout << "test_static_size_success passed\n";
}

void test_static_size_underflow() {
    ModernPacketDispatcher dispatcher;
    DummyStaticHandler handler;
    dispatcher.RegisterHandler(10, &handler);
    
    std::vector<uint8_t> payload(9, 0); // one byte short
    auto result = dispatcher.Dispatch(10, payload);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
    assert(!handler.called);
    std::cout << "test_static_size_underflow passed\n";
}

void test_dynamic_size_success() {
    ModernPacketDispatcher dispatcher;
    DummyDynamicHandler handler;
    dispatcher.RegisterHandler(20, &handler);
    
    std::vector<uint8_t> payload(15, 0); // larger than min 4
    auto result = dispatcher.Dispatch(20, payload);
    
    assert(result.has_value());
    assert(handler.called);
    std::cout << "test_dynamic_size_success passed\n";
}

void test_dynamic_size_underflow() {
    ModernPacketDispatcher dispatcher;
    DummyDynamicHandler handler;
    dispatcher.RegisterHandler(20, &handler);
    
    std::vector<uint8_t> payload(3, 0); // smaller than min 4
    auto result = dispatcher.Dispatch(20, payload);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
    assert(!handler.called);
    std::cout << "test_dynamic_size_underflow passed\n";
}

void test_handler_error() {
    ModernPacketDispatcher dispatcher;
    ErrorHandler handler;
    dispatcher.RegisterHandler(30, &handler);
    
    std::vector<uint8_t> payload(5, 0);
    auto result = dispatcher.Dispatch(30, payload);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::MalformedPayload);
    std::cout << "test_handler_error passed\n";
}

void test_unregister_handler() {
    ModernPacketDispatcher dispatcher;
    DummyStaticHandler handler;
    dispatcher.RegisterHandler(10, &handler);
    dispatcher.UnregisterHandler(10);
    
    std::vector<uint8_t> payload(10, 0);
    auto result = dispatcher.Dispatch(10, payload);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::UnknownOpcode);
    std::cout << "test_unregister_handler passed\n";
}

void test_16bit_extended_opcodes() {
    ModernPacketDispatcher dispatcher;
    DummyStaticHandler handler;
    constexpr uint16_t EXT_OPCODE = 0x0410; // 16-bit GC::DAMAGE_INFO
    
    dispatcher.RegisterHandler(EXT_OPCODE, &handler);
    assert(dispatcher.HasHandler(EXT_OPCODE));
    assert(!dispatcher.HasHandler(0x0411));
    
    std::vector<uint8_t> payload(10, 0xAB);
    auto result = dispatcher.Dispatch(EXT_OPCODE, payload);
    assert(result.has_value());
    assert(handler.called);
    
    dispatcher.UnregisterHandler(EXT_OPCODE);
    assert(!dispatcher.HasHandler(EXT_OPCODE));
    std::cout << "test_16bit_extended_opcodes passed\n";
}

void test_functional_packet_handler() {
    ModernPacketDispatcher dispatcher;
    bool fnCalled = false;
    
    dispatcher.RegisterFunctionHandler(0x0516, 4, false, [&](std::span<const uint8_t> p) -> EterBase::PacketResult<void> {
        fnCalled = true;
        if (p[0] == 0xFF) {
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }
        return {};
    });
    
    assert(dispatcher.HasHandler(0x0516));
    
    std::vector<uint8_t> payloadOk = {0x01, 0x02, 0x03, 0x04};
    auto resOk = dispatcher.Dispatch(0x0516, payloadOk);
    assert(resOk.has_value());
    assert(fnCalled);
    
    std::vector<uint8_t> payloadBad = {0xFF, 0x00, 0x00, 0x00};
    auto resBad = dispatcher.Dispatch(0x0516, payloadBad);
    assert(!resBad.has_value());
    assert(resBad.error() == EterBase::PacketError::MalformedPayload);
    std::cout << "test_functional_packet_handler passed\n";
}

void test_strangler_network_facade() {
    auto& facade = StranglerNetworkFacade::Instance();
    DummyStaticHandler staticHandler;
    
    facade.RegisterHandler(0x42, &staticHandler);
    assert(facade.HasHandler(0x42));
    assert(facade.HasHandler(static_cast<uint16_t>(0x42)));
    
    std::vector<uint8_t> payload(10, 0);
    auto res = facade.DispatchPacket(0x42, payload);
    assert(res.has_value());
    assert(staticHandler.called);
    
    facade.UnregisterHandler(0x42);
    assert(!facade.HasHandler(0x42));
    std::cout << "test_strangler_network_facade passed\n";
}

void test_register_default_handlers() {
    auto& dispatcher = ModernPacketDispatcher::Instance();
    dispatcher.Clear();
    dispatcher.RegisterDefaultHandlers();
    
    // Test that default handlers are registered (both 16-bit and 8-bit opcodes)
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::DAMAGE_INFO));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x10)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::CHAT));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x03)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::WHISPER));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x04)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::ITEM_GROUND_DEL));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x14)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::ITEM_GROUND_ADD));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x13)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::TARGET));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x18)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::STUN));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x1B)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::WARP));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x08)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::MOTION));
    assert(dispatcher.HasHandler(static_cast<uint16_t>(0x07)));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::AFFECT_ADD));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::AFFECT_REMOVE));

    // Nowo zarejestrowane handlery domenowe (Zloty Srodek SRP)
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::PARTY_INVITE));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::PARTY_ADD));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::PARTY_UPDATE));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::PARTY_REMOVE));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::GUILD));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::SCRIPT));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::QUEST_INFO));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::QUEST_CONFIRM));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::EXCHANGE));
    assert(dispatcher.HasHandler(Client::Network::Protocol::GC::REFINE_INFORMATION));
    
    // Test dispatch of DAMAGE_INFO via zero-copy span
    TPacketGCDamageInfo dmgPacket{};
    dmgPacket.header = Client::Network::Protocol::GC::DAMAGE_INFO;
    dmgPacket.dwVID = 12345;
    dmgPacket.flag = 0x01; // NORMAL
    dmgPacket.damage = 999;
    
    std::span<const uint8_t> dmgSpan(reinterpret_cast<const uint8_t*>(&dmgPacket), sizeof(dmgPacket));
    auto resDmg = dispatcher.Dispatch(Client::Network::Protocol::GC::DAMAGE_INFO, dmgSpan);
    assert(resDmg.has_value());
    
    // Test dispatch of STUN packet
    TPacketGCStun stunPacket{};
    stunPacket.header = static_cast<uint8_t>(0x1B);
    stunPacket.vid = 54321;
    std::span<const uint8_t> stunSpan(reinterpret_cast<const uint8_t*>(&stunPacket), sizeof(stunPacket));
    auto resStun = dispatcher.Dispatch(static_cast<uint16_t>(0x1B), stunSpan);
    assert(resStun.has_value());
    
    // Test underflow rejection
    std::vector<uint8_t> underflowPayload(2, 0);
    auto resUnderflow = dispatcher.Dispatch(Client::Network::Protocol::GC::DAMAGE_INFO, underflowPayload);
    assert(!resUnderflow.has_value());
    assert(resUnderflow.error() == EterBase::PacketError::BufferUnderflow);
    
    dispatcher.Clear();
    std::cout << "test_register_default_handlers passed\n";
}

int main() {
    std::cout << "Running ModernPacketDispatcher tests...\n";
    
    test_unregistered_opcode();
    test_static_size_success();
    test_static_size_underflow();
    test_dynamic_size_success();
    test_dynamic_size_underflow();
    test_handler_error();
    test_unregister_handler();
    test_16bit_extended_opcodes();
    test_functional_packet_handler();
    test_strangler_network_facade();
    test_register_default_handlers();
    
    std::cout << "All ModernPacketDispatcher tests passed.\n";
    return 0;
}
