---
task_id: "atlas_c01_13_packet_headers_structs"
cluster: "NET"
module_name: "Katalog Struktur Pakietow i Pamieciowych Layoutow"
target_files:
- src/UserInterface/Packets/PacketHeader.h
- src/UserInterface/Packets/PacketCharacter.h
- src/UserInterface/Packets/PacketCombat.h
- src/UserInterface/Packets/PacketTrade.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_13_packet_headers_structs.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul ten pelni funkcje glownego slownika i katalogu struktur sieciowych uzywanych przez klienta gry do komunikacji z serwerem Game oraz Auth. W ramach modernizacji C++23, monolityczne pliki naglowkowe (takie jak `PacketHeader.h`, `PacketCharacter.h`, `PacketCombat.h`, `PacketTrade.h`) zostaly zdeprecjonowane i podzielone na samodzielne, mniejsze naglowki w sciezce `src/Client/Network/Protocol/Packets/` (np. `Packet_ActorAdd.h`, `Packet_CGAttack.h`, `Packet_Exchange.h`, `Packet_Skills.h`) oraz `ProtocolOpcodes.h`. Kod ten jest wywolywany glownie w petli sieciowej klienta (`Network Tick`), dekodujac binarne dane ze strumienia TCP (Winsock) i konwertujac je na pamieciowe struktury (POD - Plain Old Data), przekazywane potem za pomoca `EventBus` do subsystemow wizualnych i logiki gry. Zapewnia on dokladny mapping (`#pragma pack(1)`) bajtow miedzy zadeklarowanymi offsetami C++ a ramkami danych serwera. Cykl zycia obiektow opiera sie na tymczasowych zmiennych alokowanych bezposrednio na stosie wewnatrz handlerow pakietow, odczytywanych przez rzutowanie (reinterpret_cast) surowej tablicy typu `std::span<const uint8_t>`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul powolywany jest przez warstwy sieci (np. `CPythonNetworkStream`, handlery i dekodery zdarzen), w tym mechanizm `EventBus`, a takze bezposrednio przez akcje wywolywane przez skrypty Pythona (eksportowane komendy sieciowe C-API w module gracza).
- **Zaleznosci wyjsciowe (Outbound):** Pakiety same w sobie sa niezalezne (Pure Data / DTOs), dzieki regule Zero-Conflict. Uzywane sa typy takie jak `EterBase::StrongTypes` czy silnie typowane enumy przestrzeni `Client::Core`. Pakiety te wykorzystuja standardowe biblioteki `<cstdint>`, `<span>`, `<vector>` i nie wchodza w interakcje z elementami zaleznymi od stanu silnika Direct3D czy Granny 3D.
- **Drzewo dyrektyw `#include`:** Dolaczane sa `ProtocolTypes.h`, `ProtocolOpcodes.h` oraz minimalne moduly z folderu `src/Client/Network/Protocol/Packets/`. Brak cyklicznych zaleznosci; struktury opieraja sie wylacznie na typach prymitywnych lub innych strukturach podkladek (jak `TItemPos`).
- **Model pamieciowy:** Wymuszony czysty C-style structs i raw data z wylaczeniem paddingu przez `#pragma pack(push, 1)`. Wszystkie operacje czytania to const operacje na pamieci.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

#### Tabele Klas i Struktur (Przykrojek wg zbadanej domeny):
- **`TPacketGCCharacterAdd`** (Plik: `Packet_ActorAdd.h`) - Informuje klienta o pojawieniu sie bytu. Rozmiar zalezny od offsetow: `id` (4 bajty), koordynaty X, Y, Z (int32 - 12 bajtow), typ, rasa, flagi stanu, kat obrotu (float).
- **`TPacketGCCharacterDelete`** (Plik: `Packet_ActorDel.h`) - `header` (1 bajt), `id` (4 bajty). Rozmiar: 5 bajtow.
- **`TPacketCGAttack`** (Plik: `Packet_CGAttack.h`) - Wysyla typ ataku, VID celu, dane CRC. Wykorzystuje `union` dla wsparcia zarowno nowych typow jak i pol typu "Legacy" (np. `dwVictimVID`).
- **`TPacketCGExchange` i `TPacketGCExchange`** (Plik: `Packet_Exchange.h`) - Struktury systemu handlu z polami okien handlowych, cenami i zlozonymi obiektami TItemPos oraz slotami dla kamieni (3) czy bonusow (7).
- **`PacketCGUseSkill`, `PacketGCSkillLevel`** (Plik: `Packet_Skills.h`) - Informacje o uzyciu i poziomie skilli, korzysta z limitow typu `SKILL_MAX_NUM = 255`.

#### Tabela Metod Publicznych i FFI Layout:
Klasy i struktury te nie posiadaja metod ze skutkami ubocznymi (Plain C Structs). W najnowszym silniku dostep do pakietow realizuje sie np. poprzez `inline bool HandleCharacterDelete(std::span<const uint8_t> buffer, const std::function<void(uint32_t)>& onDelete)`. Warunki wstepne: `buffer.size() == sizeof(TPacket)`. Skutki uboczne: Opublikowanie zawiadomienia (Callback) usuniecia aktora.

#### Pamieciowy Layout Struktur (Memory Layout & Offsets):
- Wszystkie struktury w domenie sieciowej uzywaja `#pragma pack(1)`. Oznacza to brak mechanizmu wyrownywania bajtow (padding bytes).
- Offsety bajtowe sa bezposrednio przewidywalne:
  - Dla `TPacketGCCharacterDelete`: `header` (offset +0), `id` (offset +1).
  - Dla `TPacketCGAttack`: `header` (offset +0), `length` (offset +2), `type` (offset +4), `targetId` (offset +5), `crc` (+9, +10). Z racji obudowania pol klauzula union kompatybilnosc jest zachowana ze starszym systemem hookow AI.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Opcody w ProtocolOpcodes.h):** 
  - `CG::LOGIN2` (0x0101), `CG::LOGIN3` (0x0102)
  - `CG::CHARACTER_CREATE` (0x0201), `CG::ENTERGAME` (0x0204)
  - Moduly handlu posiadaja specyficzny styk via Subheadery (np. `ExchangeSub::CG::...`).
- **Metody Pythona (`PyMethodDef`):** Integracja sieci jest czesto przekazywana z Pythonowego modulu gracza `PythonPlayerModule.cpp`, ktory wola odpowiednie wywolywacze w warstwie sieci. Te pakiety sluza bezposrednio jako payload (DTO) dla takich wywolan.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie operacje czytania struktur pakietow i ich rzutowania za pomoca `std::span` MUSZA odbywac sie jednowatkowo z wylacznoscia glownego watku, chyba ze sam odbior uzywa lockow lub watkow dedykowanych przed dispatchingiem poprzez `EventBus`.
- **Typowe Pulapki i Crashe:** Brak walidacji z `buffer.size() < sizeof(Struct)` przy uzywaniu starego `std::memcpy` grozi cichym wczytaniem nieprawidlowych offsetow. Silnik uzywa predykatu `std::span` i wczesnego wyjscia jesli bajty sie nie zgadzaja.
- **Zarzadzanie zasobami (RAII):** Kod na tym poziomie dziala bezalokacyjnie - nie pojawia sie instancjonowanie pamieci na stercie (`new` / `malloc`), wiec nie ma niebezpieczenstwa wyciekow z winy uzytych narzedzi - obiekty sa przekazywane poprzez stos i szybko czyszczone ze scopa.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** Aby dodac nowy pakiet (np. nowa struktura obslugi misji): 1. Zdefiniuj nowa klase typu POD z `#pragma pack(push, 1)` w nowym pliku naglowkowym w obrebie `src/Client/Network/Protocol/Packets/`. 2. Nie modyfikuj legacy naglowkow! 3. Zarejestruj unikalny opcode pakietu w `ProtocolOpcodes.h`. 4. Utworz obsluge wejsciowa (Handler) przyjmujac `std::span<const uint8_t>` w warstwie wyzszej (Dispatch).
- **Jak debugowac i logowac:** Nalezy wygenerowac zrzut heksadecymalny pakietu wychodzacego bezposrednio przed wyslaniem (`EterLib::PacketWriter`) i porownac go z ukladem w C++, a nastepnie wydrukowac zweryfikowane zmienne do konsoli diagnostycznej (logi). Zabezpieczyc breakpoints w okolicach rzutowania `reinterpret_cast`.
- **Jak testowac (Headless / Unit Test Harness):** W zwiazku z reguła Zero-Conflict testy jednostkowe buduje sie za pomoca biblioteki Doctest. Pakiety nalezy symulowac recznie wypelniajac `std::vector<uint8_t>` dokladnymi danymi offsetowymi odpowiadajacymi strukturom POD. Po zbudowaniu symulowanej payload, wywoluje sie zewnetrzny C++20 Handler analizujac czy poprawnie rozparsuje pakiet i rzuci prawidlowe zdarzenie mockujace na instancji `MockEventBus`.

