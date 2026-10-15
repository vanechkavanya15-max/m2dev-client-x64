---
task_id: "atlas_c02_12_chrmgr_lifecycle"
cluster: "ACT"
module_name: "CPythonCharacterManager - Fabryka Instancji i Rejestr Swiata"
target_files:
- src/UserInterface/PythonCharacterManager.cpp
- src/UserInterface/PythonCharacterManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_12_chrmgr_lifecycle.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

CPythonCharacterManager pelni role glownego menedzera cyklu zycia dla wszystkich aktorow (postaci graczy, NPC, potworow) w kliencie gry Metin2. Modul ten stanowi pomost pomiedzy warstwa sieciowa (gdzie pojawiaja sie nowe VIDy), silnikiem renderujacym (Granny 3D / DirectX 9) a systemami logicznymi nowej generacji (C++23 ECS, przestrzenny SpatialHashGrid, ActorRegistry).

- **Funkcja:** Zarzadza cyklem zycia (alokacja, update, renderowanie, destrukcja) obiektow `CInstanceBase`, utrzymuje mape zywych aktorow (`m_aliveActorsMap`) i przetwarza ich interakcje (kolizje, klikniecia, odleglosci).
- **Miejsce w petli gry:** Glowna aktualizacja zachodzi podczas OnUpdate (metoda `Update()`), ktora aktualizuje transformacje aktorow, czysci oddalone obiekty i synchronizuje pozycje. Renderowanie odbywa sie w fazie OnRender przez wywolania `Render()`, `RenderShadowMainInstance()` oraz delegacje do `InstanceSceneManager`.
- **Control & Data Flow:** Zdarzenia z serwera (np. nowy obiekt na radarze) wywoluja `CreateInstance` lub `RegisterInstance`, co powoduje utworzenie wewnetrznego `CInstanceBase`. Nastepnie obiekt jest rejestrowany w `ECSWorldRegistry`, `ActorRegistry` oraz `SpatialHashGrid`. W `Update()` odleglosc miedzy aktorem a `m_mainActor` jest stale mierzona; przekroczenie `CHAR_STAGE_VIEW_BOUND` uruchamia usuwanie instancji (`__DeleteBlendOutInstance`), emitujac jednoczesnie asynchroniczne powiadomienie przez `EventBus` i czyszczac ECS.
- **Cykl zycia (Lifecycle):**
  1. *Alokacja/Inicjalizacja:* `CreateInstance` -> `RegisterInstance` -> `CInstanceBase::New()`. Rejestracja we wszystkich podsystemach (ECS, siatka przestrzenna, menedzer sceny).
  2. *Aktualizacja:* `Update` iteruje po aktorach, wola `CInstanceBase::Update`, uaktualnia transformacje.
  3. *Dealokacja/Reset:* `DeleteInstance` lub `__DeleteBlendOutInstance` wyrejestrowuje podmioty, przenosi do cmentarza (`InstanceSceneManager::MoveToDead`), a fizyczne usuniecie wywoluje `CInstanceBase::Delete()`. Przy restarcie srodowiska (np. wylogowanie) uzywane jest `Destroy()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** 
  - Systemy faz gry (`PythonNetworkStreamPhaseGame.cpp`, np. odbieranie pakietow tworzenia NPC).
  - Skrypty Pythona (przez API udostepnione w Python API eksportujace funkcje `chrmgr`).
  - System UI (np. namierzanie kursorem - pick, tekst nad glowa - text tail).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `CInstanceBase` (glowny aktor, renderowanie, logika).
  - `Client::World::ActorRegistry` (nowa baza aktorow domenowych).
  - `Client::World::SpatialHashGrid` (optymalizacja wyszukiwania przestrzennego).
  - `UserInterface::InstanceSceneManager` (rysowanie aktorow, oswietlenie i cienie).
  - `UserInterface::Core::EventBus` (wzorzec wydawca-subskrybent np. dla `ActorDeadEvent`).
  - `UserInterface::ECS::ECSWorldRegistry` (system architektoniczny SoA / C++23).
  - `CEffectManager` i `CPythonBackground`.
- **Drzewo dyrektyw `#include`:** 
  - Gorsze zaleznosci (tradycyjne naglowki): `"stdafx.h"`, `"PythonBackground.h"`, `"PythonNonPlayer.h"`, `"AbstractPlayer.h"`, `"packet.h"`.
  - Zaleznosci domenowe: `"ECS/ECSWorldRegistry.h"`, `"Core/EventBus.h"`, `"World/ActorRegistry.h"`, `"World/SpatialHashGrid.h"`.
  - Potencjalne ryzyko zaleznosci cyklicznych miedzy `CPythonCharacterManager`, a systemem zdarzen lub UI tekstow; rozwiazane przy pomocy `IAbstractCharacterManager`.
- **Model pamieciowy:** Hybrydowy. W strukturach `std::map<DWORD, CInstanceBase *>` wciaz utrzymywane sa nagie wskazniki C (`CInstanceBase*`). Jednak zarzadzanie pamiecia jest recznie kontrolowane via `CInstanceBase::New()` i `CInstanceBase::Delete()`. Podsystemy (C++23) dzialaja niezaleznie opierajac sie o stos badz prealokowane areny.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur
- `CPythonCharacterManager`: Klasa glowna, dziedziczaca z `CSingleton`, `IAbstractCharacterManager` i `IObjectManager`. Wlasciciel glownego watku (gra/render). 
- `CharacterIterator`: Klasa wewnetrzna iterujaca po mapie zywych aktorow.
- `TCharacterInstanceMap`: `std::map<DWORD, CInstanceBase *>` - glowny slownik VID -> Aktor.

### Metody Publiczne (wybrane, kluczowe dla AI)
- `CInstanceBase* CreateInstance(const CInstanceBase::SCreateData& c_rkCreateData)`
  - Rejestruje instancje; zwraca nagiego pointra. Aktualizuje ECS i siatke przestrzenna. Side-effect: Modyfikuje `m_aliveActorsMap`.
- `void DeleteInstance(DWORD dwDelVID)`
  - Usuwa instancje wedlug VID. Wyrejestrowuje ze wszystkich gridow ECS i aktorow. Side-effect: Czysci wewnetrzne wskazniki (main, picked, bound).
- `void Update()`
  - Glowna petla aktualizacji menedzera, waliduje dystanse, wola update w `CInstanceBase`, i obsluguje usuwanie aktorow za horyzontem (`CHAR_STAGE_VIEW_BOUND`).
- `Core::Result<CInstanceBase*, Core::ActorError> GetInstanceResult(DWORD VirtualID)`
  - Nowoczesny (C++23) getter wykorzystujacy `std::expected` (alias `Core::Result`) umozliwiajacy bezpieczne zarzadzanie brakujacym aktorem.
- `CInstanceBase* GetCloseInstance(CInstanceBase * pInstance)`
  - Zwraca najblizsza klikalna instancje uzywajac optymalizacji przez `SpatialHashGrid::QueryRadius`.
- `void __DeleteBlendOutInstance(CInstanceBase* pkInstDel)`
  - Ustawia instancje do zanikania (fade) i powiadamia event bus.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- Wskazniki w menedzerze: `m_mainActor`, `m_pickedActor`, `m_boundActor`. Offsety nie powinny byc hardkodowane w botach poniewaz dodano nowe struktury podsystemow jak `m_actorRegistry` i `m_spatialGrid`. Nowa referencja dla botow powinna przebiegac przez obiekty domenowe (ECS), a nie bezposrednio CPythonCharacterManager (chyba, ze przez proxy pickera).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe (Network Packets):**
  - Menedzer jest uzywany bezposrednio podczas operacji na pakietach sieciowych, zwlaszcza od serwera do klienta (GC) m.in. przy pakietach `HEADER_GC_CHARACTER_ADD` czy aktualizacjach polozenia `HEADER_GC_CHARACTER_UPDATE`. Pakiety te tlumaczone sa na struktury w `PythonNetworkStreamPhaseGame`, ktore formatuja `CInstanceBase::SCreateData` i posylaja do `CreateInstance`.
- **Python C-API:**
  - Sam menedzer eksponuje swoj stan i dzialania do interfejsu graficznego przez oddzielny plik API w ktorym znajduje sie modul (np. `chrmgr`). Menedzer wykonuje wazna prace delegacji m.in w funkcjach pobierajacych VID glownego gracza, VID aktora na ktorego patrzymy (picking z uzyciem celownika). 

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Menedzer MUSI dzialac (odczyt/zapis) wylacznie z poziomu watku glownego silnika (Game Loop), ze wzgledu na niefortunne wiazania bezposrednio z iteratorem DX (np. `CCameraManager`) i modyfikacje macierzy oraz wyluskania podsystemu interfejsu.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Nullowe wskazniki przy usuwaniu; instancja przypisana jako `m_boundActor` lub `m_mainActor` musi zostac odznaczona (odbywa sie to w `DeleteInstance`, ale nalezy na to uwazac w operacjach asynchronicznych).
  - Skrajny przypadek: Usuniecie glownego aktora bez de-rejestracji. Powoduje brak mozliwosci wylaczenia `CHAR_STAGE_VIEW_BOUND` check i potencjalne zwisy przy update transformacji.
  - Oczekiwanie instancji. W starych wywolaniach uzywac sprawdzenia `!pointer`, w nowych funkcja `GetInstanceResult` gdzie wyciagamy `.has_value()`.
- **Zarzadzanie zasobami (RAII):**
  - Kategorycznie unika sie `new CInstanceBase`. Zamiast tego zaimplementowana jest fabryka pamieci przez `CInstanceBase::New()`. Destrukcja przechodzi przez `Delete()`. To pozwala na customowe alokatory, ale boty nie moga polegac na RAII przy `CInstanceBase`. Nowe moduly bazujace na `std::expected` wymuszaja poprawna obsluge bledow zamiast tradycyjnego `nullptr` checka.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji:**
  1. Jesli nowa funkcja wplywa na system ECS (np. modyfikator szybkosci ataku per klatka), upewnij sie, ze rejestracja nowej statystyki przebiega przy operacji `CreateInstance` podczas wypychania atrybutow do `ECSWorldRegistry`.
  2. Implementujac funkcje selekcji (Picking) bezposrednio w 3D, zbadaj czy zasieg wyszukiwania mozesz zoptymalizowac przez `m_spatialGrid.QueryRadius()` zamist iterowac przez cala `m_aliveActorsMap` (jak to zrobiono w `GetCloseInstance`).
  3. Zachowuj regule ZERO-CONFLICT. Modyfikacje w tym module ogranicz do minimium ze wzgledu na wysokie ryzyko naruszenia ciaglosci gry. Doprowadzaj parametry zewnetrznie przez Event Bus lub ECS System.
- **Jak debugowac i logowac:** Uzywaj nowej struktury loggera `EterBase::ModernLogger::Debug()` (nie legacy `TraceError`) do logowania akcji bez modyfikatorow C-style `%d`. Szukaj powiadomien na ekranie po fladze umozliwiajacej wgranie `IsCacheMode()`. Punkty przerwania najlepiej stawiaj w `CreateInstance` lub `__DeleteBlendOutInstance`.
- **Jak testowac bez interfejsu graficznego (Headless):** Menedzer odlacza interfejs 3D za pomoca Mockow w testach (m.in przez `#ifndef TEST_MOCK_D3D9`). Przy uzyciu EventBusa, mozna mockowac odpowiedz z `CPythonCharacterManager` i nasluchiwac emitowanego `ActorDeadEvent`, co weryfikuje czyszczenie rejestru bez odpalania faktycznego rendera instancji na oknie DirectX.
