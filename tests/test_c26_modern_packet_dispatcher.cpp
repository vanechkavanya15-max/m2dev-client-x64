#include <iostream>
#include <cassert>
#include <vector>
#include "../src/Client/Network/ModernPacketDispatcher.h"

using namespace Client::Network;

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

int main() {
    std::cout << "Running ModernPacketDispatcher tests...\n";
    
    test_unregistered_opcode();
    test_static_size_success();
    test_static_size_underflow();
    test_dynamic_size_success();
    test_dynamic_size_underflow();
    test_handler_error();
    test_unregister_handler();
    
    std::cout << "All ModernPacketDispatcher tests passed.\n";
    return 0;
}
