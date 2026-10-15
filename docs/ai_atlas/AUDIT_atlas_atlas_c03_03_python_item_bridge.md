---
task_id: "atlas_c03_03_python_item_bridge"
cluster: "ITM"
module_name: "CPythonItem - Mostek Przedmiotow na Ziemi i Interakcji"
target_files:
- src/UserInterface/PythonItem.cpp
- src/UserInterface/PythonItem.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_03_python_item_bridge.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul odpowiedzialny za wizualizacje (Update, Render), zarzadzanie instancjami na mapie oraz dzwiekami (drop/use) przedmiotow upuszczonych na ziemie w kliencie gry. Dziala jako manager instancji modeli graficznych reprezentujacych te przedmioty, a takze jako interfejs do zaznaczania ich kursorem (picking) oraz powiazywania ich z nazwiskami wlascicieli (TextTail - ownership timer).
- **Punkt wywolania w petli gry:** 
  - `Update(const POINT&)` wolany w glownej petli GUI / stanu gry `OnUpdate` (animacja modelu, opadanie przedmiotow na ziemie, wyliczanie najazdu myszki).
  - `Render()` wolany w fazie `OnRender` (wyswietlenie modeli 3D, wywolanie blend renderingu).
- **Przeplyw danych:** Z serwera lub akcji uzytkownika nadchodzi event `CreateItem` z VNUM przedmiotu, koordynatami (TPixelPosition), ID wirtualnym (VID - dropVid) oraz czy przedmiot "spada". Modul pobiera `CItemData` uzywajac `CItemManager`, pobiera model, przypisuje normalna terenu (`CPythonBackground`) do wyliczenia orientacji po upadku, odpala dzwiek opadania oraz rejestruje nazwe w `CPythonTextTail`. Pozniej Update oblicza interpolacje spadania, a podniesienie kursorem idzie przez `__Pick` oraz sprawdzanie odleglosci `GetCloseItem`.
- **Cykl zycia (Lifecycle):** Modul oparty na wzorcu CSingleton. Przechowuje struktury `TGroundItemInstance` dla poszczegolnych przedmiotow. Dynamiczna alokacja pamieci realizowana za pomoca `CDynamicPool<TGroundItemInstance> m_GroundItemInstancePool` by uniknac fragmentacji pamieci przy masowym upuszczaniu i znikaniu itemow. Oczyszczanie robione jest w `DeleteItem` i na zakonczenie w `Destroy()`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Obsluga pakietow przez NetworkStream (GCItemGroundAdd, GCItemGroundDel, GCItemOwnership) wolajacych `CreateItem`, `DeleteItem`, `SetOwnership`.
  - Glowny watek obslugi stanow (CPythonApplication / CPythonPhase) wzywa `Update`, `Render`, `Create`, `Destroy`.
  - Skrypty Pythona poprzez PythonPlayer/PythonNetwork bindujace klikniecia myszka wykorzystuja metody umozliwiajace namierzenie wybranego VID do podniesienia.
- **Zaleznosci wyjsciowe (Outbound):**
  - EterGrnLib/ThingInstance: Animacja i renderowanie Granny 3D (`CGraphicThingInstance`).
  - Gamelib/ItemManager: Baza wiedzy o przedmiotach (`CItemData`).
  - EffectLib/EffectManager: Efekt swiecenia przy opadaniu (`CEffectManager::Instance().CreateEffect`).
  - SoundEngine: Odtwarzanie dzwiekow dropu/uzycia.
  - CPythonBackground: Sprawdzanie wysokosci Z oraz wektora normalnego podloza dla kalkulacji nachylenia rzucanego itemu.
  - CPythonTextTail: Etykiety i wlasnosc przedmiotow nad modelami.
- **Drzewo dyrektyw `#include`:** Zaleznosci to m.in. `stdafx.h`, `EterLib/GrpMath.h`, `Gamelib/ItemManager.h`, `EffectLib/EffectManager.h`, `PythonBackground.h`, `pythonitem.h`, `PythonTextTail.h`.
- **Model pamieciowy:** Wskazniki surowe do klas `CItemData` i `CGraphicThing`. Mapy instancji to `std::map<DWORD, TGroundItemInstance*>`. Obiekty TGroundItemInstance recznie zarzadzane w `CDynamicPool`, bez smart-pointerow (ryzyko uzycia po zwolnieniu jezeli iteratory zostana zepsute).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Klasa CPythonItem:**
  - **Rola:** Singleton manager przedmiotow lezacych na ziemi oraz mapowan dzwiekow uzywania.
  - **TGroundItemInstance:** Struktura (ok. ~100-200 bajtow) wewnetrzna, przechowuje:
    - `DWORD itemVnum` - VNUM przedmiotu
    - `D3DXVECTOR3 v3EndPosition`, `v3Center`, `v3RotationAxis`, `D3DXQUATERNION qEnd` - Koordynaty 3D i quaternion rotacji
    - `CGraphicThingInstance ThingInstance` - Glowna instancja EterGrnLib dla geometrii
    - `DWORD dwStartTime`, `dwEndTime` - Timery animacji opadania
    - `DWORD eDropSoundType`, `DWORD dwEffectInstanceIndex`
    - `std::string stOwnership` - nazwa gracza posiadajacego priorytet na podniesienie (odliczana w TextTail)
- **Kluczowe metody publiczne CPythonItem:**
  - `void CreateItem(DWORD dropVid, DWORD itemVnum, const TPixelPosition& groundCoords, bool bDrop=true);` -> alokacja z Poola, dodanie do m_GroundItemInstanceMap, ustawienie EffectManagera i TextTaila.
  - `void DeleteItem(DWORD dropVid);` -> clear ThingInstance i efektow, usuniecie z Poola i Mapy, skasowanie TextTaila.
  - `void Update(const POINT& c_rkPtMouse);` -> puszcza Update w petli na wszystkie instancje (animacja opadania), wylicza m_dwPickedItemID przez __Pick.
  - `bool GetCloseItem(const TPixelPosition& c_rPixelPosition, DWORD* pdwItemID, DWORD dwDistance=300);` -> szuka najblizszego przedmiotu po XY przez macierz odleglosci przy uzyciu optymalizacji DISTANCE_APPROX. Zwraca VID w wskazniku.
  - `void SetOwnership(DWORD dwVID, const char* c_pszName);` -> modyfikuje wlasciciela (string) w instancji oraz zawiadamia CPythonTextTail.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Network):**
  - Ten modul wprost nie konsumuje tcp/ip payloadow, jednak jego funkcje sa punktem koncowym dla handlerow z sieci (z CPythonNetworkStream):
  - `TPacketGCItemGroundAdd` (czesto opcode 0x1B lub zblizony zaleznie od wersji) odpala `CreateItem`.
  - `TPacketGCItemOwnership` (czesto oddzielny pakiet) odpala `SetOwnership`.
  - `TPacketGCItemGroundDel` odpala `DeleteItem`.
- **Python C-API:**
  - Metody klienta nie udostepniaja wprost wskaznikow CPythonItem, ale w `PythonPlayerModule.cpp` znajduja sie bindy uzywajace zapisanego `m_dwPickedItemID` (ustawianego z myszki) do wysylania do serwera zadania podniesienia (`SendClickItemPacket`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Wielowatkowosc:** Modul w pelni operuje w glownym watku gry ze wzgledu na zaleznosci od interfejsu D3D/Granny (`CGraphicThingInstance::Render` i `Deform`) oraz dostepu do UI myszki. Jakakolwiek ingerencja z watku asynchronicznego (siec/wczytywanie) wywola wyjatek D3D lub race condition w `std::map`.
- **Zarzadzanie Pula Pamieci:** Obiekty alokowane przez `m_GroundItemInstancePool.Alloc()`. Zawsze przy zwalnianiu nalezy uzyc `pGroundItemInstance->Clear()` przed `m_GroundItemInstancePool.Free()`, aby poprawnie zniszczyc efekty (EffectManager) i nie zgubic stringow std::string (moga przeciekac wywolania dynamiczne, string SSO itp).
- **Crash Point:** Brak sprawdzenia granic przy `CPythonItem::GetNoGradeNameDataPtr(DWORD dwIndex)`. Jezeli vektor zostal zainicjowany w `BuildNoGradeNameData` poprawnie, jest okay, jesli nie, mozliwy access violation na tablicy. W `CreateItem` jest instrukcja `if(!CItemManager::Instance().GetItemDataPointer... return;)`, ktora poprawnie ratuje przed crashem na niezidentyfikowanym przedmiocie.
- **Szybkie obliczanie odleglosci:** `GetCloseItem` zostalo zoptymalizowane makrem badz inline `DISTANCE_APPROX` uzywajacym matematyki bitowej do pominiecia potegowania i zsqrt. Trzeba byc swiadomym tej zamiany (dokladnosc mniejsza, szybkosc wyzsza).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja rozbudowy (Extending):** 
  - Aby dodac np. swietlisty slup dla itemow rzadkich (rarity): 
    1. W `PythonItem.cpp::CreateItem` odczytaj z `CItemData` wlasciwosc rarity.
    2. Przypisz inny MSE z `CEffectManager::Instance().RegisterEffect` lub podmien `m_dwDropItemEffectID` na per-instance ID. 
    3. Dodaj to ID jako nowe pole do `TGroundItemInstance` by zwalniac je bezpiecznie w `Clear()`.
- **Logowanie i Debugowanie:** Rejestr `Tracenf` uzyty w `SetDropSoundFileName`. Do sledzenia wlasnosci dropu warto zrobic Tracef w `SetOwnership`.
- **Headless Testing (doctest):** Skrajnie trudne bez mockow D3D, gdyz `TGroundItemInstance` posiada membera `CGraphicThingInstance` rzadzacego cala logika pamieci DX/Granny. Konieczne byloby zbudowanie kompletnego interfejsu `MockEterGrnLib` i `MockEffectManager` wg. wzorcow z pamieci (TEST_ENV) by osiagnac stabilny start singletona `CPythonItem`.
