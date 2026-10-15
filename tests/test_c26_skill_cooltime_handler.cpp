#include "../src/Client/Network/Handlers/SkillCooltimeHandler.h"
#include "../src/Client/Gameplay/SkillDomain.h"
#include "../src/UserInterface/Core/EventBus.h"
#include <iostream>
#include <cassert>
#include <vector>

void test_buffer_underflow() {
    Client::Gameplay::SkillDomain domain;
    std::vector<uint8_t> buffer(sizeof(TPacketGCSkillCoolTimeEnd) - 1, 0); // Too small
    
    auto result = SkillCooltimeHandler::HandlePacket(buffer, domain);
    assert(!result.has_value() && "Expected error but got success");
    assert(result.error() == EterBase::PacketError::BufferUnderflow && "Expected BufferUnderflow error");
    
    std::cout << "test_buffer_underflow passed.\n";
}

void test_successful_handling() {
    Client::Gameplay::SkillDomain domain;
    domain.RegisterSkill(10, 1);
    domain.StartCooldown(10, std::chrono::milliseconds(5000));
    assert(!domain.IsSkillReady(10) && "Skill should be on cooldown");

    TPacketGCSkillCoolTimeEnd packet{};
    packet.header = 123;
    packet.length = sizeof(TPacketGCSkillCoolTimeEnd);
    packet.bSkill = 10;
    
    std::vector<uint8_t> buffer(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));

    bool eventReceived = false;
    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<SkillCooltimeEndEvent>(
        [&eventReceived](const SkillCooltimeEndEvent& event) {
            if (event.skillId.value() == 10) {
                eventReceived = true;
            }
        });

    auto result = SkillCooltimeHandler::HandlePacket(buffer, domain);
    assert(result.has_value() && "Expected successful packet handling");
    assert(domain.IsSkillReady(10) && "Cooldown should be reset in the domain");
    assert(eventReceived && "EventBus should have published the event");

    UserInterface::Core::EventBus::GetInstance().Unsubscribe<SkillCooltimeEndEvent>(subId);

    std::cout << "test_successful_handling passed.\n";
}

int main() {
    test_buffer_underflow();
    test_successful_handling();
    std::cout << "All SkillCooltimeHandler tests passed successfully.\n";
    return 0;
}
