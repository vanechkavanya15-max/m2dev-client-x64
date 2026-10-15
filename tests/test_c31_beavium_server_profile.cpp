#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdint>

#include "Client/Network/Protocol/Drivers/BeaviumProtocolProfile.h"
#include "Client/Network/Protocol/Drivers/BeaviumProtocolDriver.h"
#include "Client/Gameplay/BeaviumCombatPolicy.h"
#include "UserInterface/Network/Routers/PhaseGamePacketDispatcher.h"

using namespace Network::Protocol::Drivers;
using namespace Client::Gameplay;
using namespace UserInterface::Network::Routers;

void TestBeaviumProtocolProfile()
{
    std::cout << "[RUN] TestBeaviumProtocolProfile..." << std::endl;

    BeaviumProtocolProfile profile;

    // 1. Weryfikacja remapowania
    assert(profile.RemapHeader(0x0B) == 0x0B);
    assert(profile.RemapHeader(0x0E) == 0x0E);
    assert(profile.RemapHeader(0x0C) == 0x0C);
    assert(profile.RemapHeader(0x03) == 0x03);
    assert(profile.RemapHeader(0x10) == 0x12); // ITEM_SET 0x10 -> 0x12
    assert(profile.RemapHeader(0x6C) == 0x6C);

    // 2. Weryfikacja odrzucania pakietow customowych (Discard Table)
    // Pakiety te musza byc ignorowane bez desynchronizacji bufora TCP:
    assert(profile.IsDiscardHeader(0x8D)); // Voice Chat
    assert(profile.GetDiscardSize(0x8D) == 170);

    assert(profile.IsDiscardHeader(0x76)); // Switchbot State
    assert(profile.GetDiscardSize(0x76) == 133);

    assert(profile.IsDiscardHeader(0x94)); // Aura System
    assert(profile.GetDiscardSize(0x94) == 31);

    assert(profile.IsDiscardHeader(0x95)); // Sash / Acce System
    assert(profile.GetDiscardSize(0x95) == 31);

    assert(profile.IsDiscardHeader(0x0D)); // Stun / Guild Beavium
    assert(profile.GetDiscardSize(0x0D) == 3073);

    assert(profile.IsDiscardHeader(0x7C)); // Overhead Title
    assert(profile.GetDiscardSize(0x7C) == 70);

    assert(profile.IsDiscardHeader(0x45)); // Refine Information (136B w Beavium x64)
    assert(profile.GetDiscardSize(0x45) == 136);

    assert(profile.IsDiscardHeader(0x83)); // Target Loot Close (6B w Beavium x64)
    assert(profile.GetDiscardSize(0x83) == 6);

    assert(profile.IsDiscardHeader(0x6B)); // Ranking Entry (6B w Beavium x64)
    assert(profile.GetDiscardSize(0x6B) == 6);

    assert(profile.IsDiscardHeader(0x9A)); // Security Challenge (2B w Beavium x64)
    assert(profile.GetDiscardSize(0x9A) == 2);

    assert(profile.IsDiscardHeader(0x62)); // Teleport Panel (2B w Beavium x64)
    assert(profile.GetDiscardSize(0x62) == 2);

    // Pakiet dynamiczny PET_INFO nie moze byc w stalej tabeli discard
    assert(!profile.IsDiscardHeader(0x8A));
    // Pakiet CHAR_ADDITIONAL_INFO (0x65) nie moze byc discardowany (niesie dane postaci)
    assert(!profile.IsDiscardHeader(0x65));

    // Standardowe pakiety nie moga byc discardowane
    assert(!profile.IsDiscardHeader(0x01)); // CHARACTER_ADD
    assert(!profile.IsDiscardHeader(0x02)); // CHARACTER_DEL
    assert(profile.GetDiscardSize(0x01) == 0);

    // 3. Wstrzykniecie do dyspozytora PhaseGamePacketDispatcher
    PhaseGamePacketDispatcher& dispatcher = PhaseGamePacketDispatcher::Instance();
    dispatcher.SetProtocolProfile(&profile);

    // Pakiet customowy musi zostac bezpiecznie skonsumowany przez dyspozytor
    std::vector<uint8_t> dummyVoicePacket(170, 0xAA);
    bool voiceHandled = dispatcher.RouteGamePacket(static_cast<uint8_t>(0x8D), dummyVoicePacket.data(), dummyVoicePacket.size());
    assert(voiceHandled);

    std::vector<uint8_t> dummySwitchbot(133, 0xBB);
    bool switchbotHandled = dispatcher.RouteGamePacket(static_cast<uint8_t>(0x76), dummySwitchbot.data(), dummySwitchbot.size());
    assert(switchbotHandled);

    // Przywrocenie domyslnego profilu
    dispatcher.SetProtocolProfile(nullptr);

    std::cout << "[PASS] TestBeaviumProtocolProfile zakonczony sukcesem!" << std::endl;
}

void TestBeaviumCombatPolicy()
{
    std::cout << "[RUN] TestBeaviumCombatPolicy..." << std::endl;

    BeaviumCombatPolicy combatPolicy;

    // 1. Weryfikacja klucza XOR
    assert(BeaviumCombatPolicy::COMBAT_OBFUSCATION_KEY == 0xE73AC1DA);

    uint32_t targetVid = 54321;
    uint32_t obfuscatedKey = combatPolicy.CalculateObfuscatedTargetKey(targetVid);
    assert(obfuscatedKey == (54321 ^ 0xE73AC1DA));
    // Dwu krotny XOR musi przywrocic oryginalny VID
    assert((obfuscatedKey ^ 0xE73AC1DA) == targetVid);

    // 2. Weryfikacja formuly Hit Chance: 70% + (Level + 2 * Stat) / 9
    // Level 1, Stat 10: 70 + (1 + 20)/9 = 70 + 2 = 72%
    assert(combatPolicy.CalculateHitChance(1, 10) == 72);

    // Level 50, Stat 60: 70 + (50 + 120)/9 = 70 + 18 = 88%
    assert(combatPolicy.CalculateHitChance(50, 60) == 88);

    // Dolny limit (clamp 70%): Level 0, Stat 0 -> 70%
    assert(combatPolicy.CalculateHitChance(0, 0) == 70);

    // Gorny limit (clamp 100%): Level 99, Stat 90 -> 70 + (99 + 180)/9 = 70 + 31 = 101 -> 100%
    assert(combatPolicy.CalculateHitChance(99, 90) == 100);

    // 3. Weryfikacja logiki ataku i cooldownu
    assert(!combatPolicy.CanAttackInstance(0)); // VID 0 nie moze byc atakowany
    assert(combatPolicy.CanAttackInstance(targetVid));

    // Wykonanie ataku
    bool attackOk = combatPolicy.AttackTarget(targetVid);
    assert(attackOk);
    assert(combatPolicy.GetLastTargetVID() == targetVid);

    // Bezposrednio po ciosie cel jest na cooldownie (250 ms)
    assert(!combatPolicy.CanAttackInstance(targetVid));

    // Uplyw czasu mniejszy niz cooldown (np. 100 ms)
    combatPolicy.UpdateAttackState(0.10f);
    assert(!combatPolicy.CanAttackInstance(targetVid));

    // Uplyw czasu przekraczajacy cooldown (dodatkowe 200 ms -> lacznie 300 ms)
    combatPolicy.UpdateAttackState(0.20f);
    assert(combatPolicy.CanAttackInstance(targetVid));

    std::cout << "[PASS] TestBeaviumCombatPolicy zakonczony sukcesem!" << std::endl;
}

void TestBeaviumProtocolDriver()
{
    std::cout << "[RUN] TestBeaviumProtocolDriver..." << std::endl;

    BeaviumProtocolDriver driver;
    assert(driver.GetDriverName() == "beavium");

    // 1. Test InspectFrame
    // Ramka stala ATTACK (0x0C) o rozmiarze 12B
    std::vector<uint8_t> attackBuffer(12, 0);
    attackBuffer[0] = 0x0C;
    auto headerInfo = driver.InspectFrame(attackBuffer);
    assert(headerInfo.has_value());
    assert(headerInfo->unifiedOpcode == 0x0C);
    assert(headerInfo->packetLength == 12);
    assert(headerInfo->headerSize == 1);

    // 2. Test EncodeAttack (8 bajtow specyficzne dla Beavium z ProxyPacketCGAttack)
    Domain::AttackCommand attackCmd{
        .targetVid = 998877,
        .attackType = 1,
        .sequence = 42
    };

    auto encodedAttack = driver.EncodeAttack(attackCmd);
    assert(encodedAttack.has_value());
    assert(encodedAttack->size() == 8);
    assert((*encodedAttack)[0] == 0x02); // Beavium::CG::ATTACK = 0x02
    assert((*encodedAttack)[1] == 1);    // attackType

    uint32_t encodedVid = 0;
    std::memcpy(&encodedVid, &(*encodedAttack)[2], sizeof(uint32_t));
    assert(encodedVid == 998877);

    uint16_t encodedSeq = 0;
    std::memcpy(&encodedSeq, &(*encodedAttack)[6], sizeof(uint16_t));
    assert(encodedSeq == 42);

    std::cout << "[PASS] TestBeaviumProtocolDriver zakonczony sukcesem!" << std::endl;
}

int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << " START TESTU C31: BEAVIUM SERVER PROFILE INTEGRATION  " << std::endl;
    std::cout << "======================================================" << std::endl;

    TestBeaviumProtocolProfile();
    TestBeaviumCombatPolicy();
    TestBeaviumProtocolDriver();

    std::cout << "======================================================" << std::endl;
    std::cout << " WSZYSTKIE TESTY PROFILU BEAVIUM ZAKONCZONE SUKCESEM! " << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
