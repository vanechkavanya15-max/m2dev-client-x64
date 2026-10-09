---
task_id: "atlas_c04_04_combat_sequence_guard"
cluster: "CBT"
module_name: "Straznik Sekwencji Atakow (Anti-Fast-Attack Guard)"
target_files:
- src/Client/Gameplay/CombatSequenceGuard.h
- src/GameLib/AttackRateLimiter.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_04_combat_sequence_guard.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul zapewnia ochrone przed modyfikacjami klienta i tzw. "Speed Hackami" w obszarze szybkosci atakow. Odpowiada za weryfikacje czestotliwosci (rate limiting) atakow na dwa sposoby: klasycznie przez weryfikacje przesuwnego okna w czasie (sliding window) w celu unikania odlaczenia od serwera (anti-kick protection) oraz nowoczesnie, limitujac interwaly atakow, zarazem generujac CRC z modyfikatora sekwencyjnego dla weryfikacji po stronie serwera, co zapobiega desynchronizacji.
- **Wywolywanie w petli gry:** Kod najprawdopodobniej uruchamiany jest przed zleceniem pakietu ataku do sieci, czyli podczas przetwarzania logiki akcji gracza (np. na zakonczenie lub starcie animacji ataku w OnUpdate / w mechanizmie stanow kontrolerow instancji) lub bezposrednio w mostku sieciowym (Network Tick) przy pakowaniu `CombatPacketCodec`.
- **Przeplyw danych:** 
  1. `AttackRateLimiter::CheckLimit` weryfikuje stempel czasowy zlecenia ataku (ms). Weryfikuje i oczyszcza nieaktualne ataki i w razie skoku czasu (wrap-around) czysci bufor. Jezeli atak miesci sie w oknie, zapisuje go, inaczej blokuje logike sieciowa przed generowaniem nastepnych pakietow.
  2. `CombatSequenceGuard::ProcessAttackSequence` sprawdza delte miedzy aktulnym `std::chrono::time_point` a ostatnim atakiem. Jesli uderzenie bylo za szybko, uzywa nowoczesnego sposobu obslugi bledu - zwraca `std::unexpected(Core::CommandError::RateLimited)`. Gdy sie powiedzie, aktualizuje czas i stan wewnetrzny liczydla sekwencji (`m_sequence`), zwracajac reszte z dzielenia przez 256. 
- **Cykl zycia obiektow:** Typowe obiekty kompozytowe. Powinny znajdowac sie w strukturze stanow aktorow (np. `ActorRecord` / `ECSComponents` w `ActorRegistry` lub w `CInstanceBase`). Nie zarzadzaja aktywnymi zasobami (tylko stany wewnetrzne typu `deque` czy `time_point`). Usuwane wraz z usuwaniem encji gracza / postaci ze swiata.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Do tego modulu odwoluja sie klasy sieciowe / encje, takie jak kontroler encji klienta `CInstanceBase`, logiki stanow walki lub bridge'y typu `PhaseGameCombatBridge` przesylajace `TPacketCGAttack` do serwera.
- **Zaleznosci wyjsciowe (Outbound):** Zaleznosci to czyste abstrakcje wewnetrzne: `../Core/DomainCommands.h` oraz `../Core/Result.h`. Obydwa obiekty sa calkowicie uniezaleznione od interfejsow graficznych i nie odpytuja o zaleznosci od bibliotek takich jak DirectX czy Python.
- **Drzewo dyrektyw `#include`:** 
  - `src/Client/Gameplay/CombatSequenceGuard.h`: `<chrono>`, `<cstdint>`, `"../Core/DomainCommands.h"`, `"../Core/Result.h"`. (Czyste narzedzia).
  - `src/GameLib/AttackRateLimiter.h`: `<cstdint>`, `<deque>`, `<stdexcept>`. (Brak ryzyka cykli dyrektyw i czyste moduly STL).
- **Model pamieciowy:** W pelni wyizolowane klasy wartosciowe. Przechowuja dane by-value w obiekcie (`uint32_t`, `std::deque`, std::chrono::time_point). Nie eksponuja wskaznikow, nie posiadaja mechanizmow RAII narzutowych. Moga byc kopiowane lub przenoszone wedle specyfikacji C++.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `Client::Gameplay::CombatSequenceGuard` | Kontrola szybkosci sekwencji wg CRC | ~16 bajtow | Prawdopodobnie watek glowny logiczny lub dedykowany thread pool ECS.
  - `AttackRateLimiter` | Ograniczanie spamu zadan atakow wg okna ms (Sliding Window) | ~40 bajtow | Prawdopodobnie watek glowny logiczny.
- **Tabela Metod Publicznych:**
  - `CombatSequenceGuard::ProcessAttackSequence(std::chrono::steady_clock::time_point now, std::chrono::milliseconds minInterval)` -> Zwraca `Core::Result<uint8_t, Core::CommandError>`. Warunki: brak. Skutki uboczne: uaktualnienie wewnetrznego zegara ataku i numeru.
  - `AttackRateLimiter::AttackRateLimiter(uint32_t limit, uint32_t window_ms)` -> Zwraca `void`. Warunki: Obie wartosci > 0. Rzuca `std::invalid_argument` jesli parametry rowne zero.
  - `AttackRateLimiter::CheckLimit(uint32_t current_time_ms)` -> Zwraca `bool`. Warunki: Brak. Skutki: Usuniecie przedawnionych stanow w kolejce, czyszczenie przy przewinieciu czasu (wrap-around), a takze zrzucenie nowego rekordu ataku na deque.
  - `AttackRateLimiter::Reset()` -> Zwraca `void`. Skutki: usuniecie calego stanu z kontenera deque.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - `CombatSequenceGuard`: Offset `0x00`: `uint32_t m_sequence`, Offset `0x08`: `std::chrono::steady_clock::time_point m_lastAttackTime` (mozliwe 8-bajtowe przesuniecie na 64bit wg paddingu x64).
  - `AttackRateLimiter`: Offset `0x00`: `uint32_t limit`, Offset `0x04`: `uint32_t window_ms`, Offset `0x08`: `std::deque<uint32_t> attack_times` (offset 8B na systemach 64 bit).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Wyliczany wynik bajtowy CRC z `CombatSequenceGuard::ProcessAttackSequence` uzywany jest przypuszczalnie w generowaniu danych pakietu walki (`TPacketCGAttack`, opcody ataku na serwer np. dla wysylania informacji do `Network::CombatPacketCodec`). Limiter powstrzymuje zalewanie serwera przez wazne zadania akcji bojowych.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnie zakorzeniony w C++ i nie ma zaleznosci do Pythona w warstwie C-API (brak PyObject*, METH_VARARGS itp.). Nie eksponuje wprost dostepu do UI ani Pythonowych skryptow. Pakiety i weryfikacja leza w glebokich trzewiach klienta.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekty nie chronia dostepu do wlasnych atrybutow przed wielowatkowoscia (np. manipulacje na `std::deque` lub zmienianie `m_sequence`). Trzeba ich uzywac jako lokalnych bytow izolowanych w procesach encji bez wylaniania pamieci, zsynchronizowanych per Encja, np. z multi-threaded w `ActorRegistry`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Overflow dla standardowego uint32 w biezacym czasie ms (`current_time_ms`) obslugiwane przez reczne czyszczenie bufora jezeli `current_time_ms < attack_times.back()`.
  - Powstanie `std::invalid_argument` podczas instancjonowania obiektu `AttackRateLimiter` przy zmiennych wejsciowych mniejszych rownych 0 (nalezy miec pewnosc logiki domeny konfiguracyjnej z serwera).
- **Zarzadzanie zasobami (RAII):** Nie dochodzi do zjawiska uzycia surowej pamieci typu `new`/`delete` - std::deque samodzielnie dba o dynamiczna alokacje a rozmiar bufora nigdy nie przekroczy wartosci `limit`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W razie wprowadzania modyfikacji (np. zmiana mechaniki CRC na inna lub dodawanie odleglosci limitowania atakow w zaleznosci od wybranej rasy), dodawaj dane parametrowe do funkcji (np. do `ProcessAttackSequence`). Nalezy uzywac zwrotu typu `Core::Result` unikajac wyjatkow w czasie trwania i unikajac magic_numbers.
- **Jak debugowac i logowac:** Przechwycenie nadmiarowych atakow i uzycie nowoczesnego loggera EterBase. Zastosowac wyjscie logowania w warunku wylapywania `std::unexpected(Core::CommandError::RateLimited)`, co pozwala przesledzic dlaczego system ucial atak na serwer (zapobiega pytaniom "dlaczego ataki nie wchodza").
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W obu przypadkach architektura wspiera czyste srodowiska testowe. Test w `doctest` tworzony jest przez manipulacje `std::chrono::time_point` i wywolywanie wielokrotne `ProcessAttackSequence` oraz symulujac dzialanie dla limitera, testujac warunek przekroczenia 0, wyjatek oraz typowy overload w ciagu kilku ms bez uruchamiania calego UI, poniewaz brak jakichkolwiek uwiklan DirectX/EterLib.
