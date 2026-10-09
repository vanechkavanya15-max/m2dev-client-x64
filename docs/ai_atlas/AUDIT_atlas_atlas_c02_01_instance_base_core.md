---
task_id: "atlas_c02_01_instance_base_core"
cluster: "ACT"
module_name: "CInstanceBase - Rdzen Reprezentacji Bytu w Swiecie Gry"
target_files:
- src/UserInterface/InstanceBase.cpp
- src/UserInterface/InstanceBase.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_01_instance_base_core.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Z Audytu: CInstanceBase - Rdzen Reprezentacji Bytu w Swiecie Gry

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Klasa `CInstanceBase` jest fundamentalnym bytem w kliencie gry Metin2, sluzacym jako glowna warstwa logiki i sterowania (Actor/Entity) dla graczy (PC), potworow i postaci niezaleznych (NPC), kamieni Metin (Stone), wierzchowcow, obiektow, budynkow i bram w swiecie gry. Obejmuje zarzadzanie wizualnym ekwipunkiem (czesci armoru, bron, fryzury), efektami na postacji (buff, particle), logika poruszania sie i interpolacja pozycji (Network Sync) oraz zarzadza aktualnym stanem (State Machine) i emocjami, animacjami (poprzez instancje podlegle np. CActorInstance).

### Przeplyw Danych (Control Flow & Data Flow) i Osadzenie w Petli Gry:
1. **Network Tick / Network Packet Processing:** Modul przetwarza zdarzenia oparte na pakietach sieciowych. Komendy ruchu, ataku i umiejetnosci trafiaja tu z warstwy sieciowej zsynchronizowanej opoznieniem `m_nAverageNetworkGap` (kolejka `m_kQue_kCmdNew` obslugiwana w `PushTCPState` i `StateProcess`).
2. **OnUpdate (Tick Gry):** Glowna funkcja `CInstanceBase::Update()` wykonuje `StateProcess` w oparciu o czas systemowy, obsluguje fizyke (`PhysicsProcess`), uaktualnienie kierunku i logike comba. Aktualizuje rowniez wektor przesuniecia `m_GraphicThingInstance`.
3. **OnDeform (Transformacja szkieletu / Modelowanie):** Zaleznie od LOD i obciecia kamery (`__CanRender`), funkcja `CInstanceBase::Deform()` przelicza wierzcholki geometrii oraz wierzchowca.
4. **OnRender (DirectX):** Funkcje z rodziny `Render()`, `RenderTrace()`, `RenderToShadowMap()`, `RenderCollision()` przekazuja obiekt do faktycznego rysowania na ekranie z odpowiednimi materialami lub podlaczonymi efektami (czapki niewidki - `AFFECT_INVISIBILITY` ukrywaja/odkrywaja particle).

### Cykl Zycia (Lifecycle):
- **Alokacja:** Obiekty sa recznie prealokowane do puli w bloku z `CDynamicPool<CInstanceBase> ms_kPool`. Obiekt inicjalizuje sie przez `CInstanceBase::New()`.
- **Inicjalizacja:** Ustawiana jest struktura `SCreateData` w `Create()`, ktora aplikuje VID, pozycje, rase, ekwipunek, typ (np. gracz, NPC).
- **Dealokacja:** Zwracane do puli poprzez `CInstanceBase::Delete()`, co resetuje zasoby i kasuje powiazania w grafice 3D. Oprozniana jest z pamieci ram struktura efektow.


## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

### Zaleznosci wejsciowe (Inbound):
- `CPythonCharacterManager` - zarzadzajacy instancjami per-VID, wywolujacy instancje klas, np. glowna petla gry per byt (`Update()`, `Deform()`, `Render()`).
- `CPythonNetworkStream` (czesc sieciowa / pakiety `CG`, `GC` zwiazane z postacia, pozycja i ruchem - wola `PushTCPState`, `SetArmor`).
- `CPythonPlayer` / `IAbstractPlayer` (czesc sterowania) weryfikujace typy PK i PvP dla walk i atakow.
- Python Scripts (`CPythonNonPlayer` ladujacy moby). Ewentualne eventy C++ do UserInterface np. `UserInterface::Core::MountStateChangedEvent`.

### Zaleznosci wyjsciowe (Outbound):
- `CActorInstance` (silnik `GameLib`, rendering i ruch samej struktury kosci),
- `CPythonBackground` (pobieranie mapy, uksztaltowania z-os `__GetBackgroundHeight()`),
- `EventBus` w C++23 dla rozglaszania eventow np. do GUI.
- `CItemManager` dla odczytywania rodzajow broni i blyszczenia refine (+7 do +9) po VNUM.
- C++23 Decoupled Components: `UserInterface::InstanceComponents::*` (Wizualne, Fizyka, Walka).

### Drzewo dyrektyw `#include`:
- `<GameLib/RaceData.h>` i `<GameLib/ActorInstance.h>` (Silnik gry, kosci, instancje zasobow 3D).
- `<AffectFlagContainer.h>` (obsluga setek statusow `AFFECT_*` w formacie flag dla postaci).
- Elementy mostkowania `EterBase`, `<Client/Gameplay/CombatError.h>`, `EventBus`.

### Model pamieciowy:
- Customowa kontrola w `CDynamicPool` (Memory Pool pattern). Czyste wskazniki przy pobieraniu (`CInstanceBase*`). Zero pointerow C++11 typu `std::shared_ptr`. Silnik operuje wylacznie na raw pointers dla wysokiej wydajnosci masowych entity.


## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wielkosc w bajtach | Wlasciciel watku |
|-------|------|---------------------|------------------|
| `CInstanceBase` | Glowny byt. Wrappuje stan per postac/potwor. | Zmienna, duzy blok pamieci per entity | Main Thread (Direct3D + Logika) |
| `SCreateData` | Inicjalizator poczatkowy (spawn/warp) parametrow bytu | N/A | Main Thread |
| `SHORSE` | Stan i instance Mounta (konia/dzika) per gracz | N/A | Main Thread |
| `SCommand` | Stan kolejkowania polecen z TCP (Sieciowki) | N/A | Main/Network |
| `UserInterface::InstanceComponents::*` | C++23 separacja logiki walki, kolizji, grafiki. | N/A | Main Thread |

### Tabela Metod Publicznych
| Sygnatura C++ | Wartosc Zwracana | Warunki Wstepne / Skutki Uboczne |
|---------------|------------------|-----------------------------------|
| `bool Create(const SCreateData&)` | `bool` (sukces) | Alokuje obiekt do gry. Rejestruje obiekty D3D. Wywoluje sie podczas odbierania pakietow Spawn/Warp. |
| `void Update()` | `void` | Wolane co klatke. Przetwarza wektory fizyki i uaktualnia rotacje oraz Network `SCommand`. |
| `void PushTCPState(DWORD, const TPixelPosition&, float, UINT, UINT)`| `void` | Buforuje stany przeslane z pakietow, interpolujac opoznienia do lokalnych klatek animacji. |
| `bool IsAttackableInstance(CInstanceBase&)` | `bool` | Weryfikuje tryb PvP, frakcje (Empire), ochrone Safezone przed zadawaniem uderzen i zniszczen. Zwraca TRUE jezeli cel mozna zaatakowac. |
| `void SetArmor(DWORD dwArmor)` | `void` | Oblicza speculary. Aktywuje modyfikacje refinu z `CItemManager` dla czesci korpusu/pancerza postaci. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- Posiada surowe bufory dla partykulow `ms_adwCRCAffectEffect[AFFECT_NUM]`.
- Pole `m_dwLevel`, `m_dwEmpireID`, `m_dwGuildID`.
- Flagowany obiekt bufora efektow statusu (trucizny/buff): `m_kAffectFlagContainer`.
- Pole `m_GraphicThingInstance` zawiera model logiczny bytu (Granny3D i kolizje bryl `CActorInstance`).


## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

### Pakiety Sieciowe
Komunikacja od klienta do serwera i z serwera do klienta przetwarza tu dyrektywy:
- `FUNC_WAIT` (0)
- `FUNC_MOVE` (1)
- `FUNC_ATTACK` (2)
- `FUNC_COMBO` (3)
- `FUNC_MOB_SKILL` (4)
- `FUNC_EMOTION` (5)
- `FUNC_SKILL` (0x80)
Opoznienia sa wyrownywane przez zmienna `m_nAverageNetworkGap` dla gladkiego odtwarzania chodu (Movement Process interpolacja ruchu i blending obrotu). Funkcja `PushTCPState` zarzadza wrzucaniem akcji do kolejki dekompresujacej eventy sieciowe w realna akcje w swiecie.

### Metody Pythona
Mimo iz plik ten to wewnetrzna C++, wiekszosc API (poza nowym `EventBus`) z tej klasy jest ujawniana i opakowywana w plikach takich jak `PythonCharacterManager.cpp` lub `PythonPlayer.cpp`, pozwalajac skryptom Pythona modyfikowac zaznaczenie, widocznosc i efekty.
W systemie UI klasy uzywa sie flag pod maska (np. `AFFECT_INVISIBILITY`, `AFFECT_EUNHYEONG`).


## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystko w `CInstanceBase` jest STRICTLY SINGLE THREADED i dziala w obrebie watku D3D `MAIN THREAD`. Odpytywanie graficzne z innych watkow czy dodawanie akcji na instancji sieciowej grozi fatalnym Race Condition silnika (`m_GraphicThingInstance`). Nowe C++23 eventy (`UserInterface::Core::EventBus`) moga dzialac synchronicznie dla Main Thread.
- **Potencjalne punkty awarii (Nulled Pointers):** `__GetMainActorPtr()` moze zwrocic `NULL` / `nullptr`, jesli gracz jeszcze nie zdazyl zaladowac swojej glownej postaci, a klient przetwarza instancje z zewnatrz (np. widok ze stoiska lub intra). Koniecznosc weryfikacji przez `__IsExistMainInstance()`.
- **Zarzadzanie zasobami:** Usuniecie instancji przez `delete pkInst` wywola krach systemu przydzialu pamieci. Obiekty wyciagane i niszczone MAJA uzywac `CInstanceBase::New()` i `CInstanceBase::Delete()` powiazanych z `ms_kPool`.
- **System Culling & Syncing:** Obiekty moga zniknac z ViewFrustum D3D. Przetwarzanie i wysylanie pakietu w Culling moze prowadzic do out-of-sync dla serwera, dlatego nie modyfikuje sie logiki `m_kAliveInstMap` podczas samego rysowania.


## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

### Instrukcja dodawania nowej funkcji (Step-by-step extension guide):
1. Dodaj stala np. nowa wartosc flagi statusu (buff/debuff) w `CInstanceBase::enum` z sekcji enumeratorow `AFFECT_*`.
2. Odbierz ten pakiet w `CPythonNetworkStream` i zaaplikuj metode do modyfikacji `m_kAffectFlagContainer` w pliku postaci `InstanceBase.cpp`.
3. Dodaj particle (plik `.mse` - particala 3D) do wywolania przy aktualizacji stanu (np. w `__AttachEffect()`).
4. Upewnij sie, ze zachowanie respektuje `Deform()` i renderowanie (np. ukrywanie dla czarnej magii przy czapce niewidce).
5. Zawsze hermetyzuj nowa logike do `UserInterface::InstanceComponents` (np. Visual Component).

### Jak debugowac i logowac:
- W `StateProcess()` ukryty jest preprocesor `Tracenf` logujacy network-gaps. Odkomentuj to do analizy ruchu pakietow.
- Uzyj punktow przerwan (breakpoints) w `Update()`, by sledzic plynnosc wektorow ruchu dla poszczegolnych `m_dwVirtualNumber` (VNUM np. Vnum moba 101).
- Zawsze loguj poprzez system w `EterBase` podajac identyfikator bytu (`GetVirtualID()`).

### Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):
- Modul instancji jest skompilowany z masa struktur D3D w tle (Granny/Metin2). W `tests/` stworz mock object dla `CPythonCharacterManager` i mock `CPythonBackground` w celu analizy `IsAttackableInstance()` sprawdzajac, czy flagi PvE/PvP dzialaja poprawnie dla frakcji.
- Aby spelnialo wymogi unit testow, mockuj D3D jako zera dla funkcji `SetAlphaValue`, omijajac realne call w D3D9. Uzywaj naglowkow pustych/makr.
