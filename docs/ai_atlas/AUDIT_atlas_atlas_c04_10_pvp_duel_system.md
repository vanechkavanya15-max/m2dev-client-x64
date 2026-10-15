---
task_id: "atlas_c04_10_pvp_duel_system"
cluster: "CBT"
module_name: "Mechanika Pojedynkow PvP i Stany Wrogosci"
target_files:
- src/UserInterface/PythonPlayer.cpp
- src/Client/Gameplay/PvpModeState.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_10_pvp_duel_system.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul "Mechanika Pojedynkow PvP i Stany Wrogosci" odpowiada za sledzenie i walidacje trybow PvP oraz relacji wrogosci (wyzwania, zemsta) miedzy graczami. Plik `PvpModeState.h` dostarcza logike stanow dla poszczegolnych trybow PvP (Peace, Revenge, Free, Protect, Guild) oraz ich warunki walidacji (`CanAttack`), gwarantujac bezstanowosc i ulatwiajac integracje na poziomie domenowym klienta bez bezposredniego powiazania z GUI. Klasa `CPythonPlayer` zarzadza globalnym stanem pojedynkow z perspektywy klienta i zapewnia interfejs Pythona (C-API), umozliwiajac zapamietanie kto wyzwal gracza na pojedynek (`m_ChallengeInstanceSet`) i w stosunku do kogo gracz ma prawo zemsty (`m_RevengeInstanceSet`). Przeplyw sterowania zaklada odbieranie pakietow sieciowych (GC) dotyczacych wyzwan, aktualizacje setow zidentyfikowanych przez `dwVID` i udostepnianie tych informacji systemom ataku i renderowania (np. zmiana koloru nicku w oparciu o stan wrogosci i przynaleznosc do gildii). Cykl zycia obiektow opiera sie o struktury `std::set`, w ktorych alokacja wpisow nastepuje po otrzymaniu zapytania, a reset przy zmianie mapy, smierci lub zakonczeniu walki.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** `CPythonPlayer` jest centralnym managerem wywolywanym przez zdarzenia sieciowe (pakiety GC z `src/UserInterface/PythonNetworkStream`), akcje uzytkownika klikajacego w UI (skrypty Pythona) oraz inne menedzery jak `CPythonCharacterManager`.
- **Zaleznosci wyjsciowe (Outbound):** Kod ten uzywa systemow wewnetrznych EterLib. Wskazania modulu PvpModeState polegaja m.in. na standardowej bibliotece `std::string_view` i `std::out_of_range`. `CPythonPlayer` uzywa wzorca Adaptera by wysylac pakiety (np. poprzez `m_networkAdapter`) oraz powiadamia kontrolery domenowe, jak `m_pkController` czy `m_combatController`.
- **Drzewo dyrektyw `#include`:** `PvpModeState.h` wlacza `<cstdint>`, `<string_view>`, `<stdexcept>`. `PythonPlayer.cpp` za pomoca `StdAfx.h` zalezy od olbrzymiego ekosystemu GUI i Direct3D, w tym event busow, serwisow inwentarza (`IInventoryService`) oraz statystyk. Brak widocznych groznych zaleznosci cyklicznych w samym `PvpModeState.h`.
- **Model pamieciowy:** `PvpModeState` przechowuje tryb jako wartosc (`PvpMode`). `CPythonPlayer` gromadzi identyfikatory postaci jako kopie `DWORD` (czyste UID postaci, co wyklucza uzycie smart pointerow) wykorzystujac plaskie kolekcje `std::set<DWORD>`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

### Tabela Klas i Struktur
| Nazwa | Rola | Wielkosc (bajty) | Wlasciciel watku |
|---|---|---|---|
| `PvpMode` (enum class) | Wyliczenie stanow PvP (Peace, Revenge, Free, Protect, Guild) | 1 | - |
| `PvpModeState` | Przechowuje i waliduje zasady ataku miedzy graczami | 1 | Watek Glowny |
| `CPythonPlayer` | Centralny obiekt zarzadzajacy strefami gildii i relacjami PvP (singleton) | Potezna | Watek Glowny (DirectX / Logika) |

### Tabela Metod Publicznych
| Klasa | Sygnatura C++ | Wartosc Zwracana | Warunki Wstepne / Skutki |
|---|---|---|---|
| `PvpModeState` | `bool SetMode(PvpMode mode)` | `bool` | Sprawdza poprawnosc podanego enuma. Zmienia tryb wlasny; wyrzuca `std::out_of_range` gdy indeks przekroczy `MaxNum`. |
| `PvpModeState` | `bool CanAttack(PvpMode targetMode) const` | `bool` | Okresla czy postac w `currentMode` moze zaatakowac cel z `targetMode` (Peace=false, Free=true). |
| `PvpModeState` | `std::string_view GetModeName() const` | `std::string_view` | Zwraca stala nazwe znakowa dla trybu (np. "Peace"). |
| `CPythonPlayer` | `void RememberChallengeInstance(DWORD dwVID)` | `void` | Przenosi `dwVID` z setu Revenge do Challenge. |
| `CPythonPlayer` | `void RememberRevengeInstance(DWORD dwVID)` | `void` | Przenosi `dwVID` z setu Challenge do Revenge. |
| `CPythonPlayer` | `bool IsChallengeInstance(DWORD dwVID)` | `bool` | Sprawdza czy `dwVID` istnieje w `m_ChallengeInstanceSet`. |
| `CPythonPlayer` | `bool IsRevengeInstance(DWORD dwVID)` | `bool` | Sprawdza czy `dwVID` istnieje w `m_RevengeInstanceSet`. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `PvpModeState` zawiera jedynie pole `currentMode` typu enumeracyjnego (1 bajt). Do podpiecia przez FFI adres zmiennej `currentMode` odpowiada offsetowi `+0x00`.
- Zbiory w `CPythonPlayer` opieraja sie na wezlach RB-tree implementowanych w `std::set`. Ich zaczepy adresowe mozna uzyskac przez zbadanie glownego singletonu za posrednictwem dekompilacji. Pola: `m_ChallengeInstanceSet` oraz `m_RevengeInstanceSet` (zbieraja instancje uzytkownikow pod UID).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powiazane bezposrednio opcody stanowia czesc GC. Na przyklad zapytania PvP i gildyjne obslugiwane przez pakiety odpowiadaja flagom (np. `HEADER_GC_CHARACTER_ADDITIONAL_INFO`, `HEADER_GC_DUEL_START`). Uaktualnianie obszaru gildii powiadamia interfejs uzytkownika poprzez pakiety.
- **Metody Pythona (`PyMethodDef`):** Integracja wykracza poza sam cpp, wywolujac metody w ew. klasach podpietych (np. `BINARY_Guild_EnterGuildArea(i)`, `BINARY_Guild_ExitGuildArea(i)`). Metody C-API (dostepne w modulach rozszerzajacych jak np. player.*) odpytuja glownego klienta korzystajac z `CPythonPlayer::Instance().GetGuildID()`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod uruchamiany i synchronizowany na Glownym Watku Gry. `CPythonPlayer` i `PvpModeState` nie maja wlasnych zamkow `std::mutex`, co wymaga, aby modyfikacje PvP nie wychodzily poza petle zewnetrznego glownego update'u klienta.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Metoda `SetMode` rzuca `std::out_of_range`. Nalezy wylapywac bledy poprzez `try-catch`, gdyz wywolanie go na nieprawidlowym wejsciu uzytkownika na etapie sieciowym grozi Crastem Klienta (Crash To Desktop). Innym punktem jest zarzadzanie instancjami - nalezy usuwac uzytkownikow z pamieci wyzwan, aby nie gromadzic niepotrzebnie PIDow, co mogloby wyczerpac zasoby (memory leakage in sets).
- **Zarzadzanie zasobami (RAII):** `std::set` z `DWORD` jest w 100% bezpieczny od wyciekow (nie wymaga dealokacji VRAM). Struktura automatycznie uwalnia wezly przy niszczeniu gracza (np. na funkcji Clear).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Gdy wprowadzany jest nowy typ PK, zmodyfikuj enum `PvpMode` w `PvpModeState.h` i odpowiednio podwyzsz `MaxNum`.
  2. Rozszerz walidacje `switch` w `GetModeName`, aby wyswietlac poprawny opis.
  3. Skonfiguruj uprawnienia w bloku `CanAttack`.
  4. Dodaj implementacje wizualna: wysylanie powiadomienia do interfejsu (skrypty pythona).
- **Jak debugowac i logowac:** Wyniki logowania dla Pythona dostepne sa poprzez moduly `TraceError` w `CPythonPlayer`. Punkty breakpointow umieszczaj na mutatorach (np. `RememberChallengeInstance`), z podgladem wartosci lokalnego wnetrza `dwVID`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W izolacji nalezy wyekstrahowac `PvpModeState` do harnessu za pomoca Doctest. Jest on struktura POD/Trivially Copyable bez bezposrednich zaleznosci od DX9. `CPythonPlayer` jest ciezej testowac na headless i wymaga mockowania calego srodowiska `EterLib`.

