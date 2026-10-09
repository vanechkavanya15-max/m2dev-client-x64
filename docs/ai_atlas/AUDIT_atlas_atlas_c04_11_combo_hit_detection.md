---
task_id: "atlas_c04_11_combo_hit_detection"
cluster: "CBT"
module_name: "Sekwencer Ciosow Combo i Efekt Odrzutu (Knockback)"
target_files:
- src/UserInterface/InstanceControllers/InstanceComboSequence.cpp
- src/Client/Gameplay/CombatDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_11_combo_hit_detection.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: Sekwencer Ciosow Combo i Efekt Odrzutu (Knockback)

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul ten odpowiada za obsluge sekwencji atakow wrecz (combo) oraz fizyke walki (obliczanie trafien, obrazen). Dzieli sie logicznie na dwie warstwy:
- **Warstwa Prezentacji (UI/Animacje):** Reprezentowana przez `InstanceAnimComboController` (w pliku `InstanceAnim_Combo.cpp`). Zarzadza ona maszyna stanow animacji i okienkami czasowymi na wyprowadzenie kolejnego ciosu w serii. Pozwala graczowi "kolejkowac" ataki klikajac w trakcie trwania obecnej animacji. Dziala na podstawie eventow z UI i czasu rzeczywistego (`std::chrono::steady_clock`). Uruchamiana jest cyklicznie w metodzie `Update()`.
- **Warstwa Domeny Walki (Logika):** Reprezentowana przez `CombatDomain` oraz wspolpracujace klasy jak `CombatCalculator`, `AttackGeometry`, i `DamagePresentationQueue` (w plikach `CombatDomain.h`/`CombatDomain.cpp`). Warstwa ta przelicza matematyczna geometrie ataku (zasieg, kat, stozki ataku), prawdopodobienstwa trafienia/uniku oraz obrazenia (w tym efekty statusow: krytyk, przebicie, etc.). 

**Przeplyw danych (Data Flow) i Cykl zycia:**
1. Alokacja: `CreateInstanceAnimComboController()` tworzy kontroler. Obiekty domeny walki czesto maja charakter statycznych funkcji narzedziowych lub singletonow (`CombatDomain::GetCurrentTarget`).
2. Kiedy gracz klika przycisk ataku, wywolywane jest `PlayMotion()`.
3. Jesli wciaz trwa atak (stan `MotionState::Attack`), kontroler sprawdza okno czasowe (`comboWindowMs = 1500ms`). 
4. Jesli czas nie uplynal, atak trafia do kolejki `attackQueue` (limit 4).
5. Podczas klatek renderingu / logiki, metoda `Update()` przetwarza koncowki animacji i zdejmuje ataki z kolejki, odpalajac je plynnie jeden za drugim.
6. Rownelegle, silnik gry wysyla pakiety ataku na serwer, a serwer ewentualnie odpowiada pakietami obrazen. Te trafiaja do `DamagePresentationQueue`, gdzie `displayTime` pozwala synchronizowac pojawienie sie cyferek obrazen z odpowiednim momentem animacji. Zmiany stanu (np. dlugosc kolejki combo) rozglaszane sa przez `Core::EventBus` poprzez `ComboStateChangedEvent`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
**Zaleznosci wejsciowe (Inbound):**
- Systemy odczytujace Input od gracza wywoluja `PlayMotion` lub `BlendMotion`.
- System UI nasluchuje eventow `ComboStateChangedEvent` poprzez `EventBus` w celu zaktualizowania interfejsu np. wskaznika combo.
- `DamagePresentationQueue` konsumuje pakiety z serwera dot. otrzymanych i zadanych obrazen.

**Zaleznosci wyjsciowe (Outbound):**
- `Core::EventBus`: Publikacja zdarzen (`ComboStateChangedEvent`).
- `EterBase::ModernLogger`: Zrzucanie logow (debug i info).
- Narzedzia standardowe C++: `<chrono>`, `<queue>`, `<optional>`, `<mutex>`, `<random>`.
- `EterBase::EntityId` i system Result.

**Drzewo dyrektyw `#include`:**
W `InstanceAnim_Combo.cpp`:
- `../StdAfx.h`
- `IInstanceAnimationController.h`
- `../../EterBase/StrongTypes.h`
- `../../EterBase/Result.h`
- `../../EterBase/ModernLogger.h`
- `../Core/EventBus.h`
- `<queue>`, `<chrono>`, `<optional>`

W `CombatDomain.h`:
- `<cstdint>`, `<vector>`, `<cmath>`, `<mutex>`, `<queue>`, `<optional>`, `<chrono>`
- `<EterBase/StrongTypes.h>`

**Model pamieciowy:**
- Kod korzysta z nowoczesnych wzorcow z C++23. Wskazniki sa opakowywane przez `std::unique_ptr` (np. kontroler w fabryce).
- Parametry przekazywane glownie przez stale referencje `const T&`.
- Wektor zdarzen w UI uzywa `std::vector` i zarzadzany jest poprzez standardowa biblioteke (RAII).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa | Rola | Wlasciciel Watku |
| :--- | :--- | :--- |
| `ComboStateChangedEvent` | Zdarzenie przekazywane przez EventBus informujace o stanie combo. | Dowolny watek publikujacy. |
| `InstanceAnimComboController` | Glowny zarzadca kolejkowania animacji i stanow ataku. (Ok. 48 bajtow + narzut std::queue) | Watek glowny (UI / Logika instancji) |
| `WeaponType` (enum) | Klasyfikacja broni dla obliczen geometrii ataku (Sword, TwoHanded, itp.) | N/A |
| `DamageFlag` (enum/bitfield) | Typy obrazen/skutkow (Critical, Penetrate, Miss, etc.) | N/A |
| `Position3D` | Pozycja w swiecie, metoda dystansu (`DistanceTo`). | N/A |
| `Box3D` | Bounding box (minExtents, maxExtents). | N/A |
| `CombatDamageEvent` | Reprezentacja zadanych obrazen (Attacker, Target, DMG, Flagi, czas wyswietlenia). | Dowolny |
| `AttackGeometry` | Kalkulator zasiegu i lukow ataku dla danego typu broni. | N/A |
| `DamagePresentationQueue` | Kolejka (Thread-safe) do przetrzymywania i odczytywania zdarzen uszkodzen w UI. | Wielowatkowy (uzywa std::mutex) |
| `CombatCalculator` | Obliczanie finalnych obrazen na podstawie statystyk i RNG. | N/A |
| `CombatDomain` | Repozytorium stanu aktualnego celu gracza (VID i HP). | Watek glowny |

### Tabela Metod Publicznych

**InstanceAnimComboController (dziedziczy po IInstanceAnimationController)**
- `EterBase::PacketResult<void> PlayMotion(const MotionConfig& config)` - Dodaje animacje do kolejki lub puszcza od razu, jesli Idle. Zwraca Result. Efekt uboczny: modyfikuje `attackQueue`.
- `void Update()` - Przetwarza element z kolejki combo. Odpalana w kazdej klatce. Zmienia stan i powiadamia EventBus.

**AttackGeometry**
- `static float GetWeaponAttackRange(WeaponType weapon)` - Zwraca promien ataku.
- `static float GetWeaponAttackArc(WeaponType weapon)` - Zwraca kat (w stopniach) przed postacia, gdzie trafienie jest zaliczone.
- `static bool IsInAttackArc(...)` i `static bool CheckTargetHit(...)` - Oblicza kolizje pocisku / ciosu wrecz z hitboxem celu (Box3D).

**DamagePresentationQueue**
- `void EnqueueDamage(const CombatDamageEvent& event)` - Bezpieczne watkowo dopisywanie DMG do kolejki. 
- `std::vector<CombatDamageEvent> PopPresentableDamages()` - Pobiera obrazenia gotowe do wyswietlenia wzgledem `std::chrono::steady_clock::now()`.

**CombatCalculator**
- `static std::pair<int32_t, DamageFlag> CalculateMeleeStrike(...)` - RNG-obliczenia ciosu, uzywa `thread_local std::mt19937`.
- `static std::pair<int32_t, DamageFlag> CalculateSkillStrike(...)` - Analogicznie jak Melee, ale skille pomijaja Unik/Blok.

**CombatDomain**
- `static void UpdateTargetHP(EterBase::EntityId targetVid, uint8_t hpPercent)` - Aktualizuje stan celu (singleton).

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- W klasie `InstanceAnimComboController` wystepuje wewnetrznie:
  - `std::queue<MotionConfig> attackQueue`
  - `std::chrono::steady_clock::time_point lastAttackTime`
  - `MotionState state`
  - `uint32_t currentMotionKey`
  - `float speedMultiplier`
- Enum `DamageFlag` jest bitfieldem - gotowy na bitowe uzycie mask z protokolami sieciowymi.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Kod domeny walki operuje na strukturach typowo wyprowadzanych z opcodow takich jak otrzymywanie obrazen (`CombatDamageEvent` uzywa `attackerId`, `targetId`, `damage`). Klasa `DamagePresentationQueue` czesto stanowic bedzie most pomiedzy Handlerem Sieciowym (odbierajacym pakiet Game->Client z obrazeniami i opoznieniem `displayTime`), a UI gry (ktore konsumuje dane do narysowania EffectText).
- **Python C-API:** Brak bezposredniego bindowania w tych dwoch plikach, ale `ComboStateChangedEvent` zazwyczaj jest lapanym zdarzeniem po stronie zarzadcy UI (np. Python uzywa go do narysowania wskaznika combo i zaktualizowania HP bara celu wg `CombatDomain::UpdateTargetHP`).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Kontroler animacji Combo musi byc uzywany i uaktualniany (metoda `Update()`) WYLACZNIE W WATKU GLOWNYM (zero mutexow we wnetrzu klasy). Z kolei `DamagePresentationQueue` wyraznie chroni dostep zamkiem (`std::mutex`), co pozwala wkladac obrazenia z watku sieciowego (`EnqueueDamage`), a rysowac z glownego (`PopPresentableDamages`).
- **Problemy z timingiem (RNG i std::chrono):** `std::chrono::steady_clock` uzywany jest w obu plikach (okna combo w `InstanceAnim_Combo` oraz displayTime w `DamagePresentationQueue`). W wypadku lagow (skoki ramkowe) system moze pominac okno combo, dlatego nie polega sie na liczbie klatek, a realnym czasie.
- **RNG thread_local:** W `CombatCalculator` generator pseudolosowy inicjowany jest per-thread (`thread_local std::mt19937`), wiec operacje te sa thread-safe, unikamy contention locks na rand().
- **Krucha logika dystansow:** Metoda wyliczajaca hitbox celu (w `AttackGeometry::CheckTargetHit`) opiera sie na znalezieniu najblizszego punktu na AABB. Nalezy ostroznie edytowac Box3D mobow, bo moga one zostac "wepchniete" poza swoj punkt rejestracji obrazen.
- **Zero-Conflict Rule:** Wszystkie interfejsy z `CombatDomain.h` nalezy rozszerzac tak, by zachowac stabilnosc API dla starszych systemow - uzywac opcjonalnych wartosci (`std::optional`).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli dodajesz nowy tym broni (np. Wlocznia), rozszerz enum `WeaponType` w `CombatDomain.h`.
  2. Rozszerz instrukcje `switch` w `AttackGeometry::GetWeaponAttackRange` i `AttackGeometry::GetWeaponAttackArc`.
  3. Skompiluj testy jednostkowe lub headless test bed dla modulu z nowym promieniem.
  4. Pamietaj ze `InstanceAnimComboController` dziala agnostycznie od broni, interesuja go jedynie klucze animacji.
- **Jak debugowac i logowac:** Do przesledzenia zlego timingu, sprawdz `ModernLogger::Debug` pod katem fraz "Combo attack queued" oraz "Combo time window expired". Jesli animacje sie zawieszaja, sprawdz logi "Executing attack motion".
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Utworz mock klasy implementujacej `Core::IEvent`, uruchom sztuczna petle `Update()` z krokiem np 10ms (symulujac now() zwiekszane statycznie), dodaj pare ciosow do `PlayMotion` i upewnij sie, ze queue jest wlasciwie konsumowana bez alokowania renderingu. W wypadku CombatDomain, funkcje statyczne (typu kalkulatory i geometria) mozna bezposrednio wstawiac w `doctest` bazujac czysto na matematyce.
