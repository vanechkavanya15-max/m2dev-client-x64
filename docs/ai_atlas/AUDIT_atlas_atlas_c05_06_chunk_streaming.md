---
task_id: "atlas_c05_06_chunk_streaming"
cluster: "WLD"
module_name: "ChunkStreamingCoordinator - Koordynator Bezszwowego Przesylania"
target_files:
- src/GameLib/Terrain/ChunkStreamingCoordinator.cpp
- src/GameLib/MapChunkStreamingCache.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_06_chunk_streaming.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Rola w architekturze:** Modul odpowiada za bezszwowe ladowanie (streaming) i wyrzucanie z pamieci (eviction) sektorow (chunkow) terenu podczas poruszania sie gracza, co ma na celu eliminacje mikro-przyciec (stutters) kamery przy szybkim przemierzaniu mapy. `MapChunkStreamingCache` pelni role nowoczesnej, niezaleznej od watkow pamieci podrecznej (korzystajac z monad C++23 `std::expected` oraz `std::optional`), natomiast `ChunkStreamingCoordinator` realizuje logike asynchronicznego prefetched ladowania, sledzenia pozycji oraz okreslania priorytetow ladowania.
- **Moment wywolania w petli gry:** Kod koordynatora (`UpdateStreaming`) jest wywolywany na biezaco (np. w OnUpdate glownej petli gry lub watku strumieniujacego), aktualizujac stan kafelkow wokol gracza na podstawie jego pozycji i szybkosci. 
- **Przeplyw danych (Data Flow):** W pierwszej kolejnosci pozycja i predkosc gracza (`playerPos`, `playerVel`) uzywane sa do zidentyfikowania sektorow w zasiegu widzenia (`EnsureSlot`). Nastepnie sektory potrzebne sa kolejkowane w `ChunkStreamingCoordinator` z odpowiednimi priorytetami. Z kolei zdarzenia strumieniowania sa emitowane do `UserInterface::Core::EventBus` (`ChunkEvictedEvent`, `ChunkLoadedEvent`, `ChunkUnloadedEvent`), co decoupluje logike ui i renderowania od samego modulu terenu.
- **Cykl zycia (Lifecycle):**
  - Alokacja/Inicjalizacja: Uruchamiane jest za pomoca `CreateMapChunkStreamingService()`. `MapChunkStreamingCache` rezerwuje elementy z pomoca `std::make_shared<ChunkData>()`.
  - Rejestracja/Prefetch: Kafelki sa rejestrowane (`RequestChunk`, `EnsureSlot`) w mapach i ich stan to `Queued`.
  - Aktywacja: `MarkChunkActive` oznacza zaladowany sektor jako `Active`.
  - Ewikcja: Oddalone o `evictRadius` sektory lub usuniete recznie sa usuwane, zwalniajac zasoby i publikujac odpowiednie eventy na `EventBus`. Przy wyjsciu `ClearAllChunks()` lub dekonstrukcji odpowiednie kontenery czyszcza swoje rezerwy.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Kod wywolywany z zewnatrz przez watek glowny lub logike poruszania sie (wolanie `UpdateStreaming()`). Wywolywany tez moze byc manualnie przez polecenia deweloperskie (np. `RequestChunk`). Inne podsystemy (UI, Render) sluchaja komunikatow przeslanych przez `EventBus` (`ChunkLoadedEvent`, `ChunkUnloadedEvent`, `ChunkEvictedEvent`).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `TerrainHeightStorage` (wykonywanie `RemoveChunk`, `Clear`).
  - `UserInterface::Core::EventBus` (Publikowanie eventow na szynie komunikatow).
  - `EterBase::ModernLogger` (Logowanie nowoczesne: Debug, Info, Warn).
  - Standardowe biblioteki C++23: `std::expected`, `std::optional`, `std::format`, `std::mutex`, `std::shared_ptr`.
- **Drzewo dyrektyw `#include`:** 
  - `MapChunkStreamingCache.h`: `<unordered_map>`, `<optional>`, `<expected>`, `<cstdint>`, `<format>`, `<memory>`, `<string_view>`, `../EterBase/StrongTypes.h`, `../EterBase/Result.h`, `../EterBase/LogModern.h`, `../UserInterface/Core/EventBus.h`.
  - `ChunkStreamingCoordinator.h`: `"IMapChunkStreamingService.h"`, `"TerrainCoordinates.h"`, `"TerrainHeightStorage.h"`, `<unordered_map>`, `<vector>`, `<mutex>`, `<memory>`.
  - `ChunkStreamingCoordinator.cpp`: `"../StdAfx.h"`, `"ChunkStreamingCoordinator.h"`, `"TerrainEvents.h"`, `"../../EterBase/LogModern.h"`, `<algorithm>`, `<cmath>`.
- **Model pamieciowy:** Hybrydowy. W `MapChunkStreamingCache` uzywa sie `std::shared_ptr<ChunkData>` zarzadzanego pod `std::unordered_map`. Z kolei w `ChunkStreamingCoordinator` korzysta sie z klasycznych struktur na stosie `ChunkSlot` w `std::unordered_map`, `TerrainHeightStorage` to surowy wskaznik (niewlascicielski / non-owning raw pointer). Wszedzie obecna miedzywatkowa blokada chroniaca stany koordynatora (`std::mutex`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - `GameLib::ChunkCacheError` - Enum klasy wyliczajacy bledy pamieci podrecznej.
  - `GameLib::ChunkLoadedEvent` / `ChunkUnloadedEvent` - Eventy dziedziczace po `UserInterface::Core::IEvent`. Okreslaja `mapIndex`, `chunkX`, `chunkY`.
  - `GameLib::ChunkCoordinates` - Struktura `mapIndex`, `x`, `y`. Wspiera `<=>` oraz `std::hash`.
  - `GameLib::ChunkData` - Pojedynczy flaga statusu (`bool isReady`).
  - `GameLib::MapChunkStreamingCache` - Menadzer cache z operacjami (Load/Unload/Get). Wykorzystuje monady C++23, dziala bez mutexow, wiec obowiazuje uzycie z tego samego watku lub dolozenie zewnetrznej synchronizacji.
  - `GameLib::Terrain::ChunkState` - Enum stanu chunkow (`Unloaded`, `Queued`, `Loading`, `Active`, `Evicting`).
  - `GameLib::Terrain::ChunkSlot` - Struktura przechowujaca `SectorCoord`, `ChunkState`, `loadPriority`. Rozmiar: ok. 16 B.
  - `GameLib::Terrain::ChunkStreamingCoordinator` - Glowny koordynator implementujacy `IMapChunkStreamingService`. Posiada mutex (`m_mutex`), zatem jest thread-safe.

- **Tabela Metod Publicznych:**
  - `MapChunkStreamingCache::LoadChunk(const ChunkCoordinates& coords) -> ChunkResult`: Wynikiem `std::expected<void, ChunkCacheError>`. Publikuje `ChunkLoadedEvent`. Brak obostrzen thread-safe, konieczna uwaznosc.
  - `MapChunkStreamingCache::GetChunk(const ChunkCoordinates& coords) const -> std::optional<std::shared_ptr<ChunkData>>`: Metoda monadyczna (and_then).
  - `ChunkStreamingCoordinator::UpdateStreaming(float playerX, float playerY) -> void`: Wylicza wektor predkosci, ustala zasieg strumieniowania i kolejkowe sektory (EnsureSlot) oraz zwalnia odlegle sektory. Zawiera muteks w strefach chronionych krytycznych sekcji.
  - `ChunkStreamingCoordinator::RequestChunk(ChunkCoordinate coord) -> void`: Recznie ustala dany chunk jako oczekujacy (Queued) z priorytetem maksimum 1.0f. W srodku uzywa `std::lock_guard`.
  - `ChunkStreamingCoordinator::GetPendingLoadQueue() -> std::vector<SectorCoord>`: Zwraca przefiltrowana i posortowana tablice chunkow do wczytania, zabezpieczone mutexem.

- **Pamieciowy Layout Struktur:**
  - `GameLib::ChunkCoordinates`: `EterBase::MapIndex` (typowo uint32/uint16), `int32_t x`, `int32_t y`. Dobrze spasowana wielkosc do pamieci (~12 B).
  - `GameLib::Terrain::ChunkSlot`: `SectorCoord` (prawdopodobnie dwie wartosci np int x, int y, 8B), `ChunkState` (uint8_t, 1B), `float loadPriority` (4B). Calosc ok. 16 bajtow (wliczajac paddingi).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Brak bezposredniego zwiazku w warstwie logiki (czyste C++ zarzadzanie kafelkami lokalnymi). Ewentualne synchronizacje z pakietami GC zaleza od nadrzednych systemow tworzacych obiekty serwerowe.
- **Metody Pythona:** Nie naraza sie zadnych scislych powiazan `PyMethodDef`. Odseparowanie do warstwy natywnej zmniejsza narzut Pythona (jest tu EventBus dla wspolpracy z reszta silnika i UI).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** `ChunkStreamingCoordinator` posiada jawne blokady `std::mutex` na operacjach mapowania chunkow, ale z kolei `MapChunkStreamingCache` dziala BEZ wbudowanych mutexow. Zatem metody `MapChunkStreamingCache` powinny byc wolane z jednego watku, podczas gdy operacje prefetczowe z koordynatora moga przychodzic z watku pobocznego.
- **Potencjalne punkty awarii (Crash Points):** Niewlascicielski surowy wskaznik na `TerrainHeightStorage* m_heightStorage` w `ChunkStreamingCoordinator`. Instancja `TerrainHeightStorage` MUSI przezyc koordynator. Ewentualnie nullowa wartosc (`nullptr`) jest czesto obslugiwana np. `if (m_heightStorage)`. Nalezy uwazac na lapanie dead-lockow przy mieszaniu callbackow i wywolan na blokadach (np. unikanie logiki UI w srodku `UpdateStreaming`).
- **Zarzadzanie zasobami (RAII):** Kod powszechnie stosuje `std::shared_ptr` w cache. Dla wektorow prefetchingu w `GetPendingLoadQueue()` kopie list z chronionej struktury niweluja wycieki lub ucieczki iteratorow. Ewikcja wywola destrukcje `shared_ptr`, zwalniajac prawidlowo VRAM/RAM (o ile pamiec pod spodem w `ChunkData` bylaby wypelniona i dekonstruowana).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step):**
  1. Zidentyfikuj, czy funkcja tyczy sie priorytetow koordynatora czy logiki bezposredniej.
  2. Jesli to priorytety, edytuj `EnsureSlot` w `ChunkStreamingCoordinator.cpp` i przelicz wzor `slot.loadPriority`.
  3. Zauwaz obecnosc EventBusa. Dodanie logiki graficznej nastepuje poprzez utworzenie nasluchu `UserInterface::Core::EventBus::GetInstance().Subscribe(...)` gdzie indziej bez ingerencji w powyzsze pliki.
- **Jak debugowac i logowac:** Logowanie dziala od razu poprzez `EterBase::ModernLogger` (widoczne wpisy `Debug`, `Info`, `Warn`). Uzywaj tego narzedzia w razie potrzeby weryfikacji. 
- **Jak testowac bez interfejsu graficznego (Headless):** Oba moduly sa kompletnie odizolowane od bibliotek DX9 (Zero powiazan graficznych). W testach z uzyciem mocka `doctest` wystarczy zainicjalizowac recznie `ChunkStreamingCoordinator`, ustawic pozycje postaci poprzez `UpdateStreaming` i weryfikowac liste zwrocona z `GetPendingLoadQueue()` zeby sprawdzic czy prawidlowe wektory kafelkow zostaly zwrocone. Analogicznie przetestowac stany `IsChunkReadySafe` dla cache, symulujac ladowanie. Nalezy sprawdzic czy eventy trafiaja do EventBus.
