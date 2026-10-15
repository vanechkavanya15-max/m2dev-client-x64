---
task_id: "atlas_c07_13_bone_attachment_sync"
cluster: "MOD"
module_name: "Mocowanie Broni i Przedmiotow do Kosci Szkieletu"
target_files:
- src/UserInterface/InstanceControllers/InstanceEquipmentModelImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_13_bone_attachment_sync.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul ten odpowiada za synchronizacje i wyliczanie logicznego przypisania (doczepienia) broni (jednorecznej, dwurecznej, luku, dzwonu - tzw. Main Weapon) oraz broni dodatkowej (Offhand) do odpowiednich kosci szkieletu postaci (np. Bip01 R Hand, Bip01 L Hand).
- **Przeplyw danych i cykl zycia:** Pusty (lub usuniety w toku refaktoryzacji) plik `InstanceEquipmentModelImpl.cpp` zostal zastapiony bardziej specjalistycznymi implementacjami takimi jak `InstanceEquip_MainWeapon.cpp` oraz `InstanceEquip_OffhandWeapon.cpp`.
  Kiedy serwer przesyla pakiet lub zachodzi akcja UI wyposazenia broni, kontroler (np. `InstanceEquip_MainWeapon`) weryfikuje VNUM, aktualizuje swoj wewnetrzny stan i publikuje odpowiednie zdarzenia do szyny zdarzen (`Core::EventBus`), takie jak `MainWeaponEquippedEvent` czy `OffhandWeaponChangedEvent`. Te eventy nastepnie informuja warstwe renderowania o koniecznosci przypiecia/odpiecia fizycznego modelu 3D do kosci, uniezalezniajac logike wyposazenia od bezposredniego manipulowania Granny 3D w klasie kontrolera.
- **Punkt wywolania:** Wywolywane glownie jako skutek zdarzen sieciowych w Game Phase lub podczas incjalizacji instancji klienta. Nie sa bezposrednio wpiete w glowna petle OnUpdate/OnRender, lecz reaguja na konkretne komendy ekwipunku.
- **Cykl zycia:** Kontrolery (dziedziczace po `IInstanceEquipmentModelController`) alokowane sa zazwyczaj przez dedykowane fabryki (`CreateMainWeaponController`) powiazane z bytem encji (powiazane przez `EntityId`). Cykl zycia eventow trwa do momentu skonsumowania przez `EventBus`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Fabryki UI oraz system zarzadzania bytami, ktore konstruuja instance wyposazenia (np. podczas ladowania postaci).
  - Klasy wiazace siec (NetworkBridge/PhaseGame), ktore reaguja na komendy zmiany ekwipunku.
- **Zaleznosci wyjsciowe (Outbound):**
  - `Core::EventBus`: Sluzy do asynchronicznego rozglaszania zdarzen (`MainWeaponEquippedEvent`, `MainWeaponClearedEvent`, `OffhandWeaponChangedEvent`, `EquipmentModelChangedEvent`).
  - `EterBase::ModernLogger`: Zapewnia C++23-kompatybilne, formatowane logowanie za pomoca `std::format`.
  - Warstwa narzedziowa (StrongTypes): Klasy domenowe jak `EterBase::ItemVnum`, `EterBase::EntityId`, `EterBase::EntityError`, `EterBase::PacketResult`.
- **Drzewo dyrektyw `#include`:**
  - `../StdAfx.h` - Precompiled header.
  - `IInstanceEquipmentModelController.h`
  - `EterBase/LogModern.h`, `EterBase/ModernLogger.h`
  - `EterBase/Result.h`
  - `EterBase/StrongTypes.h`
  - `UserInterface/Core/EventBus.h`
- **Model pamieciowy:** W klasach uzywa sie referencji, inteligentnych wskaznikow (np. `std::unique_ptr<IInstanceEquipmentModelController>` dla bezpiecznej, wylacznej wlasnosci), enkapsulacji poprzez mocne typy silnie ustrukturyzowane (typy prymitywne zamkniete w silnych typach jak `ItemVnum`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `UserInterface::InstanceControllers::InstanceEquip_MainWeapon`: Kontroler wyposazenia broni glownej, implementuje `IInstanceEquipmentModelController`. Wlasciciel: watek glowny klienta. Wielkosc zalezy od stanow: m.in. `EntityId`, `ItemVnum` (zwykle ok. 16 bajtow powiazanego obiektu).
  - `UserInterface::InstanceControllers::InstanceEquipOffhandWeapon`: Kontroler wyposazenia w lewej rece, analogicznie zarzadza bronia dodatkowa. Wlasciciel: watek glowny.
  - `UserInterface::InstanceControllers::MainWeaponEquippedEvent`: Zdarzenie powiadamiajace o doczepieniu broni, implementuje `Core::IEvent`. Zawiera `entityId` oraz `vnum`.
  - `UserInterface::InstanceControllers::MainWeaponClearedEvent`: Zdarzenie powiadamiajace o usunieciu broni, zawiera samo `entityId`.
  - `UserInterface::InstanceControllers::OffhandWeaponChangedEvent`: Uniwersalne zdarzenie dla broni w lewej rece, obejmuje aktualizacje z zarowno usunieciem jak i zalozeniem w zaleznosci od vnum.
- **Tabela Metod Publicznych:**
  - `InstanceEquip_MainWeapon::SetPart(ModelPart part, EterBase::ItemVnum vnum) -> EterBase::PacketResult<void>`
    - Pre-condition: `part == ModelPart::Weapon` lub `part == ModelPart::Main`. `vnum` musi byc prawidlowym `ItemVnum`.
    - Side-effects: Wypuszcza do szyny `MainWeaponEquippedEvent`, ustawia log w `ModernLogger`.
  - `InstanceEquip_MainWeapon::ClearPart(ModelPart part) -> EterBase::PacketResult<void>`
    - Pre-condition: Zgodnosc z enumeracja zdefiniowana.
    - Side-effects: Resets internal vnum to 0. Emits `MainWeaponClearedEvent`.
  - `InstanceEquipOffhandWeapon::EquipOffhand(EterBase::ItemVnum vnum) -> EterBase::PacketResult<void>`
    - Pre-condition: `vnum` rozne od 0.
    - Side-effects: Emits `OffhandWeaponChangedEvent`. Logs result.
  - `InstanceEquipOffhandWeapon::UnequipOffhand() -> EterBase::PacketResult<void>`
    - Pre-condition: Bron w lewej rece nie jest 0 (w przeciwnym wypadku NO-OP).
    - Side-effects: Wypuszcza `OffhandWeaponChangedEvent` z Vnum=0.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Struktura `MainWeaponEquippedEvent`:
    - offset 0: vptr (`IEvent`) (w zaleznosci od ukladu pBazy, 8 bajtow)
    - offset 8: `EntityId entityId`
    - offset 12: `ItemVnum vnum`

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul stanowi reakcje na pakiety powiazane z zarzadzaniem ekwipunkiem, najpewniej z pakietami GC `ITEM_SET`, `ITEM_USE`, `GC::CHARACTER_ADDITIONAL_INFO`. Zamiast bezposredniej manipulacji binarnej, pakiet parsuje identyfikatory, ktore wywoluja operacje typu `SetPart`.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnie C++23, nie eksponuje bezposrednio bindow do Pythona, choc skutki akcji (takie jak Eventy) sa rejestrowane/sluchane przez podsystemy zarzadzania instancjami (np. w `CPythonCharacterManager`), z ktorych dostep do interfejsu graficznego i wiaza sie one z wywolywaniem procedur w `wndMgr` w celach synchronizacji UI ze stanem broni bohatera.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Rozwiazania oparte o model zdarzen sa zalezne od synchronicznosci `EventBus`. Modul musi operowac w obrebie jednego watku (zwykle Game/Main), w innym razie wymaga uzycia muteksow tak jak to mozna znalezc w `InstanceEffect_Bone.cpp`. Tutaj nie ma blokad na szynie, co wymusza, by wywolania wejsciowe nastepowaly z wlasciwego dispatchera (NetworkBridge).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Brak encji o odpowiednim identyfikatorze `EntityId` i odwolanie do niej w obiekcie zdarzenia (EterBase::EntityError::NotFound).
  - Proba usuniecia (ClearPart) dla innego modulu (np. Head) co zrzuci ujemny/error result z `EterBase::PacketError::InvalidHeader`.
- **Zarzadzanie zasobami (RAII):** Kod powstrzymuje sie od recznej alokacji poprzez rzutowania uzywajac typow `std::unique_ptr` w fabrykach oraz wyzbywa sie bolesnych bledow poprzez nie trzymanie odniesien do samego szkieletu/Granny w klasie - sa one posredniczone przez komunikaty, co przeciwdziala bledom Use-After-Free podczas niszczenia UI/aktora (odniesienie do CWindowManager).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:**
  1. Jesli wprowadzasz nowa czesc modelu, dodaj wpis w enum `ModelPart` (np. `OffhandWeapon` -> `WeaponLeft`).
  2. Implementuj kontroler dziedziczac po `IInstanceEquipmentModelController` lub bazuj na nowej klasie powiazanej kompozycja (takiej jak `InstanceEquipOffhandWeapon`).
  3. Zaimplementuj wysylanie stosownego Eventu (np. `OffhandWeaponChangedEvent`) poprzez `Core::EventBus`.
  4. Dodaj sluchacza (listener) dla tego zdarzenia w rendererze (Granny wrapperze), aby zsynchronizowac model z odpowiednia koscia w pliku modelu np. dla nowej kosci.
- **Jak debugowac i logowac:**
  Nalezy korzystac z modulu `EterBase::ModernLogger`, rejestrujac `Info`, `Debug` z biblioteka `std::format`. Przy zlych probach alokacji wstrzykiwane jest logowanie typu `Error` i sprawdzane sa komunikaty zwrotne typu `EterBase::PacketResult`.
- **Jak testowac bez interfejsu graficznego:**
  Mozna korzystac z wlasnych makr i narzedzi w `ctest` badz mockowac `EventBus`. Rejestrujac zaslepke do subskrybowania `MainWeaponEquippedEvent`, z wstrzykiwaniem logow za sprawa mock-loggera, mozemy pominac calkowicie koniecznosc posiadania D3D/Granny do weryfikacji powiazan biznesowych.
