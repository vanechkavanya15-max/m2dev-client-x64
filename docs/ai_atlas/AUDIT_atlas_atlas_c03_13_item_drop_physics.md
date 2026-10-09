---
task_id: "atlas_c03_13_item_drop_physics"
cluster: "ITM"
module_name: "Fizyka Upuszczania Przedmiotow i Limiter Podnoszenia"
target_files:
- src/Client/Gameplay/InventoryDropPhysicsBridge.h
- src/Client/Gameplay/InventoryPickupLimiter.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_13_item_drop_physics.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul zarzadza dwiema kluczowymi funkcjami w cyklu zycia przedmiotow upuszczanych w grze:
1. **Fizyka Upuszczania (`InventoryDropPhysicsBridge`)**: W momencie upuszczenia przedmiotu przez gracza (przy uzyciu `DropItemCommand`), modul ten oblicza docelowe wspolrzedne przedmiotu. Przedmiot rzucany jest na losowy kat na odleglosc pomiedzy 50.0f a 200.0f (wartosci `MIN_DROP_RADIUS` i `MAX_DROP_RADIUS`) od aktualnej pozycji gracza, korzystajac z dystrybucji jednostajnej (C++23 `std::uniform_real_distribution`). Wspolrzedna Z pozostaje zazwyczaj niezmieniona dla uproszczenia (zakladajac lokalnie plaski teren).
2. **Ogranicznik Podnoszenia (`InventoryPickupLimiter`)**: Zapobiega spamowaniu pakietow oraz cheatom zwiazanym z podnoszeniem. Sprawdza odleglosc od gracza do przedmiotu (nie wieksza niz `MAX_PICKUP_RANGE` = 300.0f). Posiada mechanizm zapobiegania szybkiemu klikaniu (rate limiting), z cooldownem wynoszacym 100ms. Cykl zycia wymaga aktualizowania metody w czasie biezacej klatki (OnUpdate) badz w obsludze ticku komend (Network Tick), przekazujac aktualny timestamp w milisekundach.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**: 
  - `InventoryDropPhysicsBridge` jest wywolywany gdy gracz wykonuje komende upuszczenia przedmiotu. Do funkcji `CalculateDropTrajectory` przekazywana jest struktura `MapCoords` z pakietu `DomainCommands.h`.
  - `InventoryPickupLimiter` jest wywolywany podczas weryfikacji zadania podniesienia przedmiotu przez logike akcji gry/ekwipunku.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - Generatory liczb losowych z biblioteki standardowej `<random>` (`std::mt19937`, `std::uniform_real_distribution`). Wymaga to `cmath` (dla `cos`, `sin`) oraz `numbers` (`std::numbers::pi_v`).
  - Uzywa globalnego zdefiniowanego resultatu C++23: `EterBase::Result` (z pliku `EterBase/Result.h`) oraz wyliczenia bledow komend `Client::Core::CommandError` (z `Client/Core/DomainCommands.h` badz `Client/Core/DomainErrors.h`).
  - Uzywa zdefiniowanych typow i wektorow pozycji: `Client::Core::MapCoords`, `Client::Core::DropItemCommand`.
- **Drzewo dyrektyw `#include`**: 
  - Zawiera wlaczenie globalnego naglowka z przedrostkami `#include "EterBase/StdAfx.h"`.
  - Obejmuje m.in. `<random>`, `<cmath>`, `<numbers>`, `<cstdint>`, `<expected>`.
  - Powiazania z systemem komend to `../Core/StrongTypes.h`, `../Core/DomainCommands.h`, `../../EterBase/Result.h`.
- **Model pamieciowy**:
  - Obiekty te zarzadzaja prostym stanem pamieci bez uzycia inteligentnych ani surowych wskaznikow (poza strukturami bazowymi). `InventoryDropPhysicsBridge` uzywa obiektu instancji `std::mt19937`, a `InventoryPickupLimiter` przechowuje prosta wartosc `uint32_t` jako pamiec stanu cooldownu podnoszenia. Przekazywanie danych zewnetrznych odbywa sie przez `const referencje` np. `const Client::Core::MapCoords&`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
- **Tabela Klas i Struktur**:
  - `InventoryDropPhysicsBridge`: Odpowiada za obliczanie punktow do zrzucenia przedmiotow. Nie ma duzej wagi, ale przechowuje generator `std::mt19937`. Nalezy uwazac na synchronizacje jesli uzyto by w wielu watkach jednoczesnie.
  - `InventoryPickupLimiter`: Prosty walidator odleglosci i spamu. Przechowuje czas podnoszenia wewnatrz prywatnej zmiennej `m_lastPickupTime`.
  
- **Tabela Metod Publicznych**:
  - `Client::Core::MapCoords InventoryDropPhysicsBridge::CalculateDropTrajectory(const Client::Core::MapCoords& playerPos, const Client::Core::DropItemCommand& command)`: Zwraca finalne koordynaty obiektu w momencie dropu. Brak wiekszych skutkow ubocznych na obiekcie, ale popycha do przodu stan generatora `m_rng`.
  - `EterBase::Result<void, Client::Core::CommandError> InventoryPickupLimiter::CanPickup(const Client::Core::MapCoords& playerCoords, const Client::Core::MapCoords& itemCoords, uint32_t currentTimestamp)`: Zwraca brak wartosci jesli zwalidowano pozytywnie. W przeciwnym razie zwroci `std::unexpected` z bledu: `CommandError::RateLimited` badz `CommandError::OutOfRange`. Skutkiem ubocznym jest zmiana `m_lastPickupTime` w razie udanego przejscia walidacji.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets)**:
  - Brak dedykowanych pol czlonkowskich, ktore podlegalyby latwemu hookingowi bez modyfikatorow modulu FFI. Zmienna czasu ograniczajaca podnoszenie w `InventoryPickupLimiter` mozna by wyzerowac, aby oszukac klienta, ale serwer nie pozwoli na szybsze i tak. Opcja fizyki nie jest latwo modyfikowana bez podmiany hookow na `CalculateDropTrajectory`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- Moduly w architekturze `Client::Core` i `Gameplay` dzialaja w srodowisku C++23. Nie narzucaja bezposredniego powiazania z Pythonem.
- Pakiet w kliencie z komenda wywolywana to struktura w sieciowych ukladach np. `TPacketCGItemDrop` -> powiazany potem z akcja `DropItemCommand` na ktora to powoluje sie narzedzie do fizyki luku upuszczenia.
- `PickupCommand` jest zwiazkowy z paczkami `TPacketCGItemPickup`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci**:
  - W klasie `InventoryDropPhysicsBridge` stan wewnetrzny generatora liczb pseudolosowych (`std::mt19937 m_rng`) jest inicjowany jednokrotnie z `std::random_device` w konstruktorze. Poniewaz `std::mt19937` nie jest "thread-safe", jezeli instancja klasy okaze sie byc globalna badz dostepna wielowatkowo, nalezaloby zastosowac np. `thread_local std::mt19937` lub muteksy. Uklad ten zaklada wiec egzekucje na watku glownym / ticku serwera-klienta.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - W `InventoryDropPhysicsBridge::CalculateDropTrajectory` ignorowana jest calkowicie os Z. Wartosc po skosie (delta X / delta Y) nie uwzglednia kolizji ze scianami czy woda – zaklada plaski teren. Predykcja rzutu moze pchnac przedmiot w sciane w przypadku specyficznych instancji.
  - W `InventoryPickupLimiter::CanPickup`, pierwsza proba podniesienia (gdzie `m_lastPickupTime` wynosi 0) omija check czasowy, co chroni przed "false positive rate limit". Zmiana tego kodu moze uszkodzic pierwsze podnoszenie przedmiotu po zalogowaniu.
- **Zarzadzanie zasobami (RAII)**:
  - Brak manualnej alokacji – operacje bazowo wykorzystuja obiekty na stosie lub wewnetrzne stany klasy. Nie wystepuje ryzyko wylomow i wyciekow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**: 
  - Krok 1: W plikach `InventoryDropPhysicsBridge.h` (lub `.cpp`) okresl nowe dystrybucje do logiki fizycznej (np. uwzglednienie osi Z lub grawitacji).
  - Krok 2: Nadpisz uklad `targetPos.z` bazujac na mapie kolizji terenu badz obliczeniach predkosci poczatkowej V0.
  - Krok 3: W `InventoryPickupLimiter.cpp` dodaj zamiane `distance` na `distanceSq` przeciwko `MAX_PICKUP_RANGE * MAX_PICKUP_RANGE`.
- **Jak debugowac i logowac**:
  - Do logowania wprowadz dyrektywe `#include "EterBase/Debug.h"` lub wlasny rejestrator modulu AI i wywoluj np. `LogTrace("Pickup called by {}, current CD: {}", playerVid, m_lastPickupTime);`. Breakpointy mozna zakladac wewnatrz `CanPickup` na linii powrotu bledu (przy wczesnym returnie). Kluczowa zmienna do podgladu (Watch) to `m_lastPickupTime`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: 
  - Kod korzysta z C++23. Do testow uzywaj `doctest`.
  - Zalacz `.cpp` do glownego pliku testow, symuluj wywolania np. `CanPickup` z odpowiednim timestampem, by wywolywac stan limitera spamu (oczekuj `EterBase::CommandError::RateLimited`). Nie wymagana konfiguracja instancji DirectX (architektura Zero-DirectX).
