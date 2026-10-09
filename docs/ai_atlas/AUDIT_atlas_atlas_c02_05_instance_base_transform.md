---
task_id: "atlas_c02_05_instance_base_transform"
cluster: "ACT"
module_name: "CInstanceBase - Transformacje Przestrzenne i Wierzchowce"
target_files:
- src/UserInterface/InstanceBaseTransform.cpp
- src/UserInterface/InstanceControllers/InstanceMountHorseImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_05_instance_base_transform.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CInstanceBase - Transformacje Przestrzenne i Wierzchowce` odpowiada za zarzadzanie pozycja (X, Y, Z), rotacja, punktem skupienia wzroku (LookAt) i kierunkiem patrzenia w przestrzeni 3D dla obiektow postaci klienta (`CInstanceBase`). Dodatkowo odpowiada za nowoczesna obsluge wierzchowcow i koni z uzyciem architektury C++23 w podsystemie `InstanceControllers`. Kod transformacji jest uzywany w kazdej klatce (OnUpdate / OnRender) do interpolacji ruchu oraz w tickach sieciowych dla pakietow aktualizujacych koordynaty, wywolywany czesto bezposrednio z `PythonCharacterModule`.

Nowy decoupling (rozdzial) mechanik wierzchowcow korzysta z interfejsu `IInstanceMountHorseController` (zaimplementowanego czesciowo w plikach takich jak `InstanceMount_Board.cpp` i `InstanceMount_AnimLink.cpp` poniewaz podany plik `InstanceMountHorseImpl.cpp` nie istnieje fizycznie w systemie, a jego logika zaimplementowana jest w podziale na odpowiedzialnosci - MountBoard i AnimLink). Wzorzec SRP zarzadza interpolacja wejscia/zejscia z konia (Boarding) i publikuje asynchroniczne zdarzenia (np. `MountStateChangedEvent`, `MountAnimSyncedEvent`) do systemu `EventBus`.

Zarzadzanie pamiecia w `CInstanceBase` oraz `CActorInstance` zaklada stalosc bytu, natomiast podsystemy modyfikujace (kontrolery) alokowane sa przez `std::unique_ptr` w systemie zarzadzajacym, gwarantujac bezwyciekowe dzialanie w RAII.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- **PythonCharacterModule (CPythonCharacterManager):** Wywoluje metody skryptowe (np. `chrSetPixelPosition`, `chrSetRotation`) bazujac na wywolaniach C-API.
- **Maszyna Stanow (State Machine):** Modul poruszania i sztucznej inteligencji klienta, pakiety aktualizacji synchronizacyjne z serwera.

**Zaleznosci wyjsciowe (Outbound):**
- **CActorInstance (`m_GraphicThingInstance`):** To w niej znajduja sie wlasciwe dane graficzne, na ktorych bazuje `CInstanceBase`. `CInstanceBase` wywoluje bezposrednio `SetCurPixelPosition`, `SetRotation`, `BlendRotation` i `LookAt`.
- **UserInterface::Core::EventBus:** Modernizowana czesc wysyla zdarzenia takie jak `MountStateChangedEvent` i `MountAnimSyncedEvent` aby oddzielic UI.
- **EterBase (Logger / Types):** Uzycie `EterBase::ModernLogger`, `EterBase::PacketResult`, oraz `EterBase::EntityId` dla identyfikacji instancji.

**Drzewo dyrektyw `#include`:**
- `StdAfx.h` (Kluczowy prekompilowany naglowek, ryzyko cyklicznych definicji, wymaga izolacji w testach)
- `InstanceBase.h`
- `PythonBackground.h` (Do pobierania wysokosci mapy: `__GetBackgroundHeight`)
- `IInstanceMountHorseController.h`
- `IInstanceAnimationController.h`
- `EterBase/StrongTypes.h`
- `EterBase/Result.h`
- `EterBase/LogModern.h`
- `Core/EventBus.h`

**Model pamieciowy:**
- Czyste wskazniki uzywane w przekazywaniu pomiedzy zaleznosciami w trybie "weak/borrow" w starym kodzie (`CInstanceBase&`).
- C++23 `std::unique_ptr` uzywany przy tworzeniu instancji kontrolerow (`CreateMountBoardController()`).
- Monolityczne `CActorInstance` przetrzymywane "by value" (jako wewnetrzna zmienna `m_GraphicThingInstance` obiektu `CInstanceBase`).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wielkosc/Stan | Wlasciciel Watku |
|-------|------|---------------|------------------|
| `CInstanceBase` | Glowna nadklasa dla kazdego aktora w grze, wlasciciel danych. | Duzy monolityczny byt (podzielony ostatnio na komponenty) | Glowny watek logiki gry i sieciowy |
| `CActorInstance` | Obiekt graficzny odpowiedzialny bezposrednio za wektory, Granny i czesc DX. | Masywna graficzna klasa | Glowny watek (D3D/Granny) |
| `UserInterface::InstanceControllers::IInstanceMountHorseController` | Interfejs C++23 do obslugi stanu i transformacji przy operacjach jezdzca. | Interfejs / Bezstanowy z natury | Watek glowny |
| `UserInterface::InstanceControllers::InstanceMount_Board` | Nowoczesna implementacja (final) odpowiadajaca za wejscie/zejscie (lerp wysokosci Z). | Mala klasa, pamieta wektory XYZ i VNUM/VID | Watek glowny |

### Tabela Metod Publicznych (`CInstanceBase` Transformacje)
- `void SCRIPT_SetPixelPosition(float fx, float fy)`: Zwraca nic, pobiera wysokosc (fz) tla i wola `NEW_SetPixelPosition`.
- `void NEW_SetPixelPosition(const TPixelPosition&)`: Zmienia bezposrednio aktualna pozycje `m_GraphicThingInstance`. Skutki uboczne: Zmiana macierzy.
- `void NEW_GetPixelPosition(TPixelPosition*)`: Wyciaga biezaca pozycje przez pointer (referencje).
- `void SetRotation(float)` i `BlendRotation(float, float)`: Zmienia kat patrzenia i rotacji plynnie.
- `void NEW_LookAtDestPixelPosition(const TPixelPosition&)`: Obraca obiekt (w osi Z z reguly) na wybrany punkt uzywajac inwersji osi Y (`-c_rkPPosDst.y`).
- `void SetDirection(int dir)`: Rzutuje 8-kierunkowy enum na stopnie bazujac na stalej tablicy `s_dirRot` i odpala `SetRotation` oraz `SetAdvancingRotation`.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `CInstanceBase` ma wazne wydzielone komponenty w C++23 (od gory pominawszy padding):
  - `CActorInstance m_GraphicThingInstance;`
  - `UserInterface::InstanceComponents::InstanceVisualComponent m_visualComponent;`
  - `UserInterface::InstanceComponents::InstancePhysicsComponent m_physicsComponent;`
  - `UserInterface::InstanceComponents::InstanceCombatComponent m_combatComponent;`

- `InstanceMount_Board` trzyma stan interpolacji wejscia:
  - `EterBase::EntityId m_mountVid;` (4/8 bajtow)
  - `uint32_t m_mountVnum;`
  - Floaty transformacji (`m_x, m_y, m_z, m_rot`)
  - Interpolacyjne (`m_interpolationStartHeight, m_interpolationTargetHeight, m_interpolationProgress`)

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
Choc w tych plikach nie ma bezposrednich pakietow, zmiany rotacji (`SetRotation`), wejscie na konia (`Mount`, `Dismount`) dzialaja wskutek pakietow protokolu (GCCharacterUpdate / GCMount). 

**Metody Pythona (`PyMethodDef` w PythonCharacterModule.cpp):**
- `chrSetPixelPosition`: Mapuje na Python (float, float), wola posrednio operacje dla aktualnie wybranego obiektu za pomoca tuple.
- `chrSetRotation`: Ustawia kat patrzenia, dodajac 180.0f modulo 360.0f (odwrocenie w C++ vs Python).
- `chrSetRotationAll`: Zezwala na narzucenie obrotow X, Y, Z (gimbal lock mozliwy).
- `chrSetDirection`, `chrLookAt`.

Uwaga do Pythona (Regula pamieci AI): Zawsze nalezy uzywac funkcji pakujacych/wyluskujacych jak `PyTuple_GetFloat` zamiast standardowego `PyArg_ParseTuple`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
1. Metody z CInstanceBase operujace na D3D lub Granny3D (jak `m_GraphicThingInstance.SetRotation`) MUST run na glownym watku, zeby zapobiec Data Race z faza renderowania.
2. System `EventBus` przy mountach wymusza asynchronicznosc - logika UI lub czasteczek (Particle) nie powina dzialac natychmiast na ten sam tick.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
1. **Dangling Pointers:** W starszym systemie `CInstanceBase& rkInstDst` (np. uzyte w `NEW_LookAtDestInstance`) jesli wyluskana instancja przestanie istniec podczas lotu kamery / skryptu, spowoduje Segmentation Fault.
2. **Missing Files:** Plik `InstanceMountHorseImpl.cpp` jest zgloszony jako target analizy, lecz jest **NIEISTNIEJACY** fizycznie na dysku z powodu prawdopodobnie refaktoryzacji, i zostal zmapowany wewnatrz mniejszych plikow `InstanceMount_Board.cpp` i `InstanceMount_AnimLink.cpp`. Agenci nie powinni podmieniac, a logowac brak!
3. Brak uwzglednienia -Y przy `LookAt`: w grze os Y zachowuje sie odmiennie (czesto `LookAt(x, -y)`). Nalezy o tym pamietaac piszac narzedzia bota.

**Zarzadzanie zasobami (RAII):**
Mount controllery operuja w paradygmacie C++23 zwracajac z interfejsu obiekty bledow `EterBase::PacketResult<void>` lub opakowujac sie w `std::unique_ptr<IInstanceMountHorseController>`. Zawsze stosowac `EterBase::MakeError(ErrorCode)`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. Zdefiniuj nowa metode przestrzenna (np. `SetCameraAttachment`) w naglowku `InstanceBase.h`.
2. Zaimplementuj cialo w `InstanceBaseTransform.cpp` dbajac by transformacja byla oddelegowana do `m_GraphicThingInstance`.
3. Jesli ma to byc nowa funkcjonalnosc dla wierzchowcow - wykreuj nowy plik `InstanceMount_TwojaFunkcja.cpp` zamiast modyfikowac bazowe obiekty (Zasada Zero-Conflict). 
4. Opublikuj nowe interfejsy przez `EventBus` aby UI odswiezylo stan (nigdy nie uzywaj na sztywno funkcji UI wewnatrz tych klas).
5. Wyeksportuj opcjonalnie bindingi w `PythonCharacterModule.cpp` stosujac wbudowane narzedzia `PyTuple_Get`.

**Jak debugowac i logowac:**
Uzywaj `EterBase::ModernLogger::Info` (oraz Error i Warning). Nie zmyslaj "Debug" jako lewelu logowania.

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Stworz sztuczne mocki struktur `IDirect3D*` minimalnymi dummy structami bez dziedziczenia w wydzielonym `StdAfx.h` i testuj metody klasycznymi wektorami bez urachamiania DirectX 9 (Headless).
