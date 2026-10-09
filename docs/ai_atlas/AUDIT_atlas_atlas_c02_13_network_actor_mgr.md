---
task_id: "atlas_c02_13_network_actor_mgr"
cluster: "ACT"
module_name: "CNetworkActorManager - Kolejka Oczekujacych Aktorow i Resolucja Spawnow"
target_files:
- src/UserInterface/NetworkActorManager.cpp
- src/UserInterface/NetworkActorManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_13_network_actor_mgr.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# CNetworkActorManager - Raport Audytu (Jules Swarm)

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

`CNetworkActorManager` pelni role bufora i glownego wezla koordynacyjnego pomiedzy warstwa sieciowa (odbierajaca pakiety od serwera) a menedzerem obiektow wizualnych i fizycznych w kliencie gry (`CPythonCharacterManager`).
W klasycznej architekturze Metin2 zjawisko "Spawnu" (pojawienia sie postaci, potwora czy NPC) zalezy od wielu pakietow (m.in. powiadomienie o dodaniu, a potem powiadomienie o dodatkowych statystykach czy ekwipunku - `CharAdditionalInfo`). 
`CNetworkActorManager` pozwala na poprawne zebranie wszystkich parametrow poprzez kolekcjonowanie w `SNetworkActorData` bez obciazania logiki gry renderowaniem pol-kompletnych bytow i radzi sobie z desynchronizacjami miedzy logowaniem a pelnym obrazem stanu gry.

Przeplyw danych:
1. **Pakiety Sieciowe (Network Stream / Dispatchers):** System sieciowy (`ActorAddHandler`, `CharAdditionalInfoHandler`, `ActorDelHandler`) odbiera pakiety z serwera.
2. **Aktualizacja Menedzera:** Pakiety te tlumaczone sa na modyfikacje globalnego menedzera `CNetworkActorManager` przez metody `AppendActor`, `UpdateActor`, `RemoveActor` czy `MoveActor`.
3. **Stan Wewnetrzny:** Stan ten jest zachowywany w `m_kNetActorDict` - mapie aktorow oczekujacych oraz stworzonych.
4. **Petla Aktualizacji:** W klasycznym trybie, funkcja `Update()` weryfikuje logike (jak np. usuwanie aktorow niewidocznych) a przy aktywacji aktora wysyla sygnal do `CPythonCharacterManager`, gdzie nastepuje wlasciwa alokacja fizyczna/graficzna przez `CreateInstance`.

Cykl zycia obiektu to:
Zdefiniowanie przez pakiet -> Umieszczenie w `m_kNetActorDict` -> Stworzenie w `CPythonCharacterManager` (`CInstanceBase`) -> (Potencjalna wielokrotna synchronizacja pozycji/wygladu) -> Zgloszenie usuniecia i destrukcja poprzez `RemoveActor` lub opuszczenie pola widzenia.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
*   Modul ten modyfikowany jest przez handlery zdarzen sieciowych:
    *   `Network::Handlers::HandleActorAdd` (ActorAddPacket -> `AppendActor`)
    *   `Network::Handlers::HandleCharAdditionalInfo` (TPacketGCCharacterAdditionalInfo -> `UpdateActor`)
    *   `Network::Handlers::HandleActorDel` (TPacketGCCharacterDelete -> `RemoveActor`)
*   Dawniej: `CPythonNetworkStream` i dedykowane dispatchery sieci.
*   Zarzadza tym instancja, czesto jako Singleton lub obiekt glowny sieci w `CPythonNetworkStream`.

**Zaleznosci wyjsciowe (Outbound):**
*   `CPythonCharacterManager` - Glowne wyjscie graficzne/logiczne klienta. `CNetworkActorManager` tworzy i usuwa tam obiekty (`CreateInstance`, `DeleteInstance`, `DeleteInstanceByFade`).
*   `CInstanceBase` - Instancje logiki gry, w ktorych odpytuje o wlasciwosci i dokonuje update'ow pancerzy, broni i ruchow, a takze wymusza stan jazdy konnej.
*   `IAbstractPlayer` - Aby zglosic watek glownego bohatera, gdy odbierze informacje z odpowiednim VID.
*   `CPythonNonPlayer` oraz `CPythonBackground` (posrednio lub bezposrednio podczas mapowania parametrow w procesach posrednich).
*   `UserInterface::Core::EventBus` - (Przez handler) do wywolywania odswiezania interfejsu (TargetBoard).

**Drzewo dyrektyw `#include`:**
*   `"StdAfx.h"`
*   `"NetworkActorManager.h"`
*   `"PythonCharacterManager.h"`
*   `"PythonItem.h"`
*   `"AbstractPlayer.h"`
*   `"InstanceBase.h"` (w naglowku)

**Ryzyka zaleznosci:** Zaleznosci z CPythonCharacterManager sa krytyczne. Niewlasciwe czyszczenie miedzy tymi instancjami doprowadza do stalych leakow. Cykliczne relacje miedzy streamem -> managerem aktorow -> streamem.

**Model pamieciowy:** 
Wewnatrz uzywana jest `std::map<DWORD, SNetworkActorData> m_kNetActorDict`. Pamiec alokowana dynamicznie per instancja dla samego zarzadzania klasami, ale C++ trzyma same struktury danych sieciowych jako wartosci (nie wskazniki), wiec dealokacja mapy automatycznie czysci dane. Sam menedzer `CNetworkActorManager` uzywa inteligentnych referencji `CReferenceObject`. Instancje gry zarzadzane sa jako nagie wskazniki (`CInstanceBase*`), ktorych cykl zycia w pelni zalezy od CPythonCharacterManager (wiec uzywa patternu Observers i Unmanaged Pointers z gwarantowanym ownershipem u CPythonCharacterManager).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur

| Nazwa | Rola | Wlasciciel/Rozmiar |
|---|---|---|
| `SNetworkActorData` | Kontener DTO (Data Transfer Object) przechowujacy pelen znany stan aktora odbierany z sieci przed zrzutowaniem go do wizualnego CInstanceBase. | Heap / Stack |
| `SNetworkMoveActorData` | Kontener do przekazywania intencji ruchu. | Zmienna tymczasowa |
| `SNetworkUpdateActorData` | Kontener do aktualizacji ekwipunku, gildii i parametrow fizycznych. | Zmienna tymczasowa |
| `CNetworkActorManager` | Klasa zarzadzajaca kolekcja `SNetworkActorData` i tlumaczaca ja na instancje w `CPythonCharacterManager`. | Watek Glowny (Render/Update) |

### Metody Publiczne `CNetworkActorManager`

| Metoda | Argumenty | Zwraca | Skutki/Zalozenia |
|---|---|---|---|
| `AppendActor` | `const SNetworkActorData&` | `void` | Dodaje lub aktualizuje istniejacy wpis w `m_kNetActorDict`. Jezeli gracz wejdzie w zakres widocznosci, tworzy fizyczny instans `__AppendCharacterManagerActor`. |
| `RemoveActor` | `DWORD dwVID` | `void` | Usuwa aktora z logiki wizualnej (`__RemoveCharacterManagerActor`) i slownika (`m_kNetActorDict`). |
| `UpdateActor` | `const SNetworkUpdateActorData&` | `void` | Aktualizuje pancerz, bron i atrybuty. Odnajduje `CInstanceBase` po VID i wysyla komendy ChangeArmor itp. |
| `MoveActor` | `const SNetworkMoveActorData&` | `void` | Wymusza ruch aktora korzystajac z predykcji ruchu po stronie klienta (`PushTCPState`). |
| `SyncActor` | `DWORD dwVID, LONG lPosX, LONG lPosY` | `void` | Natychmiastowa synchronizacja pixeli na mapie po przeskoku (`NEW_SyncPixelPosition`). |
| `SetActorOwner`| `DWORD dwOwnerVID, DWORD dwVictimVID` | `void` | Ustawia przynaleznosc, np. dla petow, wierzchowcow by ustawic pointery i owner logic (`NEW_SetOwner`). |

### Memory Layout 
`SNetworkActorData` jest duza struktura kopiowana w pamieci z wieloma polami.
Offsety bajtowe zaleza od kompilatora x64, jednak najwazniejsze:
- `m_stName` (std::string - 32 bajty w MSVC).
- `m_kAffectFlags` (CAffectFlagContainer - bitset wewnetrzny).
- Reszta (DWORD, LONG, FLOAT) pakowane po sobie, moga zawierac padding. Zmiana kolejnosci w przyszlosci dla cache locality.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
*   `ActorAddPacket` (`HEADER_GC_CHARACTER_ADD`) (obslugiwane w `HandleActorAdd`) - Informuje o pojawieniu sie bytu.
*   `TPacketGCCharacterAdditionalInfo` (`HEADER_GC_CHAR_ADDITIONAL_INFO`) - Informuje o ekwipunku, gildii, trybie PK wchodzacego bytu.
*   `TPacketGCCharacterDelete` (`HEADER_GC_CHARACTER_DEL`) - Znika bytu z radaru/mapy.

**Metody Pythona:**
Brak bezposrednich bindow `CNetworkActorManager` w `PyMethodDef`. Zmiany sa wywolywane z logiki sieciowej C++ jako Dispatchery PhaseGame (`CPythonNetworkStream`), ktore pozniej notyfikuja zdarzeniami do EventBus lub UI (np. EventBus TargetBoardRefreshEvent po zaktualizowaniu).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Wielowatkowosc:**
Klasa ta JEST SINGLE-THREADED i MUSI byc uzywana wylacznie w glownym watku gry (Game Thread / Render Thread). Jakiekolwiek odpytywanie z asynchronicznych watkow (np. watku sieci, jesli bylby prawdziwie wielowatkowy) spowoduje wyscigi przy modyfikacji `m_kNetActorDict` i crash w renderowaniu `CInstanceBase`.

**Crash Points:**
1.  **Dangling Pointers i Utrata Synchronizacji:** `CPythonCharacterManager` zarzadza wlasnymi instancjami, ale `CNetworkActorManager` zaklada ich poprawne istnienie, robiac pointer-lookup za pomoca `CPythonCharacterManager::Instance().GetInstancePtr(dwVID)`.
2.  Gdy pakiet `CharAdditionalInfo` nadejdzie przed pakietem `ActorAdd` dla danego VID, metoda `UpdateActor` w `CNetworkActorManager` przerwie dzialanie po failu `m_kNetActorDict.find(dwVID)` zapobiegajac crashowi, ALE skutkuje to tym, ze postac straci informacje o swoim ekwipunku, co na kliencie moze objawic sie "nagim aktorem".

**Zarzadzanie zasobami (RAII):**
*   Unika wyciekow poprzez konsekwentne uzycie `.erase()` po uprzednim usunieciu instancji z `CPythonCharacterManager`.
*   Zwracajac nagie wskazniki CInstanceBase, robi to bezpiecznie poniewaz uzywa ich jako tymczasowych by wylolac na nich funkcje w tym samym stosie, bez zapisu i zawierzajac ich ownership zewnetrznym managerom.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Jak rozszerzac (Step-by-step extension guide):**
1. Jezeli dodajesz nowy typ wyposazenia (np. efekt aury, peta-kompana):
    a. Rozszerz w `NetworkActorManager.h` struktury `SNetworkUpdateActorData` i `SNetworkActorData` o pola przechowujace to ID.
    b. Odwzoruj przypisanie z pakietow sieciowych (np. w `HandleCharAdditionalInfo`).
    c. Przejdz do definicji `CNetworkActorManager::UpdateActor` i napisz przekazanie pola dalej, np. `pkInstFind->ChangeAura(c_rkNetUpdateActorData.m_dwAura);`.
    d. Pamietaj o aktualizacji kopiowania konstruktora `__copy__` w `SNetworkActorData`.

**Debugowanie:**
- Najlepsze logi sa ustawiane za pomoca narzedzia `EterBase::ModernLogger`. Warto uzywac `EterBase::ModernLogger::Trace` wewnatrz handlerow.
- Kluczowe VID mozna sledzic, jesli wstawisz `if (dwVID == 12345) { // breakpoint }`.
- Brak wizualnego bytu po dodaniu na mapie zazwyczaj oznacza, ze `__IsVisibleActor` zwrocilo `false` - weryfikuj odleglosc widzenia `CHAR_STAGE_VIEW_BOUND`.

**Testowanie Headless:**
- Nalezy stworzyc mock `CPythonCharacterManager` z wykorzystaniem atrap na `GetInstancePtr` i wstrzyknac puste metody przez makra preprocesora dla CInstanceBase (lub dodac FakeInstance).
- Konieczne jest stworzenie fake `IAbstractPlayer` dla glownego gracza poniewaz klasa odwolywuje sie przez `IAbstractPlayer::GetSingleton()`.
- Nastepnie mozna sztucznie preparowac pakiety (w postaci bajtowych Span) i przepuszczac przez system Handlerow (ActorAddHandler), weryfikujac liczbe aktorow w slowniku lub przechwytujac zgloszenia z CPythonCharacterManager.
