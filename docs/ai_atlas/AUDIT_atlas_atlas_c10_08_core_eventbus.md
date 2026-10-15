---
task_id: "atlas_c10_08_core_eventbus"
cluster: "SYS"
module_name: "Szyna Zdarzen Miedzymodulowych (EventBus Architecture)"
target_files:
- src/EterBase/EventBus.h
- src/EterBase/Core/EventBus.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_08_core_eventbus.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `EventBus` w architekturze EterBase to centralna, uniwersalna, bezpieczna watkowo szyna zdarzen (Publish-Subscribe) sluzaca do komunikacji miedzy komponentami C++23 w systemach klienta Metin2. Rozwiazuje ona problem cyklicznych zaleznosci pomiedzy takimi modulami jak Siec, GUI i Core.
- **Punkt wywolywania:** Zdarzenia wywolywane sa synchronicznie na wywolujacym watku np. `OnUpdate` (culling/animacje), watek sieciowy (odbieranie pakietow) oraz operacjach inwentarza uzytkownika.
- **Przeplyw Danych (Data Flow):** 
  1. Zdarzenia (dziedziczace po `IEvent`) sa powolywane, np. `NetworkPacketReceivedEvent`.
  2. Subskrybenci (`Subscribe`) podaja lambdy/std::function jako wywolanie zwrotne, generujac identyfikator uint32_t na wektorze `subscribers_`.
  3. Modul wola `Publish()`, co powoduje bezpieczne skopiowanie wektora subskrybentow w obrebie blokady `std::mutex` i nastepnie synchroniczne `Execute` na odblokowanym muteksie dla calego wektora handlowanych lambd (type erasure `IEventHandler`).
- **Cykl zycia (Lifecycle):** `EventBus` jest wczesnym Singletonem opartym na zmiennej statycznej. Deskryptory alokowane przy subskrybcji to `std::shared_ptr<IEventHandler>`, utrzymujace referencje dopoki nie zostanie wywolany `Unsubscribe`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Subskrypcje i publikacje z pakietow UI (`UserInterface::Core::EventBus` np. UI inwentarza, statystyk gracza).
  - Pakiety od glownego flow: sieciowe (od `CNetworkStream`), rendering (`SIMDCullingCompletedEvent` i `AnimHitFrameEvent`).
- **Zaleznosci wyjsciowe (Outbound):** Posiada absolutny brak zaleznosci na systemy klienta. Oparty calkowicie o `<mutex>`, `<unordered_map>`, `<span>` oraz typy mocne (StrongTypes.h np. `EterBase::EntityId`).
- **Drzewo dyrektyw `#include`:** Wylacznie wbudowane `<typeinfo>`, `<memory>`, `<span>` etc. Brak ryzyka cyklicznosci; wszystkie aliasy leza w przestrzeniach nazw `UserInterface::Core` i `Core`.
- **Model pamieciowy:** Wymienia wskazniki typu raw w `const void* eventPtr`, by natychmiast uzywac statycznego prze-castowania `reinterpret_cast<const EventType*>` w generycznym `EventHandler<T>`. Pamiec na subskrybentow rezyduje na stercie poprzez shared_ptr.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wlasciciel |
|---|---|---|
| `EterBase::EventBus` | Singleton, glowna szyna zdarzen (Publish-Subscribe) w C++23. | Singleton Globalny |
| `EterBase::IEvent` | Interfejs tagujacy obiekty zdarzen. | Brak / Lokalnie wywolane |
| `EterBase::IEventHandler` | Ztype-erasowany bazowy dla Handlerow zdarzen. | Singleton `EventBus` |
| `EterBase::EventHandler<T>` | Typowany obiekt przetrzymujacy callback (std::function). | Smart pointer (Heap) |
| `TargetBoardRefreshEvent` | Refresz okna targetu po operacji GUI. | UI Thread |
| `NetworkPacketReceivedEvent` | Surowe dane z headera i <span> payloadu. | Network Worker |
| `ActorDeadEvent` | Zabicie entitas po VID. | Logika Domeny Walki |

**Tabela Metod Publicznych:**
| Sygnatura | Wartosc Zwracana | Pre-Conditions / Side-Effects |
|---|---|---|
| `static EventBus& GetInstance()` | Referencja na Singleton | Inicjuje strukture bez dead-locku. |
| `template <typename EventType> uint32_t Subscribe(std::function<void(const EventType&)> callback)` | `uint32_t` | Alokuje id, przypina wektor, blokuje muteks `lock_guard`. |
| `template <typename EventType> void Unsubscribe(uint32_t subscriptionId)` | `void` | Wylacza i kasuje shared_ptr (czysci po id uzywajac typowania typeid). |
| `template <typename EventType> void Publish(const EventType& event)` | `void` | Rozglasza kopie wektora uchwytow synchronicznie na watku rozglaszajacym. Brak side efects w muteksie. |

**Pamieciowy Layout Struktur:**
- `EventBus`: Chroni wylacznie `std::mutex mutex_`, id subskrypcji atomiczne `std::atomic<uint32_t> nextSubscriptionId_` i wektor powiadomien w strukturze `std::unordered_map<std::type_index, std::vector<std::pair<uint32_t, std::shared_ptr<IEventHandler>>>> subscribers_`. 

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powolany `NetworkPacketReceivedEvent` umozliwia przekazywanie odizolowanego `std::span<const uint8_t> payload` wywolywanego bezposrednio z `CNetworkStream`, pozwalajac na delegowanie rozkodowania pakietu (m.in opcode w `uint8_t header`) do zainteresowanych podsystemow obslugi (np. okna postaci) poza cyklem.
- **Metody Pythona (`PyMethodDef`):** Modul ten to wylacznie system szkieletowy C++23. Nie mapuje sie bezposrednio do warstwy CPython API.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** System `Publish` tworzy kopie wektora uchwytow w ramach zablokowanego muteksu i wywoluje powiadomienia na odblokowanym muteksie. Umozliwia to nasluchujacym wywolanie re-entrancy np. odpiecie sie (`Unsubscribe`) bez dead-locku glownego singletona. Same modyfikacje wywolane w lambdach musza byc wlasnorecznie chronic wlasny zasob w srodku subskrypcji.
- **Potencjalne punkty awarii (Crash Points):** Nullowe interfejsy z racji niedokonanego wylaczenia subskrypcji w dekonstruktorze (Brak `Unsubscribe`). Wywola to powolanie pod adresem wykasowanego obiektu przy nastepnym Event Publishu.
- **Zarzadzanie zasobami (RAII):** Nie wykonanie jawnego `Unsubscribe` pociagnie za soba rowniez memory leak samego wskaznika lambdy zawieszonego w pamieci po zniszczeniu parent okna.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W `src/EterBase/EventBus.h` dopisz publiczny struct dziedziczacy po `IEvent` z wlasnymi polami np. `struct MyCustomEvent : public IEvent { ... }`. Nastepnie upewnij sie, ze rozpisales w polach docelowych mapowania dla namespace (`UserInterface::Core` i `Core`). Nastepnie mozesz publikowac/subskrybowac nowym obiektem.
- **Jak debugowac i logowac:** Podczas rozglaszania wewnatrz `EventBus::Publish` najlepiej dorzucic hook pod `EterBase::ModernLogger::Debug()` rejestrujacy `typeid(EventType).name()` zeby wysledzic kto rozglasza nieporzadane triggery domenowe.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wzorzec Publisher-Subscriber nadaje sie do mockowania zdarzen dla mechanizmow Headless (wystarczy rzucac mock zdarzenia na glownym singletonie bez ingerencji testowych bezposrednio w funkcjonalnosci docelowe) np. wyslanie `NetworkPacketReceivedEvent` by przetestowac parsing inwentarza. 
