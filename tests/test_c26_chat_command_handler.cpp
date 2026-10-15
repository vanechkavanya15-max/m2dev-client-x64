#include <cassert>
#include <iostream>
#include <string>
#include "Client/Gameplay/ChatCommandHandler.h"

using namespace Client::Gameplay;

void TestValidNormalMessage() {
    ChatCommandHandler handler;
    ChatMessage msg{ChatType::Normal, "Hello world", 1};
    
    auto result = handler.HandleMessage(msg);
    assert(result.has_value() && "Valid normal message should not fail");
    std::cout << "TestValidNormalMessage passed.\n";
}

void TestEmptyMessage() {
    ChatCommandHandler handler;
    ChatMessage msg{ChatType::Normal, "", 1};
    
    auto result = handler.HandleMessage(msg);
    assert(!result.has_value() && "Empty message should fail");
    assert(result.error() == CommandError::EmptyMessage && "Error should be EmptyMessage");
    std::cout << "TestEmptyMessage passed.\n";
}

void TestSpacesOnlyMessage() {
    ChatCommandHandler handler;
    ChatMessage msg{ChatType::Normal, "     ", 1};
    
    auto result = handler.HandleMessage(msg);
    assert(!result.has_value() && "Spaces-only message should fail");
    assert(result.error() == CommandError::EmptyMessage && "Error should be EmptyMessage");
    std::cout << "TestSpacesOnlyMessage passed.\n";
}

void TestMessageTooLong() {
    ChatCommandHandler handler;
    std::string longMsg(129, 'a');
    ChatMessage msg{ChatType::Normal, longMsg, 1};
    
    auto result = handler.HandleMessage(msg);
    assert(!result.has_value() && "Message > 128 chars should fail");
    assert(result.error() == CommandError::MessageTooLong && "Error should be MessageTooLong");
    std::cout << "TestMessageTooLong passed.\n";
}

void TestShoutMessageLevelTooLow() {
    ChatCommandHandler handler;
    ChatMessage msg{ChatType::Shout, "Buying sword!", 14};
    
    auto result = handler.HandleMessage(msg);
    assert(!result.has_value() && "Shout message from level < 15 should fail");
    assert(result.error() == CommandError::LevelTooLowForShout && "Error should be LevelTooLowForShout");
    std::cout << "TestShoutMessageLevelTooLow passed.\n";
}

void TestShoutMessageLevelValid() {
    ChatCommandHandler handler;
    ChatMessage msg{ChatType::Shout, "Buying sword!", 15};
    
    auto result = handler.HandleMessage(msg);
    assert(result.has_value() && "Shout message from level >= 15 should not fail");
    std::cout << "TestShoutMessageLevelValid passed.\n";
}

int main() {
    std::cout << "Running ChatCommandHandler tests...\n";
    TestValidNormalMessage();
    TestEmptyMessage();
    TestSpacesOnlyMessage();
    TestMessageTooLong();
    TestShoutMessageLevelTooLow();
    TestShoutMessageLevelValid();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
