---
task_id: "atlas_c08_11_quest_dialogs"
cluster: "UI"
module_name: "CPythonQuest - Okna Zadan i Skryptowych Dialogow NPC"
target_files:
- src/UserInterface/PythonQuest.cpp
- src/UserInterface/PythonQuest.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_11_quest_dialogs.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: CPythonQuest - Okna Zadan i Skryptowych Dialogow NPC

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CPythonQuest` pelni funkcje globalnego menedzera (Singleton) dla aktywnych zadan (questow) oraz ich stanow widocznych dla uzytkownika w kliencie gry Metin2. Przechowuje on informacje o aktualnie trwajacych misjach, odliczanych zegarach i licznikach wymaganych do wykonania celow (np. "zabij 10 dzikich psow", "pozostaly czas: 10 min"). 

Jest to czesc warstwy Interfejsu Uzytkownika. Informacje przechowywane przez `CPythonQuest` stanowia podstawowy zrodlo danych dla skryptow Pythona, ktore renderuja okno zadan oraz ikony po lewej stronie ekranu glownego UI.

**Przeplyw danych i wywolania (Control Flow & Data Flow):**
1. Aktualizacje stanow questow pochodza z pakietow sieciowych serwera (zazwyczaj pakiety `GC_QUEST_INFO` / `GC_QUEST_CONFIRM`). 
2. Faza sieciowa klienta odbiera pakiet, analizuje zaktualizowane parametry i aktualizuje dane wywolujac metody klasy `CPythonQuest` (np. `SetQuestClockValue`, `SetQuestCounterValue`).
3. Interfejs uzytkownika zaimplementowany w Pythonie (np. `uiQuest.py`) wola metody z wyeksportowanego modulu `quest` by pobrac tytuly, zaktualizowane liczniki, timery i odpowiednio wyswietlic badz ukryc ikony questow (zwoje po lewej stronie ekranu). Timer wylicza pozostaly czas w locie opierajac sie o `CTimer`.

**Cykl zycia obiektow:**
Obiekt `CPythonQuest` to Singleton. Tworzony raz, podczas inicjalizacji aplikacji. Posiada struktury `SQuestInstance` alokowane w kontenerze `std::vector` (jako `TQuestInstanceContainer`). Dodawanie i kasowanie questow odbywa sie dynamicznie za pomoca `RegisterQuestInstance` lub `DeleteQuestInstance`. Dealokacja wektora nastepuje po zniszczeniu Singletonu lub przy wywolaniu metody `Clear()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Modul jest silnie zalezny od strumieni sieciowych. Wywolania settera takie jak `SetQuestClockValue` lub `MakeQuest` przychodza najczesciej z faz gry (`CPythonNetworkStreamPhaseGame`). Skrypty uzytkownika i interfejs GUI pisany w Pythonie wykonuja read-only pobierania wartosci poprzez metody modulowe (`questGetQuestData` itp.).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `EterBase`: `CTimer::Instance().GetCurrentSecond()` (do asynchronicznego obliczania offsetu czasu zadania).
  - `EterLib`: `CResourceManager` dla pobierania pointerow z obrazami (ikonami questow - zwojami z grafiki UI).
  - Python C-API. Zwracanie wyjatkow (`Py_BuildException`), tuple-i, argumentow i ladowanie w `initquest()`.
- **Drzewo dyrektyw `#include`:** Dolacza jedynie `StdAfx.h` i wlasny plik naglowkowy `PythonQuest.h`. Brak zauwazalnych bezposrednich powiazan z modulami z renderowania (oprocz ResourceManager i pobierania CGraphicImage, co jest bezpieczne).
- **Model pamieciowy:** W srodku uzyto `std::vector<SQuestInstance>`. Wskazniki pobierane ze struktur to czyste pointery C (`SQuestInstance **`), np. dla zwrotu bez kopii bezposredniego elementu vectora w celach tylko do zapisu lub odczytu z wewnatrz C++. Ryzyka istnienia invalidowanych pointerow wystepuja wylacznie, gdy wektor dokonuje relokacji w srodku wywolania odwolujacego, lecz ze wzgledu na uzycie watku glownego - te pointery na stosie srodkowych funkcji dzialaja sprawnie na czas jednej metody.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa | Rola | Zaleznosci Wymagane | Wlasciciel Watku |
|---|---|---|---|
| `CPythonQuest` | Singleton. Centralny rejestr questow. | `CSingleton` | Watek glowny. |
| `SQuestInstance` | Struktura danych dla pojedynczego zadania. | `std::string` | Watek glowny. |
| `FQuestInstanceCompare` | Funktor porownujacy po `dwIndex`. | - | Watek glowny. |

### Tabela Metod Publicznych (`CPythonQuest`)

| Sygnatura C++ | Wartosc zwracana | Uwagi / Skutki uboczne |
|---|---|---|
| `void Clear()` | `void` | Czysci vector wejsciowy, niszczy misje. |
| `void RegisterQuestInstance(const SQuestInstance &)` | `void` | Modyfikuje vector, kasuje stara, wklada nowa, podpina `CTimer`. |
| `void DeleteQuestInstance(DWORD)` | `void` | Odszukuje instancje za pomoca std::find_if i usuwa jesli istniala. |
| `bool IsQuest(DWORD)` | `bool` | Sprawdza samo istnienie struktury. |
| `void MakeQuest(DWORD)` | `void` | Odswieza instancje o tym samym indexie (zeruje pozostale zmienne). |
| `void SetQuestTitle(DWORD, const char *)` | `void` | Aktualizacja tytulu po sprawdzeniu indexu. |
| `void SetQuestClockName(DWORD, const char *)` | `void` | Aktualizacja nazwy stopera (np. "Czas"). |
| `void SetQuestCounterName(DWORD, const char *)` | `void` | Aktualizacja nazwy licznika. |
| `void SetQuestClockValue(DWORD, int)` | `void` | Przypisuje timer; resetuje start time aktualnym CTimer. |
| `void SetQuestCounterValue(DWORD, int)` | `void` | Setter wartosci int do okna countera. |
| `void SetQuestIconFileName(DWORD, const char *)` | `void` | Setter string do wewnetrznej sciezki tga/png. |
| `int GetQuestCount()` | `int` | Rozmiar vectora. |
| `bool GetQuestInstancePtr(DWORD, SQuestInstance **)` | `bool` | Pobiera pointer wedlug **indeksu w tablicy** a nie dwIndex misji. |

### Pamieciowy Layout Struktur

`SQuestInstance`:
- `DWORD dwIndex`: Indeks questa nadany od serwera. Klucz glowny instancji.
- `std::string strIconFileName`
- `std::string strTitle`
- `std::string strClockName`
- `std::string strCounterName`
- `int iClockValue`: Laczny przewidziany czas na misje.
- `int iCounterValue`: Obecny stan zliczonego obietku.
- `int iStartTime`: Znak czasowy zapoczatkowania obliczen offsetu czasu dla tego eventu w tickach CTimer.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Eksportowane Metody Python-C API (Module: `quest`):**
- `GetQuestCount()` -> Wywoluje `CPythonQuest::Instance().GetQuestCount()` Zwraca int.
- `GetQuestData(int quest_index)` -> Zwraca `(tytul, CGraphicImage pointer, counter_name, counter_value)`. Zwraca `Py_BuildValue("sKsi", ...)`. Przyjmuje za argument "pozycje array" (nie index ID misji). Sciezka do obrazka rozwija sie automatycznie "d:/ymir work/ui/game/quest/questicon/". Plik domyslny to "season1/icon/scroll_open.tga".
- `GetQuestIndex(int array_index)` -> Zwraca wlasciwy serwerowy `dwIndex` questa (ID) w oparciu o pozycje w liscie (array index z GetQuestCount).
- `GetQuestLastTime(int array_index)` -> Zwraca w tuple `(nazwa_czasomierza, offset_czasowy)`. Offset w czasie rzeczywistym bazujac na `CTimer`.
- `Clear()` -> Wola wewnetrzne `CPythonQuest::Clear()`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Pulapki GetQuestInstancePtr:** Zauwaz, ze `GetQuestInstancePtr` bazuje na *pozycji w tablicy (array_index)* (rozmiar `GetQuestCount()`), podczas gdy wewnetrzna `__GetQuestInstancePtr` (protected) dziala z prawdziwym serwerowym kluczem po `FQuestInstanceCompare` - `dwIndex`. W module Pythona operuje sie na kolejnych indeksach (0 do N), ktore sluzaja wylacznie do iteracji GUI, by nastepnie doczytywac `dwIndex`. 
- **Pamiec (RAII):** Kod wektora przechowuje stringi za pomoca biblioteki ze standardem C++. Ze wzgledu na zaimplementowane `std::find_if`, wydajnosc dla malej ilosci questow w `TQuestInstanceContainer` jest wysoka (O(N)), co dla GUI jest akceptowalne (ilosc questow u gracza zwykle max to parenascie).
- **Zasady wielowatkowosci:** Wszystko w tym module pracuje i powinno pracowac w jednym watku glownym gry, poniewaz wywolujemy PyObject, odwolujemy sie do `CResourceManager` bez mechanizmow blokad (`std::mutex` nie sa tutaj zastosowane).
- **CTimer Instantiation:** `iLastTime` jest obliczany dynamicznie po stronie klienta podczas renderowania w UI uzywajac Singletona `CTimer`. Uchronienie przed asynchronicznym bledem lezy po stronie precyzyjnych odpytan do `GetQuestLastTime`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Jak iterowac questy w Python UI:**
Aby poprawnie wyswietlic liste questow uzywaj:
```python
import quest
count = quest.GetQuestCount()
for i in xrange(count):
    questID = quest.GetQuestIndex(i)
    data = quest.GetQuestData(i)
    # data[0] - title
    # data[1] - image pointer
    # data[2] - counter text
    # data[3] - counter value
```

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. Aby rozszerzyc `SQuestInstance` o nowe parametry na potrzeby klienta, dodaj zmienna do struktury w `PythonQuest.h`.
2. Zaktualizuj jej wartosc we wszystkich miejscach nadpisujacych (np. `MakeQuest`).
3. Utworz `SetQuestNowyParametr` analogicznie do reszty.
4. Odbierz ja ze strony `CPythonNetworkStreamGamePhase` w zaleznosci od przychodzacych pakietow `TPacketGCQuestInfo`.
5. Eksponuj ta funkcje badz nowa wartosc poprzez dodanie argumentu w tuple w `questGetQuestData` (plik `PythonQuest.cpp`). Pamietaj zaktualizowac maske formatujaca Pythona (np. "sKsi" na "sKsii").

**Testowanie Headless:** Do analizy zachowania modulow, wygeneruj sztucznie testowe SQuestInstance przez `#ifdef _DEBUG` (np. kod w wewnetrznym inicjalizatorze `__Initialize`), ale pamietaj by w trybie powaznych zautomatyzowanych testow odizolowac UI i uzyc `CMemoryFileLoader` lub minimalnego serwera testowego, ktory pchnie odpowiednie pakiety w celu walidacji czy wektor poprawnie zapisal dane.
