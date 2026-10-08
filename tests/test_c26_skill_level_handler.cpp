#include <iostream>
#include <vector>
#include <cassert>
#include <optional>
#include "../src/Client/Network/Handlers/SkillLevelHandler.h"
#include "../src/Client/Gameplay/SkillDomain.h"

// Pomocnicza struktura dla testow reprezentujaca oczekiwany uklad pakietu
#pragma pack(push, 1)
struct TestSkillLevelPacket {
    uint16_t header = 0xAA;
    uint16_t length = sizeof(TestSkillLevelPacket);
    uint8_t abSkillLevels[255] = {0};
};
#pragma pack(pop)

void TestSuccessfulParsing() {
    std::cout << "[TEST] TestSuccessfulParsing - Rozpoczeto" << std::endl;
    
    Client::Gameplay::SkillDomain domain;
    
    // Ustawiamy skill z indeksami od 10 do 20 do istnienia w domenie przed testem
    for (int i = 10; i <= 20; ++i) {
        domain.RegisterSkill(i + 100, 0); 
    }

    // Mapper ktory dodaje 100 do indeksu pakietu by zrobic z niego SkillId
    Client::Network::SkillLevelHandler::IndexToSkillIdMapper mapper = 
        [](uint8_t index) -> std::optional<Client::Gameplay::SkillId> {
            if (index >= 10 && index <= 20) {
                return static_cast<Client::Gameplay::SkillId>(index + 100);
            }
            return std::nullopt;
        };
        
    Client::Network::SkillLevelHandler handler(domain, mapper);

    TestSkillLevelPacket packet;
    // Wypelniamy pakiet testowymi danymi
    for (int i = 0; i < 255; ++i) {
        packet.abSkillLevels[i] = static_cast<uint8_t>(i % 40); // Symulacja poziomu 0-39
    }
    
    // Ręczne ustawienie specyficznych wartosci do asercji
    packet.abSkillLevels[15] = 30; // SkillId 115 powinien miec poziom 30
    packet.abSkillLevels[20] = 40; // SkillId 120 powinien miec poziom 40

    std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    
    auto result = handler.HandleSkillLevel(payload);
    
    assert(result.has_value());
    
    // Sprawdzamy czy domena zaktualizowala tylko obslugiwane indeksy
    assert(domain.GetSkillLevel(115) == 30);
    assert(domain.GetSkillLevel(120) == 40);
    
    // Sprawdzamy czy nieobsługiwany indeks pozostał niezmieniony (lub domyślnie 0, bo nie był rejestrowany)
    assert(domain.GetSkillLevel(50) == 0); // indeks 50 w mapowaniu daje nullopt

    std::cout << "[TEST] TestSuccessfulParsing - Zakończony sukcesem!" << std::endl;
}

void TestBufferUnderflow() {
    std::cout << "[TEST] TestBufferUnderflow - Rozpoczeto" << std::endl;
    
    Client::Gameplay::SkillDomain domain;
    Client::Network::SkillLevelHandler::IndexToSkillIdMapper mapper = 
        [](uint8_t index) { return std::nullopt; };
        
    Client::Network::SkillLevelHandler handler(domain, mapper);

    // Tworzymy za maly bufor
    std::vector<uint8_t> smallBuffer(10, 0);
    std::span<const uint8_t> payload(smallBuffer.data(), smallBuffer.size());
    
    auto result = handler.HandleSkillLevel(payload);
    
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);

    std::cout << "[TEST] TestBufferUnderflow - Zakończony sukcesem!" << std::endl;
}

int main() {
    std::cout << "Uruchamianie testów dla SkillLevelHandler (C++23)..." << std::endl;
    
    TestSuccessfulParsing();
    TestBufferUnderflow();
    
    std::cout << "Wszystkie testy zakończone pomyślnie!" << std::endl;
    return 0;
}
