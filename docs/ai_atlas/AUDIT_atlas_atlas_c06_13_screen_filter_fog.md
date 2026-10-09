---
task_id: "atlas_c06_13_screen_filter_fog"
cluster: "RND"
module_name: "CScreenFilter i Parametry Mgly Gestosciowej"
target_files:
- src/EterLib/ScreenFilter.cpp
- src/EterLib/ScreenFilter.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_13_screen_filter_fog.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# RAPORT AUDYTU AI: CScreenFilter i Parametry Mgly Gestosciowej

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CScreenFilter` w architekturze silnika sluzyl pierwotnie do renderowania kolorowej, polprzezroczystej nakladki na caly ekran. Jego funkcja biznesowa mialo byc wprowadzanie globalnego filtrowania kolorow ekranu (zalezne od parametrow srodowiska, np. `.msenv`), ktore mialy dopelniac efekty mgly gestosciowej lub sluzyc przyciemnianiu ekranu.
Obecnie w kodzie, klasa znajduje sie w warstwie EterLib (narzedziowka silnika) i dziedziczy po `CScreen`. 
Cykl wywolan w petli gry:
1. Obiekt `m_ScreenFilter` jest inicjalizowany jako bezposredni element klasy `CMapOutdoor` z modulu GameLib.
2. Podczas ladowania srodowiska (`CMapOutdoor::LoadEnvironment`), wywolywane sa metody `SetEnable`, `SetBlendType` oraz `SetColor` zasilane danymi z konfiguracji srodowiska mapy.
3. W fazie renderowania gry, na koniec kolejki w klasie `CMapOutdoorRender`, w watku glownym renderujacym wywolywana jest metoda `m_ScreenFilter.Render()`.
Cykl zycia obiektu zalezy bezposrednio od modulu przestrzeni zewnetrznej `CMapOutdoor`, dzieki czemu nie wystepuje dynamiczna alokacja ani rezydualne wycieki pamieci przy zmianie map (RAII za pomoca instancjonowania obiektu na stosie/wewnatrz glownej instancji).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** 
  - `src/GameLib/MapOutdoor.cpp` - wywoluje `SetEnable`, `SetBlendType` i `SetColor` podczas konfigurowania lub ladowania parametrow otoczenia.
  - `src/GameLib/MapOutdoorRender.cpp` - cyklicznie wywoluje `Render()` w swoim strumieniu renderowania mapy.
- **Zaleznosci wyjsciowe (Outbound):** 
  - `src/EterLib/GrpScreen.h` (dziedziczenie funkcji pomocniczych 2D, takich jak `SetOrtho2D`, `SetDiffuseColor`, `RenderBar2d`).
  - `src/EterLib/StateManager.h` (korzysta z globalnego singletona `STATEMANAGER` zarzadzajacego modyfikacjami maszyny stanow Direct3D 9, np. `D3DTS_PROJECTION`, `D3DRS_ALPHABLENDENABLE`).
- **Drzewo dyrektyw `#include`:** 
  - `src/EterLib/ScreenFilter.h` wlacza: `GrpScreen.h`.
  - `src/EterLib/ScreenFilter.cpp` wlacza: `StdAfx.h`, `ScreenFilter.h`, `StateManager.h`. Ryzyka cyklicznych zaleznosci sa marginalne.
- **Model pamieciowy:** Brak inteligentnych oraz surowych wskaznikow w obiekcie. Pamiec zarzadzana na stosie bezposrednio w klasie lub jako czlonek innej klasy (`CMapOutdoor`), parametry kolorow (`D3DXCOLOR`) przechowywane przez wartosc.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
**Tabela Klas i Struktur:**
- `CScreenFilter` | Nakladka 2D na rendering ekranowy | Rozmiar bajtowy: zalezny od instancji `CScreen` (vtable) + 4 (BOOL) + 2x1 (BYTE) + 2 (padding) + 16 (D3DXCOLOR) | Wlasciciel: Watek glowny renderujacy.

**Tabela Metod Publicznych (`CScreenFilter`):**
- `void SetEnable(BOOL bFlag)` | Zmiana aktywnosci filtra | Brak zewnetrznych skutkow ubocznych na D3D. **KRYTYCZNE ZACHOWANIE:** Funkcja ignoruje przekazany argument `bFlag` i twardo ustawia pole `m_bEnable = FALSE`.
- `void SetBlendType(BYTE bySrcType, BYTE byDestType)` | Ustawienie typow operacji alpha-blend | Brak bezposredniego wplywu na rurociag renderingu do momentu wywolania metody `Render()`.
- `void SetColor(const D3DXCOLOR & c_rColor)` | Ustawienie docelowego koloru nakladki filtra ekranowego | Bezposrednia kopia pamieciowa struktury zmiennoprzecinkowej.
- `void Render()` | Rysowanie prostokata 2D poprzez modyfikacje ustawien maszyny stanow D3D | Modyfikuje na krotki czas stany macierzy Transform (WORLD, VIEW, PROJECTION) oraz stany renderowania RenderState (ALPHABLENDENABLE, SRCBLEND, DESTBLEND). Jezeli `m_bEnable` ustawiono na `FALSE`, funkcja natychmiast konczy dzialanie nie generujac cykli GPU.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Wskaznik vtable z klasy bazowej `CScreen`.
- Offset wlasny struktury CScreenFilter `+0x00` -> `BOOL m_bEnable;`
- Offset `+0x04` -> `BYTE m_bySrcType;`
- Offset `+0x05` -> `BYTE m_byDestType;`
- Offset `+0x06` -> 2 bajty ukrytego paddingu sluzacego prawidlowemu wyrownaniu pamieci.
- Offset `+0x08` -> `D3DXCOLOR m_Color;` (16 bajtow dla 4 pol o typie float: r, g, b, a).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul zostal odizolowany od przeplywu danych pakietow. Parametry powiazane z filtrem (mgla, odcien mapy) wchodza w sklad parametrow obslugi otoczenia, znanych z plikow ustawien `.msenv` ladujacych sie z systemu dyskowego VFS. Nie korzysta z powiadomien na biezaco przez opcody GC (Game->Client).
- **Metody Pythona (`PyMethodDef`):** Modul nie eksportuje swojego API do interfejsu Python C-API. Interakcje wywolywane z warstwy narzedzi w Pythonie wystepuja bardzo posrednio, np. poprzez procedury `background.LoadEnvironment`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Modul zalezy od menedzera statycznego `STATEMANAGER`, ktory owija bezposrednie rzadania wywolan operacji COM urzadzenia Direct3D9. Interfejs z definicji musi byc ograniczony operacyjnie tylko do watku, ktory uzyskal wyciag urzadzenia od biblioteki systemowej, czyli stricte glownego watku aplikacji, by uchronic program przed blokadami i zrzutami pamieci (race conditions).
- **Zarzadzanie zasobami (RAII):** Kod na poziome filtra ekranu nie alokuje zewnetrznych obiektow (nie wola D3DXCreateTexture), dlatego nie wystepuja potencjalne memory leaks zwiazane z zapomnieniem wywolania `Release()`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Skrajnie niska szansa na rzucenie wyjatkiem "access violation". Klasa posluguje sie zmiennymi statycznymi globalnymi (`CScreen::ms_iWidth`), ktore musza byc wczesniej wypelnione poprawym rozmiarem okna.
- **KRYTYCZNA PULAPKA Z LOGIKA BIZNESOWA:** Cale rozwiazanie `CScreenFilter` ulega celowemu, recznemu zablokowaniu w kodzie zrodlowym (tzw. dead code path). Zrodlowa metoda `SetEnable` posiada wyciszony paramater formalny w pliku cial funkcjonalnych `BOOL /*bFlag*/` oraz uzywa na stale polecenia deaktywacji w formie `m_bEnable = FALSE`. Poprzez te akcje `Render()` przy kazdorazowym odpaleniu zwraca sterowanie, calkowicie ukrywajac graficzne zastosowanie tej klasy. Aby wykorzysac ponownie ten wazny modul konieczne bedzie doprowadzenie funkcji zapisu `SetEnable` do formy reagujacej na wpuszczony argument boolowski.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Przywrocenie uzytkowania zmiennej `bFlag` na rzecz modyfikacji `m_bEnable = bFlag` w `CScreenFilter::SetEnable` w pliku `.cpp`.
  2. W celu podniesienia klasy o funkcjonalnosci post-processingowe 2D w nowszych wersjach modyfikowanego klienta uzyj nowoczesnego wsparcia narzedzi w EterLib i rozszerz modyfikacje `Render()` o uzycie szaderow zamiast bazowac na prymitywach zdefiniowanych funkcja `RenderBar2d`.
  3. Upewnienie sie przed zmianami, ze wczytane paczki MSENV podaja zgodne i sensowne kolory do `CMapOutdoor`, ktore w obecnej chwili moga byc obciazone losowymi defaultowymi danymi od lat (korekta u zrodla narzedzia WorldEditor).
- **Jak debugowac i logowac:** Nalezy dodac bezposrednio podglad modyfikatora deaktywujacego filtr badajac z poziomu inspektora pamieci stan `m_bEnable` dla upewnienia, czy flagowanie srodowiska z mapy aktywuje zyczenie nalozenia wartosci na ekran. 
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Wymagane jest skompilowanie tego modulu poza standardowymi bibliotekami poprzez wdrozenie dyrektywy `-DTEST_MODE_DISABLE_STDAFX=1`. Kod testujacy (np. Doctest, Gtest) bedzie musial zapewnic podstawowe mocki dla `STATEMANAGER` by uchylic probom rzutowania makr bez bezposredniego linkowania bibliotek platformy DirectX.
