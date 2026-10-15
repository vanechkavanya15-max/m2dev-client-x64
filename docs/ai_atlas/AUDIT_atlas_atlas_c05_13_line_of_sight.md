---
task_id: "atlas_c05_13_line_of_sight"
cluster: "WLD"
module_name: "Line-of-Sight Raycast i Raymarching Terenu"
target_files:
- src/GameLib/LineOfSightRaycast.h
- src/GameLib/RaymarchTerrainQuery.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_13_line_of_sight.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu AI: Line-of-Sight Raycast i Raymarching Terenu

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul ten dostarcza mechanizmy do sprawdzania, czy cel znajduje sie w prostej linii wzroku gracza (Line-of-Sight, LoS) oraz do analizy przeciec promieni z wzniesieniami terenu (Raymarching). Jest to kluczowe do zapobiegania atakom (np. z luku) przez sciany budynkow, obiekty na mapie i wzniesienia terenu.

Modul zostal zaprojektowany z mysla o architekturze "zero-conflict" w nowoczesnym C++ (C++20/C++23) z silnym naciskiem na brak zaleznosci od interfejsu graficznego (GUI). Oddziela logike 2D z uzyciem algorytmu Bresenhama (`LineOfSightRaycast`) od wyliczania kolizji 3D z uzyciem wysokosci terenu mapy (`RaymarchTerrainQuery`).

- **Zastosowanie w petli gry:** Funkcje z tego modulu zwykle wolywane sa w glownym watku gry (czesto powiazanym z watkiem renderowania D3D lub aktualizacji logiki postaci). Sprawdzenia czesto maja miejsce w OnUpdate podczas przymierzania sie do ataku zasiegowego lub sprawdzania widocznosci przy nawigacji.
- **Przeplyw danych (Control & Data Flow):**
  1. Wywolujacy (np. logika ataku) generuje punkt startowy i koncowy.
  2. W przypadku `LineOfSightRaycast`, funkcja uzywa abstrakcyjnego predykatu `isObstacle`, ktory jest implementowany przez kod wywolujacy w celu odpytywania komorek siatki nawigacyjnej.
  3. W przypadku `RaymarchTerrainQuery`, funkcja pyta o wysokosc terenu za posrednictwem `CMapManager` iterujac wzdloz promienia w krokach `CTerrainImpl::HALF_CELLSCALE`.
- **Cykl zycia obiektow:** Moduly te posiadaja metody statyczne i dzialaja w oparciu o stan w pamieci (stateless). Nie zapamietuja one stanu pomiedzy wywolaniami (poza ewentualnym uzyciem EventBus, ktory dziala asynchronicznie). Blad mapy/braku pamieci obslugiwany jest na poziomie typow monadycznych takich jak `std::expected` z biblioteki glownej.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Systemy walki (Combat System), walidatory atakow zasiegowych.
- Algorytmy pathfindingu i nawigacji botow/mobow sprawdzajace widocznosc celu (AggroChecker).

**Zaleznosci wyjsciowe (Outbound):**
- `<d3dx9.h>` - dla podstawowych wektorow matematycznych `D3DXVECTOR3`.
- `EterBase/Result.h`, `EterBase/StrongTypes.h`, `EterBase/LogModern.h` - dla typow silnych, logowania (ModernLogger) i systemow powiadamiania o bledach.
- `UserInterface/Core/EventBus.h` - do asynchronicznego oglaszania eventow `RaymarchHitEvent`.
- `PRTerrainLib/Terrain.h`, `MapManager.h` - do bezposredniego dostepu do topologii i danych wysokosci mapy terenu (CMapManager, CMapOutdoor).

**Drzewo dyrektyw `#include` i Ryzyka:**
- `RaymarchTerrainQuery.h` dolacza m.in. zewnetrzne `EterBase` oraz `UserInterface/Core/EventBus.h`. Istnieje minimalne ryzyko cyklicznych zaleznosci, o ile pliki eventow nie beda uwzgledniac na slepo definicji modulu w swych handlerach.
- Plik nie wykorzystuje klas naruszajacych "zero-conflict" wiec ulatwia to mockowanie.
- `LineOfSightRaycast.h` posiada jedynie include'y standardowe `<cstdint>`, `<cmath>`, `<cstdlib>`, `<concepts>`, przez co jest wysoce uniwersalny (header-only, bez zewnetrznych bibliotek metinowych).

**Model pamieciowy:**
- Obliczenia wykonywane sa na stosie (stack-allocated).
- Wektory, monady (`std::expected`, `std::optional`), lambdy operuja wylacznie bez alokacji sterty (heap).
- Zastosowano bezpieczny dostep przez `std::reference_wrapper<CMapOutdoor>`, eliminujac dziki / wiszacy wskaznik i weryfikujac wczesniej z uzyciem monady `std::optional` czy mapa w ogole zostala zaladowana na swiecie gry.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur

| Nazwa | Rola | Wielkosc (B) | Wlasciciel Watku |
| --- | --- | --- | --- |
| `LineOfSightRaycast` | Statyczna klasa narzedziowa implementujaca logike 2D Raycastingu Bresenhama na siatce komorek (Grid) z podanym predykatem. | N/A (Statyczna) | Glowny / Dowolny (Thread-Safe jesli predykat jest bezpieczny) |
| `GameLib::RaymarchTerrainQuery` | Statyczna klasa implementujaca precyzyjny 3D raymarching odpytujacy bezposrednio o wysokosc terenu (Heightmap). | N/A (Statyczna) | Glowny (wymaga dostepu do CMapManager) |
| `GameLib::RaymarchHitEvent` | C++ struct dla EventBusa, przekazujaca koordynaty zderzenia z terenem. Dziedziczy po `UserInterface::Core::IEvent`. | sizeof(D3DXVECTOR3) + vtable = ~16B-24B | Zalezy od Handlera Busa |

### Tabele Metod Publicznych

**`LineOfSightRaycast`:**
- `template <typename Predicate> static bool CheckLineOfSight(int32_t startX, int32_t startY, int32_t endX, int32_t endY, Predicate isObstacle)`
  - **Argumenty:** Calkowitoliczbowe koordynaty (X,Y) odcineka, `isObstacle` - bool-zwracajaca funkcja/lambda do sprawdzania przeszkod.
  - **Wartosc zwracana:** `true` (czysta droga), `false` (napotkano przeszkode).
  - **Warunki wstepne:** Predykat musi spelniac koncept `std::predicate<Predicate, int32_t, int32_t>`.

**`GameLib::RaymarchTerrainQuery`:**
- `static std::expected<D3DXVECTOR3, EterBase::NavigationError> ComputeLineOfSight(CMapManager& mapMgr, const D3DXVECTOR3& origin, const D3DXVECTOR3& direction, float maxDistance)`
  - **Argumenty:** Referencja do `CMapManager`, `origin` (Poczatkowy D3DXVECTOR3), `direction` (Znormalizowany wektor kierunku D3DXVECTOR3), `maxDistance` (Maksymalny dystans do sprawdzenia w zmiennym przecinku).
  - **Wartosc zwracana:** Monada zawierajaca pozycje przeciecia (D3DXVECTOR3) LUB kod bledu `NavigationError`.
  - **Warunki wstepne:** Wektor kierunku znormalizowany, mapa musi byc na zewnatrz (`IsMapOutdoor`).
  - **Skutki uboczne:** Emisja logow poprzez `ModernLogger`, oraz wyslanie komunikatu `RaymarchHitEvent` przez `EventBus` w przypadku trafienia.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)

- `GameLib::RaymarchHitEvent`:
  - `vtable` - wirtualna tabela wywodzaca sie z `IEvent`. (offset 0)
  - `D3DXVECTOR3 hitPosition` - trzy floaty `x, y, z` reprezentujace globalny koordynat trafienia na swiecie. (offset na 64-bit zalezy od kompilatora i pakowania, przewaznie +8 bajtow od poczatku).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Powyzsze pliki nie maja wlasnych opcodow. Informacja o wektorze wzroku jest ewentualnie uwzgledniana przed zbudowaniem pakietu ataku (np. `CG_USE_SKILL` lub uzycie luku przez pakiet ataku). Jesli raymarch wypluje blad uderzenia w przeszkode (Heightmap hit), klient po prostu nie powinien wysylac pakietu zadajacego obrazenia do serwera.
- **API Pythona (`PyMethodDef`):** Na ten moment, narzedzia te funkcjonuja w warstwie backendowej `GameLib`. Brak dedykowanych bindow. Aby wyeksponowac uzycie w Pythonie (np. uzywane do UI celowania "target-lock"), nalezy dodac bindowanie przez `METH_FASTCALL`, z weryfikacja zwrotu jako Krotki zawierajacej polozenie x,y,z trafienia.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:**
  - `LineOfSightRaycast`: thread-safe pod warunkiem, ze uzyty `isObstacle` predicate nie mutuje ani nie odczytuje danych bez zabezpieczen z zewnetrznej struktury, ktora ulega modyfikacji (np. Dynamiczna Lista Bytow).
  - `RaymarchTerrainQuery`: uzywa `CMapManager`, ktory generalnie JEST zwiazany z watkiem glownym gry, dlatego raymarching powinno sie wywolywac wylacznie w glownym watku, by uniknac hazardow (race conditions) podczas procesowania komorek terenu.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Bledny kierunek:** Podanie wektora kierunku w `ComputeLineOfSight`, ktory nie jest poprawnie znormalizowany (np. wektor (0,0,0)), poskutkuje nieskonczona petla (lub przejsciem do konca dystansu bez postepu w srodowisku zmiennoprzecinkowym).
  - **Max Distance = 0.0f lub mniejszy:** Funckja ma wbudowane zabezpieczenie, zwraca od razu `EterBase::NavigationError::DestinationUnreachable`.
- **Zarzadzanie zasobami (RAII):**
  - Pliki nie zarzadzaja alokacjami w sposob jawny (brak new/delete) poniewaz wykorzystuja typy monadyczne i obiekty na stosie, co jest idealne i eliminuje wycieki.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli nowa funkcjonalnosc polega na rozszerzaniu Raycastingu (np. grubszy wektor promienia kolizji w stylu "Cylinder-Ray"), utworz nowa strukture w GameLib z osobnym algorytmem, nie zmieniajac dzialania standardowego Bresenhama, ktory jest zoptymalizowany dla 1px komorki.
  2. Sprawdz czy nowa funkcjonalnosc uzywa nowej zaleznosci, jesli tak - sprawdz czy naglowki pozostaja ciche o wewnetrznych konfliktach ("zero-conflict").
  3. Wywolaj uzycie `EventBus::GetInstance().Publish` do przekazania asynchronicznego efektu Twojej metody zamiast wprowadzania polaczen callback.
- **Jak debugowac i logowac:**
  - W klasie `RaymarchTerrainQuery` znajduja sie liczne wezwania do `EterBase::ModernLogger::Debug()`. Najlepsza metoda sledzenia jest aktywacja tych logow na poziom `DEBUG` przed uruchomieniem ataku na przeszkode. Pozwoli to odczytac proces "Raymarch hit terrain at...".
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - `LineOfSightRaycast` to latwy kandydat na Unit Test. Mozesz w testach utworzyc mock siatki w postaci 2D `std::vector<bool>` a nastepnie dac jej predykat sprawdzajacy dany indeks.
  - Testowanie `RaymarchTerrainQuery` bywa ciezsze na Linuksie pod wzgledem map, chyba ze zmockujesz `CMapManager` i jego metode powrotu interfejsu, ktory zwraca stale wartosci wysokosci bez parsowania prawidlowych assetow `.epk`.
