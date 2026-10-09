---
task_id: "atlas_c04_12_aggro_radius_calc"
cluster: "CBT"
module_name: "Zasiegi Agresji Potworow i Kalkulator Wykrywania Celu"
target_files:
- src/GameLib/AggroChecker.h
- src/GameLib/AggroRadiusCalculator.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_12_aggro_radius_calc.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu**: Wyliczanie strefy aggro wokol moba, sprawdzanie czy gracz wszedl w zasieg widzenia potwora agresywnego. Modul decyduje, czy potwor jest w ogole zdolny do agresji (`AggroChecker`), a jesli tak, okresla maksymalny zasieg wykrywania celu zalezny od poziomu i typu (`AggroRadiusCalculator`).
- **Moment w petli gry**: Kod ten wywolywany jest podczas aktualizacji stanow obiektow w petli gry (OnUpdate), zanim nastapi wlasciwy ruch czy atak, na bazie odebranych informacji o bytach i w obsludze sieciowej. 
- **Przeplyw danych (Data Flow)**: W wejsciu modul otrzymuje dane o potworze (np. id, type, aiFlags) z serwera (lub lokalnej bazy) przez zdarzenia bazy sieci. Nastepnie ocenia je z wykorzystaniem statycznych klas matematyki walki (`CombatMath`). Obliczony wynik zasiegu propagowany jest bezposrednio przez szyne zdarzen (EventBus) i nie jest bezposrednio sprzezony z renderowaniem. Z logika domenowa wspolgra system logowania `EterBase::ModernLogger`.
- **Cykl zycia obiektow (Lifecycle)**: Obiekty w tych plikach to wylacznie klasy statyczne sluzace za funkcje pomocnicze (utility classes). Wszystkie struktury danych, takie jak `AggroRadiusRequest` i `MonsterData` sa proste i alokowane na stosie (stack-allocated), zatem zarzadzanie wylacznie przez system typow C++ i monadyczne resulty `std::optional`, `EterBase::Result`. Eventy publikowane sa na `EventBus`.

## Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: 
  - Wywolywane glownie przez menedzerow bytow (Entity Manager / Actor Manager) w trakcie OnUpdate.
  - Generatory bytow dostarczajace podstawowe parametry (MonsterData, id, klasa, poziom).
- **Zaleznosci wyjsciowe (Outbound)**: 
  - `EterBase::StrongType` i system monadow (Result.h, `std::expected` / `std::optional`).
  - `EterBase::ModernLogger` do nowoczesnego logowania i bindowania zmiennych w `std::format`.
  - `UserInterface::Core::EventBus` do propagacji zdarzen (publish).
- **Drzewo dyrektyw `#include`**: 
  - `AggroChecker.h`: `<cstdint>`, `<optional>`, `<span>`, `<vector>`.
  - `AggroRadiusCalculator.h`: `<cstdint>`, `<optional>`, `<format>`, `../EterBase/StrongTypes.h`, `../EterBase/Result.h`, `../EterBase/LogModern.h`, `../UserInterface/Core/EventBus.h`. Brak ryzyka cyklicznych zaleznosci.
- **Model pamieciowy**: Kod nie uzywa wlasnych dynamicznych alokacji (brak new/delete, brak ptr), uzywa czysto `std::optional` dla unikania bindowania i przekazywania po wartosci (oraz statycznego `std::vector` z `reserve`), `span` jako niereferencyjny bufor (zero-copy), a na poziomie obliczen czysty model by-value ze srodowiska modern C++ (stack-allocated structs i silne typy).

## Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
### Tabele Klas, Struktur i Enumow:
| Typ / Nazwa | Rola | Wielkosc w bajtach | Wlasciciel watku |
|---|---|---|---|
| Enum: `CombatMath::ActorType` | Kategoryzacja bytow (Enemy, NPC itp.) (uint8_t) | 1 bajt | Agnostyczny |
| Enum: `CombatMath::AIFlag` | Flagi zachowania, np. Aggressive (uint32_t) | 4 bajty | Agnostyczny |
| Enum: `CombatMath::RaceType` | Typ potwora do modyfikatora promienia (Normal, Boss) | 1 bajt | Agnostyczny |
| Struct: `CombatMath::MonsterData` | Kontener danych do ewaluacji agresywnosci | ok. 12 bajtow | Stos / Agnostyczny |
| Struct: `CombatMath::AggroCheckResult` | Wynik pojedynczego testu | ok. 8 bajtow | Stos / Agnostyczny |
| Struct: `CombatMath::AggroRadiusCalculatedEvent` | Zdarzenie emitowane z wynikiem obliczonego promienia | ~ | EventBus / Glowny watek |
| Struct: `CombatMath::AggroRadiusRequest` | Parametry wejsciowe dla kalkulacji (EntityId, Level, Race) | ~ | Stos / Agnostyczny |
| Class: `CombatMath::AggroChecker` | Statyczna logika weryfikacji flag AI | 0 | Agnostyczny |
| Class: `CombatMath::AggroRadiusCalculator` | Statyczna logika kalkulacji i wysylki na EventBus | 0 | Agnostyczny |

### Metody Publiczne:
| Klasa | Sygnatura | Wartosc Zwracana | Warunki Wstepne i Skutki Uboczne |
|---|---|---|---|
| `AggroChecker` | `static std::optional<AggroCheckResult> CheckIsAggressive(const MonsterData& data)` | `std::optional<AggroCheckResult>` | Typ aktora musi byc Enemy; brak efektow ubocznych. |
| `AggroChecker` | `static std::vector<AggroCheckResult> CheckBatch(std::span<const MonsterData> monsters)` | `std::vector<AggroCheckResult>` | Zawsze zwraca wektor pomyslnych sprawdzen. |
| `AggroRadiusCalculator` | `static EterBase::Result<float, EterBase::EntityError> CalculateAndPublish(const AggroRadiusRequest& request)` | Monada `EterBase::Result<float>` | EntityId musi byc poprawne; Wypycha wynik na `EventBus` (skutek uboczny) i loguje obliczenia. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets):
- `MonsterData`: 
  - `id` (uint32_t): offset 0x0 
  - `type` (ActorType): offset 0x4 (plus padding do 0x8)
  - `aiFlags` (uint32_t): offset 0x8 (calosc ok 12 bajtow)
*(Offsety zalozone dla wyrownania 4-bajtowego).*

## Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**: Brak scislych opcodow wewnatrz tych klas, jednak obiekty te biora udzial w interpretacji flag pochodzacych zwykle z pakietow GC (Game->Client) dotyczacych instancjonowania NPC/Mobow (np. SNetworkActorData w pakiecie z informacja o otoczeniu / CreateCharacter / Spawn).
- **Metody Pythona (`PyMethodDef`)**: Te dwie klasy dzialaja na poziomie C++ w logice klienta bazujacej na zdarzeniach. Nie ujawniaja wlasnych wrapperow CPython bezposrednio. Wyniki (np. poprzez zaslyszane wydarzenie na EventBus) moga jednak aktualizowac CInstanceBase, z ktorym to juz interakcje po fastcall wywoluja moduly Pythona.

## Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**: Klasy kalkulatorow sa w pelni stateless. EventBus prawdopodobnie operuje na glownym watku, wiec wywolanie `CalculateAndPublish` (oraz obsluga delegatow) powinno odbywac sie w glownym watku, by uniknac hazardu podczas dystrybucji zdarzen.
- **Potencjalne punkty awarii (Crash Points)**: Brak obslugi nullptr poniewaz opiera sie na referencjach. Bledne/puste zapytanie w `CalculateAndPublish` zostaje plynnie zneutralizowane poprzez log ostrzegawczy i `EterBase::MakeError`, jednakze niezainicjowany/przepelniony `EntityId` moglby w zlych implementacjach powodowac wyciek danych.
- **Zarzadzanie zasobami (RAII)**: W pelni hermetyczne alokacje na stosie. Zwracane sa proste typy liczbowe lub standardowe monady (np. `std::vector`, `std::optional`, `Result`). Nie tworzy zagrozen leakow pamieci VRAM/RAM. Brak GC, brak zmartwien.

## Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Skodyfikuj nowe flagi AI / Ras w odpowiednich enumach w naglowkach.
  2. Dostosuj logike w `AggroRadiusCalculator::CalculateInternal` modyfikujac mnoznik lub dodajac bazowa wartosc aggro.
  3. Utrzymaj zero-dependency podejscie i monadyczna propagacje `std::expected` dla unikania null.
  4. Nie uzywaj `std::format` przed przeslaniem logow do `EterBase::ModernLogger` - uzywaj bezposrednio `{}`.
- **Jak debugowac i logowac**: `EterBase::ModernLogger::Debug()` lub `Trace()`, a w trakcie dzialania szukac np. "Calculated aggro radius" dla monitorowania poprawnosci rzutowania ras (czy defaultuje).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: Klasy `AggroChecker` i `AggroRadiusCalculator` z uwagi na silne separacje i statycznosc doskonale nadaja sie na GTest. Nalezy przekazac proste mocki do zapytan (Request) oraz w przypadku modulu publikujacego sprawdzic co wylatuje do zasymulowanej instancji `EventBus`.
