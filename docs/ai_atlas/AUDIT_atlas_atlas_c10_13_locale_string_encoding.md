---
task_id: "atlas_c10_13_locale_string_encoding"
cluster: "SYS"
module_name: "Lokalizacja, Kodowania Tekstu i Wielojezycznosc"
target_files:
- src/EterLocale/Locale.cpp
- src/EterBase/StringUtils.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_13_locale_string_encoding.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu**: Modul odpowiedzialny za zarzadzanie sciezkami i zmiennymi globalnymi zwiazanymi z lokalizacja klienta Metin2 (w pliku `Locale.cpp`) oraz dostarczanie szybkich, bezalokacyjnych narzedzi do manipulacji ciagami znakow (`StringUtils.h` - m.in. operacje typu `Trim`, `Split` oraz in-place `ToLower` wykorzystujace standard C++20 `std::string_view` i `std::span`).
- **Przeplyw danych i moment wywolania**: Zmienne globalne (`MULTI_LOCALE_PATH`, `MULTI_LOCALE_NAME`) sa ladowane we wczesnej fazie dzialania aplikacji poprzez metode `LoadConfig` czytajaca proste pliki konfiguracyjne. Operacje na stringach z `StringUtils.h` to utility (czyste funkcje `constexpr`), dostepne w calej aplikacji jako narzedzia inline w fazach parsowania konfiguracji (np. ladowania plikow tlumaczen i pakietow z serwera). Funkcje zwiazane z umiejetnosciami (`GetSkillPower`) i expem gildii (`GetGuildLastExp`) to relikty danych statycznych wykorzystywane przez UI i podsystemy gameplayu podczas updateow UI lub zapytan klienta.
- **Cykl zycia**: W przypadku modulu `Locale.cpp` mamy do czynienia z plaskim modelem stanu z buforami statycznymi. Nie nastepuje dynamiczna alokacja ani niszczenie obiektow (tablice `char` o stalym rozmiarze). Klasy w `StringUtils.h` sa projektowane jako widoki i iteratory (`SplitView`, `SplitIterator`) i nie przejmuja zarzadzania pamiecia na wlasnosc.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: 
  - Subsystem Python Application (`PythonApplication.cpp`), system interfejsu (do ladowania paczek lokalizacyjnych "locale/en", "locale/pl").
  - Skrypty UI wywolujace funkcje poprzez wrappery Pythona poszukujace wlasciwych sciezek z plikami tekstowymi (`locale_string.txt`).
  - Systemy sieciowe i parsery pakietow moga wykorzystywac `StringUtils.h` do szybkiego czytania logow i text tail.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - `Locale.cpp` polega tylko na standardowym wejsciu/wyjsciu C (`fopen`, `fgets`, `sscanf`), narzedziach EterBase (`CRC32.h` choc nieuzywane aktywnie w tym module) oraz Windowsowym interfejsie (`windowsx.h`).
  - `StringUtils.h` polega wylacznie na standardzie biblioteki standardowej C++ (`<cstdint>`, `<string_view>`, `<span>`, `<algorithm>`).
- **Drzewo dyrektyw `#include`**: W `Locale.cpp` uzywane jest `StdAfx.h`, `Locale_Interface.h`, `PythonApplication.h`, `resource.h`. Ryzyko cyklicznych zaleznosci jest niewielkie z powodu izolacji jako dostawcy sciezki.
- **Model pamieciowy**: W `Locale.cpp` dominuja stale tablice w pamieci statycznej (BSS/Data segment dla `MULTI_LOCALE_PATH`). W `StringUtils.h` obslugiwane sa wylacznie "widoki" (views/spans), co wymaga od uzytkownika zapewnienia zycia oryginalnego bufora pamieci zewnetrznej przed wywolaniem tych metod.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Zmiennych i Klas:**
- `MULTI_LOCALE_PATH_COMMON` (char[256]): Wlasciciel - Global. Przechowuje domyslna sciezke dla wspolnych plikow ("locale/common").
- `MULTI_LOCALE_PATH` (char[256]): Wlasciciel - Global. Aktywna sciezka do biezacego modulu jezykowego.
- `MULTI_LOCALE_NAME` (char[256]): Wlasciciel - Global. Skrocona nazwa regionu (np. "en", "ae").
- `EterBase::StringUtils::SplitView`: Lekki wrapper iterowalny wokolo `string_view` dla podzialu stringow. Brak narzutu pamieciowego.
- `EterBase::StringUtils::SplitIterator`: Iterator zgodny z Forward Iterator (Kategoria: `std::forward_iterator_tag`), alokowany na stosie, nie posiada wlasnosci pamieci.

**Tabela Metod Publicznych:**
- `void LoadConfig(const char* fileName)`: Czyta pierwsza linie pliku jako nazwe jezyka do zdefiniowania sciezek. Efekty uboczne: Zmienia wartosci tablic `MULTI_LOCALE_NAME` i `MULTI_LOCALE_PATH`. Moze uciac teksty na 255 bajcie.
- `unsigned GetGuildLastExp(int level)`: Czysta funkcja tablicowa. Wymagania wstepne: level <= 20 i >= 0. Wynik: wartosc uint32.
- `int GetSkillPower(unsigned level)`: Czysta funkcja tablicowa dla magii Metin2. Wymagania: level < 50.
- `const char* GetLocaleName()` / `GetLocalePath()` / `GetLocalePathCommon()`: Odkrywaja interfejs globalny do odczytu (read-only gettery).
- `bool IsRTL()`: Odpowiada czy jezyk czytany jest od prawej do lewej (obecnie hardkodowane porownanie do "ae" - Arabski).
- `constexpr std::string_view EterBase::StringUtils::TrimLeft(std::string_view str) noexcept`: Zwraca zawezony widok ciagu znakow po obcieciu whitespace'ow z lewej (nie modyfikuje oryginalu).
- `constexpr void EterBase::StringUtils::ToLower(std::span<char> buffer) noexcept`: Konwertuje bufor do malych znakow in-place za pomoca arytmetyki ASCII.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
`SplitView` oraz `SplitIterator` maja wielkosc sumy 1x wskaznika, 2x rozmiaru wielkosci (size_t) oraz typu `char`. Idealnie pasuja do przekazywania przez wartosc (pass-by-value) w rejestrach procesora (x64) wg calling convention `__fastcall`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**: Brak bezposredniego mapowania opcodow na tym poziomie modulu. Parametry pakietow, np. przy odbieraniu nazwy z serwera moga byc przetwarzane funkcjami z `StringUtils.h`.
- **Metody Pythona (`PyMethodDef`)**: To API nie jest bezposrednio owiniete jako Python Module w tym konkretnym pliku, jednak zmienne ustawione tutaj sa ladowane w `PackManager` by zaatachowac odpowiednie VFS archiwa przed przekazaniem sterowania do `app.py`. Zmienne takie zasilaja aplikacje w Py API jak `app.GetLocalePath()`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**:
  - `Locale.cpp`: Metoda `LoadConfig` nie jest "Thread-Safe". Uzywa `strcpy` i modyfikuje bufory globalne bez Mutexow. Nalezy wywolac `LoadConfig` wylacznie przed inicjalizacja modulu glownego i watkow pobocznych (w trakcie fazy pre-init).
  - `StringUtils.h`: W pelni "Thread-Safe" przy zalozeniu braku mutacji zewnetrznego bufora podczas parsowania widoku. Operacja `ToLower` z `span` mutuje bufory, wiec watek czytajacy te sama pamiec musi byc synchronizowany jesli odczyt/zapis nastepuje rownolegle.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**:
  - `GetGuildLastExp`: Posiada juz zabezpieczenie przed indeksem ujemnym i przepelnieniem granicy tablicy, unikajac Out-Of-Bounds (OOB). Podobnie dziala `GetSkillPower`.
  - Przepelnienia buforow w `LoadConfig` zlagodzone przez `_snprintf_s` i `strncpy_s` z `%255s` z uzyciem flag `_TRUNCATE`. To bezpieczne podejscie dla Windows MSVC API.
- **Zarzadzanie zasobami (RAII)**: Wszystkie klasy string utility oparte sa o widoki (`string_view` i `span`), obarczajac odpowiedzialnoscia o zarzadzanie dlugoscia zycia (lifetime) wywolujacego, unikajac "Use-After-Free". C-style API w `Locale.cpp` polega na zmiennych globalnych, wiec nie wystepuja alokacje.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**:
  1. Jesli dodajesz nowy jezyk z zaleznoscia kierunku (np. Hebrajski RTL), zaktualizuj instrukcje w `IsRTL()`.
  2. Jesli potrzebujesz kolejnych manipulacji tekstem (np. ciagow UTF-8 czy Base64), zaprogramuj je jako funkcje wolne lub constexpr wewnatrz przestrzeni `EterBase::StringUtils` zachowujac paradygmat widokow (`std::string_view` zamiast `std::string`).
  3. Zmodyfikuj `Locale_Interface.h`, eksponujac nowa funkcje.
- **Jak debugowac i logowac**: Nalezy uwazac na modyfikacje z `LoadConfig`, poniewaz od tego zalezy calkowita podstawa plikow GUI gry. W razie usterki GUI wstaw breakpoint w `LoadConfig` na wywolanie `fopen` w celu sprawdzenia poprawnosci parametru.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: Klasa `StringUtils.h` wspiera uzycie pod system testow `doctest` calkowicie w trybie standalone bez zadnych bibliotek MSVC z powodu uzycia standardu bibliotek C++17/20. Aby przetestowac narzedzia `StringUtils.h`, skompiluj plik samodzielnie uzywajac np. kompilatora `g++` w izolacji Linux i flag kompilacji C++20 bez linkowania zaleznosci graficznych Direct3D/Granny.

