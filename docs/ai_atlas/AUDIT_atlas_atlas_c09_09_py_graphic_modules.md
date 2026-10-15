---
task_id: "atlas_c09_09_py_graphic_modules"
cluster: "PY"
module_name: "Moduly Pythona 'grp', 'grpImage', 'grpText' - Rysowanie 2D/3D"
target_files:
- src/EterPythonLib/PythonGraphicModule.cpp
- src/EterPythonLib/PythonGraphicImageModule.cpp
- src/EterPythonLib/PythonGraphicTextModule.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_09_py_graphic_modules.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport z audytu AI: Moduly Pythona 'grp', 'grpImage', 'grpText'

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

- **Funkcja w architekturze klienta:** Moduly te stanowia warstwe posrednia (wrapper C-API) pomiedzy silnikiem Pythona (wykorzystywanym przez interfejs uzytkownika i logike gry) a rdzeniem graficznym klienta zaimplementowanym w C++ (EterLib) opartym na DirectX. Moduly udostepniaja Pythonowi funkcje pozwalajace m.in. na wyswietlanie geometrii (modul 'grp'), obrazow tekstur ('grpImage') i sformatowanego tekstu ('grpText').
- **Miejsce w petli gry (Game Loop):** Funkcje te wywolywane sa zazwyczaj asynchronicznie przez wirtualna maszyne Pythona. Skrypty interfejsu klienta w fazie rysowania klatek (Render) beda wielokrotnie uzywac tych metod do ustalania polozenia i rysowania widokow 2D/3D w kliencie. Wywolanie z poziomu UI przekazuje komendy renderowania w klatce, po OnUpdate.
- **Data Flow (Przeplyw danych):** Otrzymanie argumentow jako `PyObject*` z Pythona -> dekodowanie ich do prymitywow lub wskaznikow (np. przez `PyTuple_GetInteger`, `PyTuple_GetString`) -> przekazanie ich do wlasciwej warstwy C++ (jak singletony `CPythonGraphic`, obiekty klasy `CGraphicImageInstance`, `CGraphicTextInstance`) -> przetworzenie logiczne lub operacje renderowania uzywajace API D3D/Granny przez silnik w C++.
- **Cykl zycia (Lifecycle):**
  - Alokacja i inicjalizacja obiektow: Realizowane sa poprzez metody generate (np. `grpImageGenerate`, `grpTextGenerate`), ktore konwertuja pliki zasobow (CResource) za posrednictwem `CResourceManager` na dedykowane instancje (np. `CGraphicImageInstance::New()`).
  - Uzycie obiektu z wykorzystaniem uchwytu (adresu instancji) przekazywanego bezposrednio do Pythona w postaci long long (K). Metody te w operacjach odczytuja uchwyt z argumentow przekazanych krotka w Pythonie.
  - Dealokacja i niszczenie (np. `grpImageDelete`, `grpTextDestroy`) wywolywane sa ze skryptow; odpowiadaja one za odpowiednie sprzatanie (np. `CGraphicImageInstance::Delete(p)`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Kod wywolywany jest przez srodowisko skryptowe Pythona klienta gry - interfejs UI, w szczegolnosci biblioteka `ui.py` bazujaca na Python C-API. Moduly udostepniajace to `grp`, `grpImage` i `grpText`.
- **Zaleznosci wyjsciowe (Outbound):** Bezposrednie odwolania sa do podsystemow graficznych, takich jak `CPythonGraphic`, `CResourceManager`, `CCameraManager`, `CCullingManager`, `CGraphicImageInstance`, `CGraphicExpandedImageInstance` i `CGraphicTextInstance`. Sa one powiazane z `EterLib` klienta i wykorzystuja `DirectX 9`.
- **Drzewo dyrektyw `#include`:** W plikach uzyto tradycyjnego podejscia. M.in.:
  - `StdAfx.h` - Powszechny naglowek prekompilowany obejmujacy laczenie z bibliotekami systemowymi oraz definicje silnika (EterLib, GameLib, itp.).
  - Specjalistyczne jak `EterLib/Camera.h`, `EterLib/TextBar.h`.
  - Zagrozenie cyklicznymi zaleznosciami: Ze wzgledu na rozdzielenie klas logiki bezposrednio poza zasieg Pythona do `EterLib` zaleznosci cykliczne sa izolowane, niemniej zmiana C-API wymusza wspolgranie z kodem UI skryptow gry.
- **Model pamieciowy:** W C++ dla obiektow Pythona (przekazywanych jako instancje) dominuje przekazywanie na surowych wskaznikach (`raw pointers`), czesto eksponowanych do Pythona jako wartosci 64-bitowe (`unsigned long long` / znacznik formatttera `K`). Wymaga to manualnego zwalniania z poziomu Pythona przez dedykowane metody 'Delete' / 'Destroy'. Zastosowanie nie wspiera natywnego uzywania automatycznych wskaznikow (smart pointers) w polaczeniu ze standardowym C-API Pythona. W srodowisku wystepuje wyciek w wypadku niezainicjowania `Destroy` ze skryptu UI po zakonczeniu.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Modul grp
- **Klasy / Singletony zwiazane:** `CPythonGraphic`, `CCullingManager`, `CCameraManager`, `CTextBar`
- **Glowne metody (C-API wrapper):**
  - `grpPushState(poSelf, poArgs)` -> () ; Zapisanie stanu D3D.
  - `grpPopState(poSelf, poArgs)` -> () ; Odtworzenie stanu.
  - `grpSetColor(poSelf, poArgs)` -> () ; (r, g, b, a) Ustawia kolor rysowania.
  - `grpRenderLine(poSelf, poArgs)` -> () ; (x1, y1, x2, y2) Rysuje linie 2D.
  - `grpRenderBox(poSelf, poArgs)` -> () ; (x, y, w, h) Rysuje kontury prostokata 2D.
  - `grpCreateTextBar(poSelf, poArgs)` -> (K) ; Generuje uchwyt paska tekstowego, rozmiar (w, h).

### Modul grpImage
- **Klasy Powiazane:** `CGraphicImageInstance`, `CGraphicExpandedImageInstance`
- **Glowne metody (C-API wrapper):**
  - `grpImageGenerate(poSelf, poArgs)` -> (K) ; Zwraca uchwyt do zaalokowanej instancji `CGraphicImageInstance`.
  - `grpImageDelete(poSelf, poArgs)` -> () ; Dealokuje `CGraphicImageInstance` po uchwycie `(K)`.
  - `grpImageRender(poSelf, poArgs)` -> () ; Renderuje (rysuje na ekran) obraz o podanym uchwycie `(K)`.
  - `grpSetImagePosition(poSelf, poArgs)` -> () ; (handle, x, y).
  - `grpGetWidth(poSelf, poArgs)` -> (i) ; Pobiera szerokosc.

### Modul grpText
- **Klasy Powiazane:** `CGraphicTextInstance`
- **Glowne metody (C-API wrapper):**
  - `grpTextGenerate(poSelf, poArgs)` -> (K) ; Generuje instancje tekstowa `CGraphicTextInstance`.
  - `grpTextDestroy(poSelf, poArgs)` -> () ; Niszczy instancje po uchwycie.
  - `grpTextSetText(poSelf, poArgs)` -> () ; Przekazuje string (s) do wyswietlenia.
  - `grpTextRender(poSelf, poArgs)` -> () ; Wyrysowanie tekstu na ekranie.
  - `grpTextGetSize(poSelf, poArgs)` -> (i, i) ; Pobiera wymiary w pikselach wyrysowanego napisu.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Modul ten odpowiada wylacznie za warstwe graficzna, dlatego tez bezposrednio nie parsuje zadnych pakietow typu Client->Game (`CG`) ani Game->Client (`GC`).
- **Python C-API (`PyMethodDef`):** Eksport metod dokonywany jest standardowymi strukturami `PyMethodDef`, po czym jest laczony przez `Py_InitModule` pod nazwami: `"grp"`, `"grpImage"`, `"grpText"`. Mapowanie to obejmuje parametry takie jak uzycie klauzuli `PyTuple_GetInteger` badz `PyTuple_GetUnsignedLongLong` co wymusza aby skrypty wywolywaly metody z odp. typu argumentem (int, long long dla wskaznika, float etc.).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Wielowatkowosc i Thread Safety:** Modul nalezy traktowac jako NOT thread-safe. Wszystkie metody Pythona korzystaja z podsystemu `DirectX` co implikuje potrzebe wykonania bezposrednio w watku graficznym (Main Thread). Jesli API zostanie wywolane asynchronicznie, doprowadzi to do kolizji przy odswiezaniu pamieci.
- **Crash Points (Pulapki bezpieczenstwa):**
  - *Bledy rzutowania referencji i uchwytow (Pointers Cast):* Uchwyty (handles) zwracane przez `Generate` to tak naprawde surowe wskazniki rzutowane na `unsigned long long`. Jesli po operacji `Destroy`/`Delete` w Pythonie ui gdzies przetrzyma to `ID`, a potem probuje to wyrenderowac to C++ sprzetowo wysypie sie poprzez dostep do zdealokowanej pamieci (Dangling Pointer / Segfault).
  - Brak bezpiecznego walidatora w `Render` upewniajacego czy instancja wciaz wisi w puli pamieci (poza sprawdzaniem czy adres nie jest null, brak walidacji czy obiekt jest dalej poprawny).
- **Resource Management:** Python UI musi scisle zarzadzac uzywanymi obiektami za pomoca pary operacji `Generate` i `Delete`/`Destroy`. Jakiekolwiek opuszczenie pamieci UI bazowym w Pythonie z pominieciem wolania do API spowoduja trwale powstawanie zombie-objects.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Dodawanie nowej funkcji graficznej (Extension Guide):**
  1. Zaprojektuj i zaprogramuj glowna funkcje renderujaca bezposrednio w kodzie zrodlowym silnika `EterLib` (np. w `CPythonGraphic`).
  2. Przejdz do wybranego modulu mostka (np. `PythonGraphicModule.cpp`).
  3. Stworz funkcje proxy o sygnaturze `PyObject* grpNazwaFunkcji(PyObject* poSelf, PyObject* poArgs)`.
  4. Sparsuj argumenty uzywajac np. `PyTuple_GetInteger()` dla parametrow typu (int, float), sprawdzajac ewentualne bledy poprzez rzucenie wyjatku: `return Py_BuildException()`.
  5. Skonsumuj bezposrednie wywolanie silnika: `CPythonGraphic::Instance().NazwaFunkcji(...)`.
  6. Zwroc `Py_BuildNone()`.
  7. Dopisz funkcje rejestrujaca z API w tabeli modulu `s_methods[]`.
- **Debugowanie i Logowanie:** Aby wykluczyc powody bindowania API miedzy interfejsem Pythona a C++, dodawaj sledzenie logow standardowym `TraceError` badz narzedziem Python'a debug. W przypadku bledu wartosci argumentow upewnij sie, ze krotka przekazuje poprawnie indexy zaczynajac od `0`.
- **Testowanie bez interfejsu (Unit Test Harness):** Z uwagi na silne zakotwiczenie narzedzia Pythona C-API w strukturze okienkowej z D3D, testy jednostkowe na tym kodzie wymagalyby mocnych symulacji naglowkow `<Python.h>` z wyizolowaniem singletonow. Rekomendowane jest uzycie "dummy_test_env" ale w trybie `headless` gdzie wszystkie render-calls w silniku nizej sa nadpisywane (stub). Modyfikowanie zrodla testowego narusza architektoniczna zasade testowalnosci - kod jest nieizolowany. Zmiany bez konfliktow musza pozostac jedynie na zewnetrznym narzedziu raportowym AI.
