---
task_id: "atlas_c02_07_instance_base_event"
cluster: "ACT"
module_name: "CInstanceBase - Zdarzenia Instancji i Przetwarzanie Stanow"
target_files:
- src/UserInterface/InstanceBaseEvent.cpp
- src/UserInterface/InstanceControllers/InstanceStateProcessor.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_07_instance_base_event.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul ten zarzadza obsluga zdarzen oraz przetwarzaniem stanu dla instancji obiektow w grze (postaci graczy, NPC, potworow). Kod zawarty w `CInstanceBase` stanowi most miedzy logika interfejsu klienta, odbieraniem pakietow sieciowych (np. o ruchu czy atakach) a modelem graficznym i animacyjnym postaci reprezentowanym przez `CActorInstance`. `InstanceBaseEvent.cpp` dziala tu wylacznie jako proxy delegujace uchwyty zdarzen do obiektu graficznego (`m_GraphicThingInstance`). Natomiast przetwarzanie stanow i kolejek polecen (ktore rzekomo mialo znajdowac sie w `InstanceStateProcessor.cpp`, lecz fizycznie zaimplementowane jest wewnatrz `CInstanceBase` w pliku `InstanceBase.cpp`) zarzadza kolejka pakietow od serwera (np. FUNC_WAIT, FUNC_MOVE, FUNC_ATTACK) z pomoca metody `PushTCPState` i cyklicznie przetwarza je w metodzie `StateProcess()`, ktora najpewniej wolana jest w glownej petli gry w OnUpdate. Obiekty alokowane sa zazwyczaj przez menedzera instancji (np. PythonCharacterManager), a w ich cyklu zycia stany zmieniane sa na podstawie czasu sieciowego i klatek.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Kod zasilany jest z poziomu `CPythonNetworkStream` i odpowiadajacych mu handlerow pakietow, ktore wywoluja `PushTCPState`. Dodatkowo, petla glowna (`PythonCharacterManager::Update`) najpewniej wola `StateProcess()`. Inne zdarzenia moga byc rejestrowane przez klasy takie jak `CPythonPlayerEventHandler`.
- **Zaleznosci wyjsciowe (Outbound):** Zdarzenia bezposrednio dotykaja glownego obiektu aktora `CActorInstance` (a wlasciwie podklas dziedziczacych np. poprzez `IEventHandler`). Przetwarzanie stanow modyfikuje wlasciwosci wewnetrzne instancji, np. koordynaty renderowania z GameLib (TPixelPosition) i powiazania z systemem efektow czy umiejetnosci (`NEW_UseSkill`).
- **Drzewo dyrektyw `#include`:** W pliku `InstanceBaseEvent.cpp` wystepuja tylko: `"StdAfx.h"` i `"InstanceBase.h"`. W `InstanceBase.cpp` znajduja sie liczne zaleznosci m.in. GameLib, Network. Nie ma tu duzego ryzyka bezposrednich zaleznosci cyklicznych w obrebie obslugi eventow dzieki czystemu interfejsowi `IEventHandler`.
- **Model pamieciowy:** Przede wszystkim operacje na czystych wskaznikach C (np. `CActorInstance::IEventHandler* pkEventHandler`), referencje dla obiektow zwracanych (np. `GetEventHandlerRef`), oraz struktura `SCommand` przechowywana wewnatrz `std::deque` (`m_kQue_kCmdNew`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `CInstanceBase`: Glowna klasa, proxy i maszyna stanow, wlasciciel watku glownego gry.
  - `CInstanceBase::SCommand`: Struktura 32-40 bajtow, trzyma `m_dwChkTime`, `m_dwCmdTime`, `m_fDstRot`, `m_eFunc`, `m_uArg`, `m_uTargetVID`, `m_kPPosDst` (TPixelPosition). Wlasciciel: Main Thread.
  - `CActorInstance::IEventHandler`: Interfejs abstrakcyjny. Klasa bazowa z metodami wirtualnymi (`OnSyncing`, `OnWaiting`, `OnMove`, `OnStop`, `OnAttack`, itp.).
- **Tabela Metod Publicznych:**
  - `CActorInstance::IEventHandler& CInstanceBase::GetEventHandlerRef()`: Zwraca referencje. Brak side-effects.
  - `void CInstanceBase::SetEventHandler(CActorInstance::IEventHandler* pkEventHandler)`: Przypisuje instancje event handlera do `m_GraphicThingInstance`. Skutkiem jest nadpisanie poprzedniego uchwytu.
  - `void CInstanceBase::PushTCPState(...)`: Otrzymuje stan z serwera, updatuje czas, dodaje element do `m_kQue_kCmdNew`.
  - `void CInstanceBase::StateProcess()`: Procesuje elementy z `m_kQue_kCmdNew`. Przewija postac do stanu i ustawia flage ruchu lub wymusza uzycie skilli (`FUNC_MOVE`, `FUNC_WAIT`, `FUNC_ATTACK`).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):** Typowy dla MSVC. SCommand na poczatku posiada DWORDy odpowiedzialne za synchronizacje czasowa. 

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powiazanie opiera sie na stalej architekturze Game-Client, zwlaszcza pakiety zwiazane z ruchami i atakami, w ktorych uzywane sa enumeratory z rodziny FUNC, m.in.: `FUNC_WAIT (0)`, `FUNC_MOVE (1)`, `FUNC_ATTACK (2)`, `FUNC_COMBO (3)`, `FUNC_MOB_SKILL (4)`, `FUNC_EMOTION (5)`, `FUNC_SKILL (0x80)`.
- **Metody Pythona (`PyMethodDef`):** Modul sam z siebie w tych konkretnych plikach nie udostepnia bezposrednio metod dla Pythona, ale stan instancji przetwarzany tutaj jest udostepniany dalej do skryptow za posrednictwem mostkow UI klienta (np. modulu app lub chrmgr) aby pokazywac pozycje/animacje w GUI.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystko to powinno (i operuje) stricte na glownym watku klienta ze wzgledu na integracje z renderem oraz pusta (non-thread-safe) kolejka komend `std::deque`. Operowanie z innych watkow naruszy zasady D3D9 i doprowadzi do crasha.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Gdy instancja ma status IsDead() lub IsKnockDown() (np. zostala zabita z innej komendy w tej samej klatce), nalezy zaprzestac procesowania ruchu zeby uniknac asynchronicznych bugow wyswietlania postaci. Rozlaczony socket powoduje brak wywolan `PushTCPState`, wiec aktor zastyga w bezruchu na podstawie starszego polecenia w kolejce.
- **Zarzadzanie zasobami (RAII):** Brak specjalnych smart-pointerow (RAII nie jest tu aplikowane bezposrednio do event handlera, klasa ktora powoluje obiekt `IEventHandler` musi go usunac). Zle przypisanie pamieci z handlera moze spowodowac dangling pointer. Elementy TCP trzymane sa po wartosci w kolejce wiec brak wyciekow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli nowa funkcjonalnosc dotyczy zmiany stanu z zewnatrz (nowy rodzaj poruszania), trzeba dodac nowy enumarator w enumie `FUNC_*` (np. `FUNC_DASH`).
  2. Dodac obsluge (nowy case) wewnatrz glownego `switch (eFunc)` w `CInstanceBase::StateProcess()`.
  3. Zadbac by wywolywana funkcja logiki nie ladowala postaci w martwy punkt (obowiazkowy powrot w stan oczekiwania - np. przypisanie `m_kMovAfterFunc.eFunc = FUNC_WAIT`).
- **Jak debugowac i logowac:** Do diagnostyki opoznien (desynchronizacji z serwerem), mozna obserwowac zmienna `m_nAverageNetworkGap` i odkomentowac lokalne metody `Tracenf`, szczegolnie blok u gory `PushTCPState`. Uzywaj `Lognf` przy pominietych stanach (jak postac martwa `IsDead()`).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Konieczne bedzie wykorzystanie mocka `IEventHandler`, a dla symulacji pakietow w `StateProcess` mozna sztucznie napelnic kolejke wywolujac wielokrotnie `PushTCPState` podajac narastajacy w czasie `ELTimer_GetServerFrameMSec()` (konieczny mock zegara EterLib). Nie naruszaj `InstanceBaseEvent.cpp`, zaleznie od potrzeb po prostu powolaj dummy handler.
