# Modul: Client::Network
## Status: Nowoczesne Kodeki Sieciowe i Dynamiczny Silnik Protokolu C++23

### 1. Przeznaczenie i Odpowiedzialnosc
Modul odpowiada za deserializacje, serializacje i walidacje binarna pakietow sieciowych multi-server:
- Bezpieczne parsowanie buforow ze sprawdzaniem `std::span` i granic pamieci (ochrona przed Buffer Underflow / Overflow).
- Kodeki domenowe: Gildia, Party, Quest, Sklep, Handel, Skille, Cel/HP, Swiat, Ulepszanie, Walka.
- Dynamiczny parser ukladu pakietow JSON z serwerow (`PacketSchemaEngine`).

---

### 2. Zelazne Reguly Architektoniczne
1. **Semantyka EterBase::PacketResult<T>:** Kazda metoda `Decode` zwraca `PacketResult<T>`, uzywajac typowanych bledow z `enum class PacketError`.
2. **Ochrona Rozmiaru Bufora:** Przed odczytem naglowka i payloadu bezwzgledny wymog weryfikacji `buffer.size() >= sizeof(TPacketHeader)`.
3. **Pakiety Dynamiczne:** Weryfikacja `header.length >= sizeof(header)` i bezpieczna terminacja stringow.
4. **Izolacja UI:** Kodeki nie moga miec pojecia o istnieniu klas z `src/UserInterface/`.

---

### 3. Eksportowane Kodeki
- `Client::Network::GuildPacketCodec`
- `Client::Network::PartyPacketCodec`
- `Client::Network::QuestPacketCodec`
- `Client::Network::ShopPacketCodec`
- `Client::Network::ExchangePacketCodec`
- `Client::Network::SkillPacketCodec`
- `Client::Network::TargetPacketCodec`
- `Client::Network::WorldPacketCodec`
- `Client::Network::RefinePacketCodec`
- `Client::Network::CombatPacketCodec`

---

### 4. Przypisane Testy Jednostkowe
- `tests/test_c26_strangler_facade.cpp`
- `tests/test_c26_zero_copy_ring_buffer.cpp`
