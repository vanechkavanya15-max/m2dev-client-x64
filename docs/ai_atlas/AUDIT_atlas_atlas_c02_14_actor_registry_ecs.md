---
task_id: "atlas_c02_14_actor_registry_ecs"
cluster: "ACT"
module_name: "Nowoczesny Rejestr Aktorow i Komponenty ECS"
target_files:
- src/UserInterface/Actors/ActorRegistry.h
- src/UserInterface/ECS/ECSComponents.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_14_actor_registry_ecs.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul implementuje dwa glowne aspekty zarzadzania bytami w nowej architekturze klienta. `Client::World::ECSComponents` definiuje podstawowe struktury danych oparte na paradygmacie Data-Oriented Design (Transform, Visual, Motion, CombatState), oddzielajac stan od logiki i zachecajac do optymalnego ukladu w pamieci. Z kolei `UserInterface::Actors::Subsystems::ActorRegistry` sluzy jako jednowatkowy rejestr referencji (`CInstanceBase*`) dedykowany dla interfejsu uzytkownika. Rozdzielenie to zabezpiecza logike symulacji przed bezposrednimi wplywami UI i wiaze cykl zycia widocznych aktorow w deterministyczny, przewidywalny sposob (brak mutexow).
- **Punkt wywolania w petli gry:** Rejestr modyfikowany jest asynchronicznie poprzez system zdarzen i tworzenie instancji, zas wyliczenia ECS operuja na danych ciaglych podczas fazy OnUpdate() oraz tuz przed renderowaniem w celu propagacji stanow (np. transformacji).
- **Przeplyw danych (Data Flow):** Stan sieciowy/gry zostaje zapisany do komponentow (w `Client::World`). Na podstawie zmian widocznosci, aktor UI moze zostac zarejestrowany poprzez wywolanie `RegisterActor` i powiazany z `VirtualID` (VID). Wszelkie odpytania UI uzywaja metody `FindActor` dla bezpiecznego dostepu do reprezentacji graficznej postaci (`CInstanceBase`).
- **Cykl zycia (Lifecycle):** Alokacja i inicjalizacja instancji (`CInstanceBase`) maja miejsce poza rejestrem, jednak sam rejestr monitoruje ich obecnosc. Podczas zniszczenia bytu lub rozgloszenia zdarzenia smierci aktora, `UnregisterActor` zostaje wywolany, wskaznik jest zerowany i natychmiast odpinany, a opcjonalny `IGameEventSink::OnActorDead` i `DeadCallback` sa informowane, umozliwiajac bezpieczne zwolnienie powiazanych zasobow UI i unikanie "Dangling Pointers".

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywane glownie przez fasade `UIManager` i moduly z klastra `UI` reagujace na pakiety spawnu/usuniacia obiektow od serwera (siec -> symulacja -> UI).
  - Klasy wyszukujace instancje po `VID` celem wyswietlenia tekstu (TextTail) lub zaktualizowania okien dialogowych i paskow HP.
- **Zaleznosci wyjsciowe (Outbound):**
  - Zaleznosc koncepcyjna na `CInstanceBase` (forward declaration) co odcina ten modul od bezposredniej inkluzji ogromnych naglowkow graficznych.
  - Odbiornik zdarzen `UserInterface::Contracts::IGameEventSink`.
  - W klasie `ECSComponents.h` zaleznosc na wewnetrznych podstawach typu `EterBase::StrongType`, `EterBase::EntityId`, i `EterBase::ItemVnum`.
- **Drzewo dyrektyw `#include`:**
  - W rejestrze: `<cstdint>`, `<unordered_map>`, `<functional>`, `"UserInterface/Contracts/IGameEvents.h"`. Posiada czysty uklad z minimalnymi inkluzjami, uzywajac forward-declarations (`class CInstanceBase;`).
  - W ECS: `<cstdint>`, `"../../EterBase/StrongTypes.h"`. Ryzyko cyklicznych zaleznosci jest bliskie zeru.
- **Model pamieciowy:** W rejestrze UI uzyto konwencji `std::unordered_map` bazujacej na czystych wskaznikach (`CInstanceBase*`), ale zabezpieczonych restrykcyjnym cyklem zycia. Dane komponentow ECS opieraja sie wylacznie na strukturach POD/Trivial i silnym typowaniu.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `Client::World::MapCoords`: Reprezentuje pozycje w swiecie 3D. Zawiera float x, y, z (12 bajtow).
- `Client::World::Mat4`: Obejmuje macierz 4x4 uzywana do transformacji renderowania (64 bajty).
- `Client::World::TransformComponent`: Komponent polozenia, obrotu i macierzy globalnej (80 bajtow). Przechowuje pozycje, float rot (obrot), oraz swiatowa macierz `Mat4`.
- `Client::World::VisualComponent`: Komponent wizualny (Vnum rasy, armor, bron, flaga isVisible, float alpha).
- `Client::World::MotionComponent`: Komponent animacji (uint16_t motionMode, index, float speed, loopTime).
- `Client::World::CombatStateComponent`: Stan bojowy (curHP, maxHP, targetVid, bool isDead).
- `UserInterface::Actors::Subsystems::ActorRegistry`: Obiekt bedacy jednowatkowym, wlascicielskim menedzerem referencji (std::unordered_map). Kontroluje mape aktorow, opcjonalny callback oraz pointer na IGameEventSink.

**Tabela Metod Publicznych (`ActorRegistry`):**
- `ActorRegistry()` / `~ActorRegistry()` / Konstruktory przenoszace. Brak kopiowania (zablokowane).
- `bool RegisterActor(DWORD dwVID, CInstanceBase* pInst)`: Rejestruje wskaznik, chroniac przed duplicate VID.
- `bool UnregisterActor(DWORD dwVID)`: Usuwa wiersz z mapy i informuje podczepione callbacki. Zabezpiecza wskazniki przed zniszczeniem (Dangling pointer safety).
- `CInstanceBase* FindActor(DWORD dwVID) const`: Wyszukuje pozycje. W razie braku, zwraca `nullptr`.
- `void ClearAll() noexcept`: Wyrejestrowywuje wszystkich aktorow naraz.
- `bool ContainsActor(DWORD dwVID) const noexcept`: O(1) weryfikacja obecnosci aktora.
- `size_t GetActorCount() const noexcept`: Zwraca size z wewnetrznej `std::unordered_map`.
- `bool IsEmpty() const noexcept`: Sprawdza czy mapa == 0.
- `void SetEventSink(IGameEventSink*)` / `GetEventSink()`: Obsluga integracji zdarzen (IGameEvents).
- `void SetDeadCallback(DeadCallback callback)`: Konfiguruje `std::function` powiadamiajacy o usunieciu wpisu.
- `const ActorMap& GetAllActors() const noexcept`: Bezposredni podglad na dane (Const).
- `template <typename Func> void ForEachActor(Func&& func) const`: Bezpieczny iterator dzialajacy na aktywnych wpisach.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Komponenty ECS zadeklarowano optymalnie z minimalnym paddingiem (szczegolnie `TransformComponent` i `MapCoords`).
- Uklady moga byc bezproblemowo uzywane w integracjach FFI, botach typu Arthion lub przez moduly hookujace odczyty pamieci, z uwagi na silne wlasciwosci Plain-Old-Data dla struktur ECS.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Rejestr i Komponenty stanowia wylacznie warstwe reprezentacyjna i domenowa, i sa zupelnie odlaczone od bezposredniej obslugi kodu sieciowego. Inne moduly, np. `PhaseGameCombatBridge` moga dekodowac pakiety GC (Game->Client) jak `TPacketGCDamageInfo` (0x...) lub `TPacketGCCharacterAdd` i zapisywac te stany w obiektach ECS lub instruowac UIManager aby zarejestrowac w `ActorRegistry`.
- **Metody Pythona (`PyMethodDef`):** Same pliki nie udostepniaja struktur C-API. Jednakze, obiekty uzyskane przez `FindActor` w tym rejestrze moga byc pozniej mapowane na Python handle (`unsigned long long` lub PyObject) by skrypty UI mogly z nich korzystac.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod `ActorRegistry` to wylacznie jednowatkowy zasob. Wywolania powyzszych metod musza nastepowac w glownym watku klienta! Brak operacji blokujacych `std::mutex` eliminuje bottleneck wydajnosci przy jednoczesnym nakazie rygoru kontroli dostepu z zewnatrz.
- **Potencjalne punkty awarii (Crash Points):** Dangling Pointers sa glownym narazeniem, co kod lapie poprzez konwencje resetu w `UnregisterActor`. Proba uzycia usunietego aktora i brak obslugi `nullptr` od `FindActor` na poziomie dzwoniacym moze grozic C-rash'em (`Access Violation`).
- **Zarzadzanie zasobami (RAII):** Sam `ActorRegistry` celowo uzywa golyc wskaznikow (`CInstanceBase*`), nie pelni funkcji alokatora. Modul wlascicielski aktora zarzadza fizyczna pamiecia.
- W komponentach ECS: nalezy unikac uzywania konstruktorow implicit w wektorach. Struktury maja jawne metody `Reset()`, pozwalajace na czyszczenie prealokowanych obiektow pamieci w ramach puli, zamiast ich reallokacji.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jezeli planujesz modyfikacje logiki aktora UI, upewnij sie, ze rozszerzasz klasa `CInstanceBase` oraz `ActorRegistry` bez lamania zasady pojedynczego watku.
  2. Jesli chcesz wdrozyc dodatkowe wlasciwosci w ukladzie ECS, dodaj nowa definicje `struct` wewnatrz `Client::World::ECSComponents.h` i utrzymuj zalozenia Data-Oriented Design (brak wirtualnych funkcji, ciagly uklad zmiennych).
  3. Skonfiguruj wywolania w plikach z klastra `ECSWorldRegistry` aby wdrozyc tabele tego nowego komponentu.
- **Jak debugowac i logowac:** Do sprawdzenia dzialania struktury sprawdz logi odpalajac aplikacje w srodowisku okienkowym (np. na Visual Studio w debug). `ContainsActor` to najwazniejsze miejsce na "Data Breakpoint", aby wylapywac zglaszanie nienalezytych usuniec instancji po id VID.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W tescie jednostkowym `doctest` mozesz smialo utworzyc atrape implementacyjna i zweryfikowac operacje wstawiania i usuniacia `RegisterActor/UnregisterActor` wewnatrz glownej petli programu headless poniewaz modul ten jest absolutnie niezalezny od okien DirectX 9 oraz modulow renderujacych (`CInstanceBase` jest w nim jedynie wskaznikiem niekompletnym, `forward declared`). Uzyj `EterBase::EntityId` dla testow ECS.
