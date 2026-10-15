---
task_id: "atlas_c08_04_grid_slot_window"
cluster: "UI"
module_name: "CGridSlotWindow - Siatka Wieloslotowa Ekwipunku"
target_files:
- src/EterPythonLib/PythonGridSlotWindow.cpp
- src/EterPythonLib/PythonGridSlotWindow.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_04_grid_slot_window.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Rola modulu:**
Klasa `CGridSlotWindow` zarzadza siatka inwentarza pozwalajaca na wizualizacje i interakcje z przedmiotami zajmujacymi wiecej niz jedno pole (np. bronie 1x2, zbroje 1x3). Jej glownym zadaniem jest obliczanie zajetosci obszarow siatki, wykrywanie, nad ktorymi slotami znajduje sie kursor podczas przeciagania przedmiotu (mechanika Drag&Drop) oraz wizualne informowanie gracza, czy moze w danym miejscu upuscic przedmiot.

**Cykl wywolan (Render & Update Tick):**
Klasa operuje glowie na watku renderowania. Funkcja `OnRenderPickingSlot` jest wywolywana, gdy okno z siatka jest rysowane i analizuje stan Singletona `CWindowManager`, aby podswietlac modyfikowane miejsca w kolorze zielonym/bialym (mozna polozyc), czerwonym (kolizja, niemozliwe) lub zoltym (uzycie przedmiotu na obiekcie, np. Refine Scroll).

**Przeplyw danych i Lifecycle:**
- **Alokacja/Inicjalizacja:** Obiekt klasy wywolywany jest dla warstwy skryptow (dziedziczy po okienku Python UI). Najpierw nastepuje `__Initialize()`, a potem z poziomu skryptu wykonuje sie `ArrangeGridSlot`, definiujac ilosc wierszy, kolumn oraz odstepy, a takze zasilajac tablice pol.
- **Odswiezanie:** Podpiete pod `OnRefreshSlot`. Metoda synchronizuje stan zajetosci: dla obiektow zajmujacych `X` komorek uzywa glownego numeru slotu i powiela go do sasiadujacych pol (`dwCenterSlotNumber`), umozliwiajac zachowanie integralnosci przedmiotu wieloslotowego.
- **Dealokacja:** Przed skasowaniem okienka (lub na nowym `ArrangeGridSlot`) uruchamiane jest `Destroy`, resetujac wektor `m_SlotVector`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
  - Menedzer UI Pythona powolujacy klase oraz klasa bazowa `CSlotWindow`.
  - Skrypty interfejsu deklarujace rozmiary okien ekwipunku, okien wymiany, itp.

- **Zaleznosci wyjsciowe (Outbound):**
  - `UI::CWindowManager` (Singleton) - odpytywany o to, czy kursor "trzyma" obiekt (`IsAttaching`, `GetAttachingSlotNumber`, `GetAttachingIconSize`).
  - `CPythonGraphic` (Singleton) - renderowanie kwadratow informacyjnych nad siatka (`RenderBar2d`, `SetDiffuseColor`).
  - `EterBase/CRC32.h` - dla implementacji RTTI okna w postaci `Type()`.

- **Drzewo dyrektyw `#include`:**
  - `StdAfx.h` (Precompiled Headers)
  - `EterBase/CRC32.h`
  - `PythonGridSlotWindow.h` (Zawiera deklaracje oraz dolacza `PythonSlotWindow.h`)

- **Model pamieciowy:**
  - Tablica siatki oparta jest o wewnetrzny czlonek: `std::vector<TSlot *> m_SlotVector`. Wektor ten przetrzymuje WYLACZNIE czyste wskazniki typu weak do struktur zainicjalizowanych na liscie `m_SlotList` przez klase matke `CSlotWindow`. Zapobiega to wyciekom, ale nakazuje bezwzgledna walidacje.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wielkosc | Wlasciciel Watku |
|---|---|---|---|
| `UI::CGridSlotWindow` | Okno ekwipunku w formie siatki ze wsparciem obiektow wieloslotowych. | - | Watek Glowny (UI/Render) |

### Tabela Metod Publicznych / Chronionych
| Sygnatura C++ | Argumenty | Wartosc zwracana | Warunki wstepne i skutki uboczne |
|---|---|---|---|
| `ArrangeGridSlot` | `DWORD dwStartIndex, dwxCount, dwyCount, int ixSlotSize, iySlotSize, int ixTemporarySize, iyTemporarySize` | `void` | Rozmieszcza tablice na nowo. Czysci stary bufor `m_SlotVector`. Ustawia pozycje i rozmiary slotow. |
| `GetPickedSlotPointer` | `TSlot ** ppSlot` | `BOOL` | Wypelnia podany wskaznik wybranym obiektem ze wskazanej grupy wieloslotowej (z reguly celuje w lewy gorny rog klastra). |
| `GetPickedSlotList` | `int iWidth, int iHeight, std::list<TSlot*> * pSlotPointerList` | `BOOL` | Zbiera do listy wszystkie wskazniki `TSlot`, ktore kryja sie pod polem zajmowanym przez przeciagany przedmiot. |
| `GetGridSlotPointer` | `int ix, int iy, TSlot ** ppSlot` | `BOOL` | Tlumaczy wspolrzedne X i Y na indeks 1D `ix + iy * m_dwxCount`. Wykonuje range-check. |
| `GetPickedGridSlotPosition`| `int ixLocal, int iyLocal, int * pix, int * piy` | `BOOL` | Pozyskuje kordy 2D siatki (index X, index Y) na podstawie lokalnych pixeli myszy wewnatrz kontrolki. |
| `CheckMoving` | `DWORD dwSlotNumber, DWORD dwItemIndex, const std::list<TSlot*> & c_rSlotList`| `BOOL` | Waliduje logike umieszczania: sprawdza, czy docelowe zmapowane sloty sa poprawne dla relokowanego itemu. |
| `OnRefreshSlot` | `void` | `void` | Przypisuje `dwCenterSlotNumber` od slotu poczatkowego po wezlach zajetych przez rozszerzenie graficzne obiektu. |
| `OnRenderPickingSlot` | `void` | `void` | Podswietla pola przy nakierowywaniu uzywajac logiki grafiki 2D EterLib. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `m_dwxCount` i `m_dwyCount` trzymaja biezaca logike granic (Bounds) siatki po jej alokacji w `ArrangeGridSlot`.
- `m_SlotVector` dziala jak bufor plaski 1D (wierszowo - Row-major order). Do mapowania X/Y uzywamy formuly: `y * m_dwxCount + x`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Klasa nie wysyla bezposrednich polecen sieciowych. Pakiety zwiazane z przemieszczaniem w obrebie Gridu (np. `CG_ITEM_MOVE`, opcody wymiany) leza w gestii skryptow Python UI. Ten plik jedynie okresla uwarunkowania wizualne kolizji.
- **Python C-API:** `ArrangeGridSlot` oraz inwarianty tej klasy nie sa bezposrednio widoczne na poziomie `PyMethodDef` w tym pliku. Rejestracja C-API znajduje sie w pliku narzedziowym modulu obslugujacego wszystkie okna (`PythonWindow.cpp`).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Ze wzgledu na zaleznosci renderowania i instancje singletonow graficznych (`CPythonGraphic::Instance()`), metody okna zmuszone sa byc odpytywane jedynie przez glowna petle renderujaca aplikacji (Main GUI Thread).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  - Rozmiar twardo-zakodowany: W metodzie `GetPickedSlotList` logika iterowania klastra pionowego zostala zamknieta w twardym bloku typu `if (1 == iHeight) ... else if (2 == iHeight) ... else if (3 == iHeight)`. Jesli podano by element o wielkosci 4 wysokosci lub wiekszej - zmienne `iyStart` i `iyEnd` przypisza bledne wartosci (0), co sprowadzi logike wyszukiwania siatki do porazki wizualnej.
- **Zarzadzanie zasobami (RAII):** Kod na tym poziomie powstrzymuje sie od wywolan `new` i `delete`. Opiera bufor referencji o `std::vector`, ktorego alokacja uwalniana jest domyslnie lub jawnie w `Destroy()`. Zabezpiecza przed Memory Leakami w UI.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  Zwiekszanie wielkosci ekwipunku dla obiektow `1x4` czy `2x4`:
  1. Zidentyfikuj i przeanalizuj metode `GetPickedSlotList`.
  2. Zmodyfikuj drabinke if-else, wprowadzajac np. `else if (4 == iHeight)` definiujac przesuniecia od kliknietego slotu `iyStart` i `iyEnd` lub, w miare mozliwosci, zaadaptuj uogolniony wzor matematyczny oparty na srodku masy i przesunieciach symetrycznych kursora.
  3. Dokonaj analizy w plikach zewnetrznych, aby sprawdzic czy nie psuje to logiki wyposazenia `CheckMoving`.
- **Jak debugowac i logowac:**
  Poniewaz to modul silnie osadzony w renderze, nie stawiaj breakpointow potrafiacych zablokowac watki GPU. Zamist tego loguj output poprzez wbudowane funkcje lub `OutputDebugString` w punktach `CheckMoving` (by sprawdzic Index przenoszonego obiektu oraz wartosc validacji).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  Nalezy wymockowac Singletony Direct3D: `CPythonGraphic` i `CWindowManager`, zwracajace stale wartosci logiczne w symulowanym srodowisku. Po zainicjalizowaniu macierzy poleceniem `ArrangeGridSlot` na wyliczonym srodowisku mockowym, badaj wyjscie wektora slotow za pomoca `GetPickedGridSlotPosition`.
