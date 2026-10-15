---
task_id: "atlas_c04_06_skill_cooldown_engine"
cluster: "CBT"
module_name: "Silnik Czasow Odnowienia Umiejetnosci (Skill Cooldown Tracker)"
target_files:
- src/Client/Gameplay/SkillCooldownTracker.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_06_skill_cooldown_engine.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
**Cel biznesowy:**
Modul ten odpowiada za dokladne i bezpieczne wielowatkowo sledzenie czasow odnowienia (cooldownow) umiejetnosci (skilli) uzywanych przez gracza. Zapewnia mechanizmy dla obliczania pozostalego czasu, postepu odnowienia (wyrazonego w ulamku np. od 0.0f do 1.0f przydatnego do interfejsu uzytkownika - zaciemnienie ikony), a takze sprawdzania, czy dany skill jest aktualnie gotowy do uzycia.

**Architektura i dzialanie:**
- Modul ten operuje w calkowitej izolacji od systemu renderowania (DirectX) czy mostkow (Pythona), co czyni go modelem domenowym (Zero-Conflict).
- Kod opiera sie na `std::unordered_map` przypisujacej `skillVnum` (identyfikator skilla) do `CooldownEntry`, ktore jest prosta struktura zawierajaca czas startu i trwania w milisekundach.
- Zapewnia determinizm czasu poprzez poleganie na wlasnych stalych odniesieniach czasu (`startTimestampMs`) na bazie `std::chrono::steady_clock` (nie uzywa zmiennych globalnych ani zaleznych od FPS np. `GetTickCount` bezposrednio, lecz owija je w czyste C++ funkcje `GetCurrentTimeMs`).
- Wywolywany moze byc w roznych kontekstach, m.in.:
  1) Kiedy przychodzi pakiet z serwera wymuszajacy cooldown.
  2) Podczas aktualizacji UI, by zaktualizowac graficzny pasek naladowania (OnUpdate interfejsu).
  3) Przed proba wyslania zadania uzycia skilla przez gracza (walidacja po stronie klienta, by uniknac niepotrzebnego ruchu sieciowego, jesli skill nie jest gotowy).

**Cykl zycia obiektow:**
- Obiekt `SkillCooldownTracker` moze zostac wykreowany jako Singleton lub komponent postaci (instancji gracza).
- `CooldownEntry` w mapie tworzy/nadpisuje metoda `StartCooldown()`.
- Wpis o skillu mozna zresetowac pojedynczo przez `ResetCooldown()`, albo wyczyscic calosc `ResetAll()`. Przy ustawieniu duration = 0 nastepuje usuniecie (erase).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Moduly renderowania UI skilli (pobieranie `GetCooldownProgress` w celu renderowania animacji ladowania na GUI).
- Obsluga pakietow (`RefineCommandEncoder` / warstwa `PacketCodec` - pakiety z serwera, ktore zarzadzaja uzyciem magii i odnowieniem, `SkillDomain.cpp` jako warstwa abstrakcji sprawdzajaca gotowosc).
- Klawiatura / Modul sterowania postaci uzywajacy `IsOnCooldown` zanim wypusci zdarzenie ataku.

**Zaleznosci wyjsciowe (Outbound):**
- Ten modul dziala "w prozni". Wola wylacznie standardowe biblioteki C++.
- Modul korzysta z przestrzeni nazw `Client::Core::SkillId` pochodzacej z `../Core/StrongTypes.h` uzywajacej wzorcow typu "Strong Type" w celu unikania bledu pomylek z typem wbudowanym w C++.

**Drzewo dyrektyw `#include`:**
```cpp
#include <cstdint>       // Dla ukladania typow calkowitoliczbowych
#include <unordered_map> // Jako podstawa bazy cooldownow
#include <chrono>        // Do rzetelnego obliczania czasu bazujac na standardach
#include <shared_mutex>  // Do bezpieczenstwa wielowatkowego - wspolbieznych odczytow i jednego logowania
#include "../Core/StrongTypes.h" // "Typy mocne" EterBase / StrongTypes dla SkillId
```
Nie wykryto cyklicznych zaleznosci.

**Model pamieciowy:**
- Przydzielanie pamieci lezy na obsludze hash mapy `std::unordered_map`. Kod uzywa stalych referencji, unika przesuwania w pamieci po alokacji.
- `CooldownEntry` to czysta, lekka struktura typu *POD/Trivial* (Plain Old Data, dwa 64- i 32-bitowe inty, 12 bajtow = alignment).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
| Nazwa (Klasa/Struktura) | Rola | Wielkosc w pamieci | Zalezna od watku? |
| --- | --- | --- | --- |
| `SkillCooldownTracker` | Zarzadca czasu odnowienia, agreguje cooldowny | Rozmiar muteksu + hash_map | Bezpieczny wielowatkowo (`std::shared_mutex`) |
| `CooldownEntry` | Rekord opisujacy start i czas trwania | 12 bajtow (align do 16B) | Brak stanu, czyste dane |

**Tabela Metod Publicznych (`SkillCooldownTracker`):**
| Sygnatura Metody | Typ Argumentow | Zwraca | Skutki Uboczne / Uwagi |
| --- | --- | --- | --- |
| `StartCooldown` | `uint32_t skillVnum`, `uint32_t durationMs` (oraz warianty dla `std::chrono` / `SkillId`) | `void` | Zapisuje nowy rekord. Uzyskuje wylacznosc Mutexu (`unique_lock`). Usunie rekord przy `durationMs = 0`. |
| `IsOnCooldown` | `uint32_t skillVnum`, [Opcjonalnie `uint64_t currentTimestampMs`] | `bool` | Pobiera wspoldzielony Mutex (`shared_lock`). O(1) odczyt. Zwraca true jesli pomiedzy czasem rozpoczecia a start + trwanie. |
| `GetRemainingCooldownMs` | `uint32_t skillVnum` | `uint32_t` | Pobiera wspoldzielony Mutex (`shared_lock`). |
| `GetCooldownProgress` | `uint32_t skillVnum` | `float` | Pobiera wspoldzielony Mutex. Zwraca miedzy 0.0f a 1.0f. |
| `ResetCooldown` | `uint32_t skillVnum` | `void` | Wylaczny lock. Usuwa z `m_cooldowns`. |
| `ResetAll` | - | `void` | Wylaczny lock. Czysci cala mape. |
| `GetCurrentTimeMs` (static) | - | `uint64_t` | Brak. Zwraca tick z `std::chrono::steady_clock`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Dla bota / hookowania / FFI w Rust (struktura z perspektywy C-API):
```cpp
struct CooldownEntry {
    uint64_t startTimestampMs; // offset: 0x0, size: 8
    uint32_t durationMs;       // offset: 0x8, size: 4
}; // Total size with padding: 16 (0x10)
```

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- Pakiet na ktory to reaguje: GC Skill Cooldown / Cast packets - serwer decyduje, by upewnic sie co do braku desynchronizacji w walce, kiedy uzyta zostaje magia. Sam plik `SkillCooldownTracker.cpp` tego nie wie. Odpowiada za to warstwa nadrzedna, np. dekoder w `src/Client/Network`.

**Metody Pythona (`PyMethodDef`):**
- Sam modul jest odciety od Pythona (Zgodnosc z inicjatywa "C++23 Zero-Conflict"). Moze byc jednak udostepniany interfejsom C-API (mostki PhaseGameBridge) poprzez mapowanie takie jak `player.IsSkillOnCooldown(vnum)` mapujace do instancji `SkillCooldownTracker::IsOnCooldown`. (Do znalezienia w PythonPlayer.cpp albo PlayerStatsDomain.cpp)

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
- Odczyt/Pisanie z `m_cooldowns` chronione jest przez wzorzec "Readers-Writer Lock" (`std::shared_mutex`).
- Czytanie informacji o pasku cooldown (UI render thread) to `std::shared_lock` w `GetCooldownProgress`. To pozwala zablokowac tylko kiedy z sieci lub z eventu ataku zostaje odpalony `StartCooldown` / `unique_lock`.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. Problem: Zapytanie o vnum, na ktorego nie ma rekordu.
   - Rozwiazanie w kodzie: `IsOnCooldown` zwraca false, `GetCooldownProgress` zwraca `1.0f` (100% dostepny). Bardzo odporny edge case.
2. Problem: Trwanie 0 milisekund (`durationMs == 0`).
   - Rozwiazanie: Blok z `durationMs == 0` usuwa hash key calkowicie (traktuje jak Reset). Dzielenie przez 0 w funkcji Progress zabezpieczone jest tez poprzez ten warunek.
3. Problem z cofaniem sie czasu wirtualnego / pauzowaniem / testami.
   - Rozwiazanie w kodzie: Metody sa przeciazone o zewnetrzny `currentTimestampMs` albo `time_point`, co pozwala wstrzykiwac czas (Time Injection) by omijac spanie w testach jednostkowych (sleep).

**Zarzadzanie zasobami (RAII):**
Wyrzucane wyjatki nie groza uszkodzeniem srodowiska (mutex RAII w postaci `std::unique_lock` i `std::shared_lock` zostanie rozwiniety z automatu na bazie Scope, unikniecie Deadlocku). Brak "Goly" wskaznikow - wszystko zyje na wartosci z mapy, brak wyciekow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Przykladowy scenariusz "Wstrzymanie czasu odnowienia" (Pause Cooldown):
1. Dopisz `isPaused` (bool) do `CooldownEntry`.
2. Dodaj metode `PauseCooldown(uint32_t skillVnum)`. W srodku zablokuj przez `unique_lock(m_mutex)`, zmien flage i zapisz, ile aktualnie uplynelo od `startTimestampMs`.
3. Zaktualizuj `GetRemainingCooldownMs` tak, by dla przerwanych, nie dodawala czasu uciekajacego (`GetCurrentTimeMs`).

**Jak debugowac i logowac:**
Z uwagi na bardzo czeste odpytywanie w UI (`GetCooldownProgress` jest czesto wolany co klatke ok 60FPS lub nawet 144FPS), *zdecydowanie odradza sie* wrzucanie logow (np. `ModernLogger::Trace`) do metod czytajacych. Logowanie powinno byc wrzucane TYLKO do `StartCooldown` i `ResetCooldown`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
- Stworz wirtualny test w izolacji.
- Aby zasymulowac przejscie czasu OMIJAJ funcje `std::this_thread::sleep_for(100ms)`. Zamiast tego zrob:
```cpp
uint64_t virtualTime = 1000;
tracker.StartCooldownWithTimestamp(1, 5000, virtualTime);
REQUIRE(tracker.GetRemainingCooldownMs(1, virtualTime + 2500) == 2500); // Polowa minela
REQUIRE(tracker.GetCooldownProgress(1, virtualTime + 2500) == 0.5f);
```
Pozwala to na natychmiastowe testy asynchroniczne i synchroniczne.
