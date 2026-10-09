---
task_id: "atlas_c02_03_instance_base_battle"
cluster: "ACT"
module_name: "CInstanceBase - Stan Walki, Celowanie i Uderzenia"
target_files:
- src/UserInterface/InstanceBaseBattle.cpp
- src/UserInterface/InstanceControllers/InstanceBattleControllerImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_03_instance_base_battle.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CInstanceBase` w pliku `InstanceBaseBattle.cpp` stanowi pomost miedzy silnikiem wizualnym i graficznym (`CActorInstance`) a warstwa logiki rozgrywki, sieci i interfejsem Pythona, wylacznie w kontekscie walki. Zarzadza on glownie logika ataku: od sprawdzania warunkow ataku (`CheckAttacking`, `CanAttackHorseLevel`, `IsAttackableInstance`), poprzez obliczanie dystansu do celu (np. dla lotu pociskow), po fizyczne inicjowanie atakow (normalnych, combo i uzywania umiejetnosci) z uwzglednieniem kolizji miedzy obiektami i terenem (`CheckAdvancing`).

Mimo obecnosci nowoczesnych komponentow wydzielonych z monolitu (`InstanceCombatComponent`), analizowany plik zrodlowy wciaz odpowiada za wazne zadania, takie jak obsluga animacji uderzen i stanu (np. smierc, omdlenie - `Stun`, `Die`). Odpytuje obiekty docelowe by uniknac strzalu w martwe cele (`TargetDead`) badz w bezpiecznej strefie (`InvalidTarget`). 
Cykl wywolan: Logika walki opiera sie na interakcjach z myszka, eventach sieciowych, po ktorych postac kieruje sie w strone celu i wlacza wywolania z `InstanceBaseBattle.cpp` takie jak `NEW_AttackToDestInstanceDirection`. Metody te nastepnie wydaja komendy `InputComboAttackCommand` / `InputNormalAttackCommand` lub animacje umiejetnosci dla wewnetrznego `CActorInstance`.

*Uwaga:* Plik `src/UserInterface/InstanceControllers/InstanceBattleControllerImpl.cpp` podany jako docelowy nie istnieje bezposrednio, ale czesc funkcjonalnosci walki zostala zaadoptowana przez `PlayerCombatController.cpp` lub jest nadal w `InstanceBaseBattle.cpp`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Obiekty zarzadzania graczem i sterowaniem (np. `CPythonPlayer`, `PlayerCombatController`) wywoluja ataki i sprawdzaja zasieg (np. `NEW_IsClickableDistanceDestInstance`).
- Komunikaty i zdarzenia UI Pythona i myszki (np. `OnMouseLeftButtonDown`).
- Zaleznosci sieciowe: Obsluga pakietow obrazen (`NetCombatRouter`, `PhaseGamePacketDispatcher`), gdzie uzywane sa np. wywolania dotyczace uderzen i synchronizacji.

**Zaleznosci wyjsciowe (Outbound):**
- `CActorInstance` z `GameLib` - obsluga wlasciwej animacji ataku, testowania kolizji, zderzen, graficznej reprezentacji (`m_GraphicThingInstance`).
- `CPythonCharacterManager` - pobieranie referencji do innych jednostek (np. w `AttackProcess`).
- `CPythonBackground` - sprawdzanie kolizji atrybutow terenu (np. blokady) przez `isAttrOn`.
- `EterBase::Result` i enumeratory `Client::Gameplay::CombatError` do czystego zarzadzania bledami domenowymi.
- Zdarzenia sieciowe i efekty specjalne (`__AttachEffect`).

**Drzewo dyrektyw `#include`:**
- `StdAfx.h` (Precompiled headers).
- `EterBase/LogModern.h` (Logowanie nowoczesne C++23).
- `InstanceBase.h` (Definicja glownej klasy `CInstanceBase`).
- `PythonBackground.h` (Teren).
- `PythonCharacterManager.h` (Zarzadca postaci na ekranie).
- `PRTerrainLib/Terrain.h` (Typy atrybutow terenu).

**Model pamieciowy:**
Obiekty zarzadzane w tej warstwie to zazwyczaj referencje do innych obiekotw `CInstanceBase` oraz komunikacja przez referencje (`CInstanceBase& rkInstVictim`). Uzycie `CPythonCharacterManager::Instance()` korzysta z bezposrednich wskaznikow do istniejacych w pamieci jednostek. Plik zawiera czyste wskazniki C przy iterowaniu po widocznych characterach (`CInstanceBase* pkInstEach=*i;`).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Metod Publicznych w `CInstanceBase` (Z kontekstem z `InstanceBaseBattle.cpp`):**
| Sygnatura Metody | Argumenty | Wartosc Zwracana | Pre-conditions | Skutki Uboczne / Uwagi |
|------------------|-----------|------------------|----------------|------------------------|
| `CheckAttacking` | `CInstanceBase& rkInstVictim` | `EterBase::Result<void, CombatError>` | Cel musi istniec i byc prawidlowy (nie martwy). | Zwraca blad domenowy (std::unexpected/EterBase::Result) w razie niemozliwosci zaatakowania (np. Stun, SaveZone). |
| `NEW_Attack` | `float fDirRot` (opcjonalnie) | `void` | Postac nie moze byc w stanie smierci, stun'u czy lezenia. | Zatrzymuje chodzenie (`EndWalking`), przerywa nawigacje (`m_isGoing = FALSE`), wlacza animacje ataku combo/normal. |
| `NEW_UseSkill` | `UINT uSkill, UINT uMot, UINT uMotLoopCount, bool isMovingSkill` | `bool` | Podobnie, brak negatywnych stanow (Stun, KnockDown, Dead). | Ustawia rotacje, przypisuje animacje skilla na aktorze. Przy niewidzialnosci nie odtwarza animacji (`InterceptOnceMotion` pomijane). |
| `CheckAdvancing` | `void` | `BOOL` | Wykonywane w trackie ataku/ruchu. | Zwraca TRUE jesli napotka sciane/drzwi/inna postac (wylaczajac celowe skille). Blokuje ruch (`BlockMovement`). |
| `NEW_IsClickableDistanceDestInstance` | `CInstanceBase& rkInstDst` | `bool` | `rkInstDst` istnieje. | Zalezne od broni postaci glownej (`GetWeaponType`), dobiera wlasciwy zasieg (2.5f m dla mieczy, wiekszy dla lukow). |
| `ProcessHitting` | `DWORD dwMotionKey, CInstanceBase* pVictim` | `void` | `pVictim` istnieje. | Obecnie puste funkcje, dzialanie przeniesione gdzie indziej (Asertywna pulapka: `assert(!"-_-" && ...)`). |
| `Die` / `Revive` / `Stun` | `void` | `void` | Zalezne od akcji. | `Die` - odpina konia, czysci afekty, zdejmuje selekcje/targetowanie i wola `m_GraphicThingInstance.Die()`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Instancja klasy `CInstanceBase` przechowuje wiele pod-modulow. Na potrzeby analizy logiki bitewnej:
- `m_GraphicThingInstance` (`CActorInstance`): Glowny obiekt zarzadzajacy widzialnoscia i silnikiem granny, wywolywany bardzo czesto (offset na samym poczatku struktury `CInstanceBase` w naglowku bazowym).
- `m_isGoing` (BOOL): Kontroluje, czy postac podaza za ruchem.
- `m_dwLastDmgActorVID` (DWORD): Przechowuje VID ostatnio zaatakowanego lub atakujacego (celowy VID po iteracji uzytkownika).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- `TPacketGCAttack`, `TPacketGCDamageInfo`, `TPacketGCCreateFly` - obslugiwane przez routery sieciowe np. `NetCombatRouter` co prowadzi do eventow typu `CombatAttackDomainEvent`. Plik `InstanceBaseBattle.cpp` nie dekompresuje bezposrednio tych pakietow, ale reaguje na zdarzenia (lub stan wykreowany przez te pakiety) w systemie (np. wywolywane z `PythonNetworkStreamPhaseGameCombat`).

**Metody Pythona (`PyMethodDef`):**
Ten plik sluzy jako backend C++. Funkcje te sa eksponowane bezposrednio w klasie `CPythonPlayer` albo w wrapperach instancji (`CPythonCharacterManager`) jako np. `player.SetAttackKeyState(True)`, co posrednio wywoluje `CInstanceBase::NEW_Attack` u glownej instancji postaci.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
Plik, podobnie jak wiekszosc warstwy wizualnej klienta, operuje w glownym watku gry (Main / DirectX thread). Sprawdzanie terenu (CBackground) oraz modyfikacje widzialnych encji (CCharacterManager) nie sa chronione mutexami i MUSZA byc z tego powodu wykonywane sekwencyjnie.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Dangling Pointers przy Iteracji:** W `CheckAdvancing` czy `AttackProcess` petla iteruje po wszystkich postaciach w `rkChrMgr`. Nalezy wczesniej sprawdzic, czy instancja docelowa (`pkInstEach`) zostala zaladowana (z reguly gwarantowane przez manager).
2. **Crash w ProcessHitting:** Aktualnie wywolywanie `CInstanceBase::ProcessHitting` rzuca `assert(!"-_-" && "CInstanceBase::ProcessHitting")`. Agenci **MUSZA OMIJAC** korzystanie z tej przestarzalej metody, logika zostala wyprowadzona na zewnatrz lub do `InstanceCombatComponent`.
3. **Zaleznosci kolizji terenu:** Wywolanie `CheckAdvancing` dzieli sciezke (wektor) na stepy 10-jednostkowe. Moze nieprawidlowo dzialac przy bardzo wysokich Movement Speedach lub w sytuacjach skokow (zbyt dlugie wektory moga ominac siatke terenu).

**Zarzadzanie zasobami (RAII):**
Obiekty i celowniki latajace (`FlyTargetInstance`) wewnatrz walki wymagaja czyszczenia, co dzieje sie glownie w `ClearFlyTargetInstance()`. W przypadku smierci obowiazkowo nalezy zwolnic wierzchowca przez `__DetachHorseSaddle()` (co tez robi metoda `Die()`).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
Jesli potrzebujesz zmodyfikowac zasady dzialania walki badz ataku w poblizu:
1. Nie ruszaj starego kodu, jesli mozesz napisac komponent poboczny w `InstanceCombatComponent`. 
2. Jesli implementujesz modyfikator sprawdzania obrazen albo warunku - uzyj `EterBase::Result<void, Client::Gameplay::CombatError>` i dodaj ewentualny nowy enumerator w `CombatError.h` zamiast uzywac wartosci `bool` z magicznymi wartosciami.
3. Gdy dodajesz animacje nowego ataku, sprawdz, czy `NEW_UseSkill` / `NEW_Attack` to obsluzy. Musisz wprowadzic nowy kod `if (IsNewMode()) { InputComboAttack(fDirRot); }`.

**Jak debugowac i logowac:**
Uzywaj `EterBase::ModernLogger::Debug()` lub `Warn()`. Istnieje juz poprawny kod formatujacy: `EterBase::ModernLogger::Warn("Attack Process declined: {}", Client::Gameplay::ToString(attackResult.error()));`. Przydatne do debugowania kolizji - wylacz zakomentowanie `Tracenf("%x VID %d ... ")` w `CheckAdvancing`.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Aby testowac `CheckAttacking`, nalezy:
- Zmockowac instancje obroncy `rkInstVictim` (`MockCInstanceBase` wywodzace sie z IInstanceBase, lub zmontowac puste CInstanceBase).
- Podmienic flagi takie jak `m_isStunned` w `InstanceCombatComponent`, badz wywolac `Stun()`.
- Uzyc asserta `EXPECT_FALSE(result.has_value()); EXPECT_EQ(result.error(), TargetDead);`. Nie modyfikowac wnetrza petli glownej do testowania.
