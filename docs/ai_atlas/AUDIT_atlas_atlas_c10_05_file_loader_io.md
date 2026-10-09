---
task_id: "atlas_c10_05_file_loader_io"
cluster: "SYS"
module_name: "CFileLoader i Mapowanie Pamieci (Memory Mapped Files)"
target_files:
- src/EterBase/FileLoader.cpp
- src/EterBase/FileBase.cpp
- src/EterBase/MappedFileReader.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_05_file_loader_io.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu: CFileLoader i Mapowanie Pamieci (Memory Mapped Files)

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CFileLoader` oraz powiazane klasy do obslugi i mapowania plikow stanowia fundamentalna warstwe VFS (Virtual File System) dla klienta gry. Sa odpowiedzialne za wysokowydajny, bezposredni odczyt duzych wolumenow danych - tekstur, modeli, map terenu, bazy wlasciwosci obiektow i pakietow zewnetrznych.  

**Umiejscowienie w petli gry:**
Podsystem dziala glownie podczas faz ladowania (Loading Phase) zasobow gry oraz strumieniowego doczytywania danych srodowiska (np. wchodzenie w nowy chunk mapy przez postac). Czesciowo jest asynchroniczny lub dziala poza glowna petla `OnUpdate`/`OnRender` by minimalizowac zaciecia klatek. Kod bywa wolany bezposrednio podczas inicjalizacji menedzerow systemowych (np. `ItemManager`, `RaceManager`, `MapManager`).

**Przeplyw danych i cykl zycia:**
1. **Zadanie odczytu:** System wyszego poziomu (np. `TextFileLoader`, silnik modeli) zada zaladowania zasobu.
2. **Alokacja:** Nastepuje instancjonowanie `CDiskFileLoader`, `CMemoryTextFileLoader`, badz `MappedFileReader`.
3. **Mapowanie do pamieci:** W przypadku klasycznych systemow (CFileBase/DiskFileLoader) nastapi odczyt poprzez tradycyjne uchwyty I/O (`ReadFile`, `fread`). W przypadku zoptymalizowanych (MappedFileReader) tworzony jest widok mapowania w pamieci wirtualnej procesu (wywolanie `CreateFileMapping` i `MapViewOfFile`), co pozwala systemowi operacyjnemu zarzadzac ladowaniem poszczegolnych stron w trybie zerowego kopiowania (Zero-Copy).
4. **Parsowanie:** Instancje `CMemoryTextFileLoader` rozkladaja zawartosc mapowanego bufora na linie i tokeny wykorzystujac optymalizacje C++.
5. **Dealokacja:** Zniszczenie instancji powoduje RAII zamkniecie widokow plikowych (`UnmapViewOfFile`, `CloseHandle`) i odzyskanie wskaznikow.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
Klasy z tego modulu sa uzywane w kluczowych narzedziach ladowania, w tym:
- `CMemoryTextFileLoader` uzywany m.in. w `src/EffectLib/EffectMesh.cpp`, `src/GameLib/RaceManager.cpp`, `src/GameLib/MapOutdoorLoad.cpp`, `src/GameLib/MapManager.cpp`, `src/GameLib/ItemManager.cpp`, `src/GameLib/PropertyManager.cpp`, `src/UserInterface/PythonNetworkStreamPhaseLoading.cpp`, `src/UserInterface/PythonApplicationModule.cpp`, `src/UserInterface/PythonSkill.cpp` oraz `src/EterLib/TextFileLoader.cpp`, `src/EterLib/Util.cpp`.
- `MappedFileReader` uzywany jako nowoczesny, zero-kopiujacy interfejs do buforow odczytu przez sub-systemy VFS i szyfrowania.
- Skrypty Pythona poprzez wrappery takie jak te ladujace fazy sieciowe ladowania gry.

**Zaleznosci wyjsciowe (Outbound):**
- Win32 API (`windows.h`): `CreateFileW`, `CreateFileA`, `CreateFileMappingA`, `MapViewOfFile`, `UnmapViewOfFile`, `CloseHandle`, `ReadFile`, `WriteFile`, `GetFileSizeEx`.
- Biblioteka standardowa C++: `<cstdint>`, `<string_view>`, `<span>`, `<string>`, `<vector>`, `<map>`.
- System VFS/Wielowatkowy (jesli powiazane z `st_mutex` i watkami ladowania tla w Granny/EterLib).
- Kodowania: Funkcje wspierajace konwersje formatow stringow (np. `Utf8ToWide` z `<utf8.h>`).

**Drzewo dyrektyw `#include`:**
- `src/EterBase/FileBase.h`: `<windows.h>`
- `src/EterBase/FileLoader.h`: `<windows.h>`, `<vector>`, `<map>`, `"Stl.h"`
- `src/EterBase/MappedFileReader.h`: `<cstdint>`, `<string_view>`, `<span>`, `<string>`, `<windows.h>`
- `src/EterBase/FileBase.cpp`: `"StdAfx.h"`, `"FileBase.h"`, `<utf8.h>`
- `src/EterBase/FileLoader.cpp`: `"StdAfx.h"`, `"FileLoader.h"`, `<assert.h>`, `<utf8.h>`

*Ryzyka cykliczne:* Utrzymywanie niskiego poziomu powiazan (module `EterBase` to fundacja klienta), co minimalizuje zaleznosci cykliczne. Zaleznie od `StdAfx.h` wymaga jednak dyscypliny, zeby nie dolaczac calego interfejsu klienta do klas bazowych I/O.

**Model pamieciowy:**
- Przestarzale `CFileBase` / `CDiskFileLoader` wykorzystuja surowe surowe uchwyty do plikow ( `HANDLE m_hFile` / `FILE* m_fp` ) zarzadzane wewnatrz wlasnych metod tworzenia i zniszczenia, z mocnym uzyciem nagich pointerow (np. `char* m_filename`). Wiele z nich posluguje sie tradycyjnymi buforami alokowanymi na stercie i wstrzykiwanymi wskaznikami np. przez `void* dest`.
- Nowoczesny `MappedFileReader` opiera sie w 100% na architekturze RAII, uzywajac typow z C++20 (`std::span<const uint8_t>`, `std::string_view`) w celu udostepnienia czystego, bezpiecznego dostepu do zmapowanej pamieci za posrednictwem widokow. Kopiowanie klasy jest zabronione (`= delete`), wymuszajac uzycie semantyki przesuniecia (`std::move`).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa | Rola | Wielkosc | Wlasciciel Watku |
|---|---|---|---|
| `CFileBase` | Baza obslugi tradycyjnych plikow Win32 API. | ~272B (MAX_PATH+1 string + uchwyty) | Przewaznie Glowny / VFS |
| `CMemoryTextFileLoader` | Klasa narzedziowa, binduje podany w pamieci bufor, podzial na wiersze i tokenizacja (tab, spacja, token stringowy). | Zmienna (kontenery STL `std::vector`) | Watek ladujacy zasoby |
| `CMemoryFileLoader` | Interfejs odczytu bajtowego dla zaladowanego bufora w pamieci sterty. | ~12B (wskaznik + rozmiar + pozycja) | Watek ladujacy zasoby |
| `CDiskFileLoader` | Podstawowy wrapper operacji I/O ANSI C (`fopen`, `fread`) z obsluga UTF-8 do Wide string path. | ~16B (`FILE*` + int rozmiar) | Zmienna (najczesciej Main) |
| `MappedFileReader` | Nowoczesny MMF Wrapper - obsluga `CreateFileMapping` z dostepem via `std::span` (Zero-Copy). | ~32B (HANDLE x2, `void*`, `size_t`) | B/D (Dziala asynchronicznie jesli zaprzegniety) |

### Tabela Metod Publicznych

**`MappedFileReader`**
- `MappedFileReader()` / `~MappedFileReader()`
- `MappedFileReader(MappedFileReader&& other)` / `operator=(MappedFileReader&& other)`: Semantyka przesuniecia.
- `bool Open(std::string_view file_path)`: Mapuje plik sciezki jako widok tylko do odczytu (PAGE_READONLY, FILE_MAP_READ). Zwraca `false` jesli zawiedzie lub rozmiar = 0. Brak skutkow ubocznych.
- `void Close()`: Zwolnienie uchwytow zasobow.
- `std::span<const uint8_t> GetData() const`: Zwraca ciagly widok odczytu pamieci (Zero-Copy Span), na wlasny uzytek obslugi formatu.

**`CMemoryTextFileLoader`**
- `void Bind(int bufSize, const void* c_pvBuf)`: Laczy istniejacy zrzut w pamieci, rozdzielajac go wewnetrznie po liniach (uwzglednia CR/LF 
 
). Zmienia wewnetrzny stan klasy. 
- `DWORD GetLineCount()`: Pobiera ilosc wpisow po sparsowaniu.
- `bool SplitLine(DWORD dwLine, CTokenVector * pstTokenVector, const char * c_szDelimeter)`: Rozbija ciag konkretnej linii wedlug separatora, uwzgledniajac cytowanie ("). Zmienia `pstTokenVector` podany jako pointer.
- `int SplitLine2(...)` oraz `bool SplitLineByTab(...)`: Alternatywne rutyny dzialajace na tokenizatorach stringu.

**`CFileBase`**
- `BOOL Create(const char* filename, EFileMode mode)`: Wrapper CreateFile, konwertujacy wejsciowy string na UTF-16 WinAPI.
- `void Seek(DWORD offset)`: Wrappuje SetFilePointer, posiada limit na m_dwSize.
- `BOOL Write(const void* src, int bytes)` / `BOOL Read(...)`: Typowe I/O synchroniczne.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)

**`CFileBase`:**
- `+0x00`: Wirtualna tablica metod
- `+0x04`: `int m_mode` (EFileMode)
- `+0x08`: `char m_filename[261]` (Zakladajac MAX_PATH = 260)
- `+0x10C` (lub w poblizu po wyrownaniu): `HANDLE m_hFile`
- `+0x114`: `DWORD m_dwSize`

**`MappedFileReader` (Standard Layout):**
- `+0x00`: `HANDLE file_handle` (8 bajtow na x64)
- `+0x08`: `HANDLE mapping_handle` (8 bajtow)
- `+0x10`: `void* mapped_data` (8 bajtow)
- `+0x18`: `size_t size` (8 bajtow)

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Modul I/O i File Loader nie komunikuje sie *bezposrednio* poprzez pakiety sieciowe ani nie jest sam w sobie modulem dostepnym z Pythona poprzez C-API (np. `PyModule_AddFunctions`). 
Wystepuje tu jako kluczowy zewnetrzny zasob posredniczacy:
- Python moze zarzadac zaladowania struktury `.txt` lub danych (np. plikow opisu GUI, skryptow konfiguracyjnych) a zewnetrzna klasa (jak PythonApplication) uzywa CMemoryTextFileLoader do parsowania tego w twardym C++ aby podac obiekt do PyObject_Call.
- Pakiety pobierajace obiekty gry korzystaja z identyfikatorow danych zmapowanych do pamieci, by szybko instancjonowac obiekty gry - nie byloby to mozliwe w czasie rzecywistym dla watku glownego z uzyciem powolnych obslug dysku.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady wielowatkowosci:**
- Klasa `MappedFileReader` uzywa RAII i mapowania do pamieci systemowej. Pamiec zmapowana poprzez ta klase (`GetData()`) moze byc bezpiecznie wspoldzielona do czytania we wszystkich watkach aplikacji tak dlugo, jak sam instancja bazowa pliku sie nie zamyka lub zmienia (RAII). Zamkniecie pliku w jednym watku i jednoczesny odczyt z niego w innym to gwarantowany `EXCEPTION_ACCESS_VIOLATION`.
- Parsowanie `CMemoryTextFileLoader` NIE JEST THREAD-SAFE (posiada wewnetrzne stany, modyfikuje struktury `m_stLineVector`). Tworz instancje per zadanie ladowania w danym watku.

**Potencjalne punkty awarii (Crash Points & Edge Cases):**
- W `MappedFileReader::Open`, jesli plik nie istnieje badz jest o zla uprawnieniach, zwroci bezpiecznie false. Natomiast po poprawnym zmapowaniu z modyfikatorem FILE_MAP_READ uzytkownik nie moze modyfikowac buffora z pamieci operacyjnej na ktora wskazuje Span. Jakakolwiek proba odrzucenia stalej (`const_cast`) konczy sie przerwaniem SEH w Win32.
- `CFileBase` posluguje sie twardym `strncpy` i wielkoscia MAX_PATH. Sciezki glebsze lub uzywajace zaawansowanych struktur uniksowych (NTFS extended) badz w sieci moglyby zostac obciete, powodujac uszkodzone odczyty plikow (Pulapka limitow MAX_PATH w starszym Windowsie, warto byc uwaznym).

**Zarzadzanie zasobami (RAII):**
- Starsze wrappery, takie jak `CFileBase` oraz `CDiskFileLoader`, polegaja na jawnej, recznej terminacji (`Destroy()`, `Close()`), aczkolwiek destruktory dokonuja weryfikacji. 
- Zawsze korzystaj z nowego `MappedFileReader`, gdy interesuje Cie tylko odczyt pliku (np. binarki, zasoby grafiki) - korzysta z pelnej RAII semantyki przesuniecia `std::move()`, i `std::span` odcinajac od niebezpiecznych offsetow poza krawedz bufora.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
1. Ogranicz wplyw na stare systemy: Nie edytuj logiki w `CFileBase` bez absolutnej koniecznosci, sluzy jako kompatybilnosc wsteczna.
2. Zintegruj z nowoczesnym API: Jezeli musisz wgrac nowy parser (np. dla formatu XML lub nowszego Proto), zrob klase przyjmujaca `std::span<const uint8_t>` w swoim interfejsie i przekaz na jej wejscie zawartosc z `MappedFileReader`. 
3. Pozbywaj sie starych uzyc I/O: Rozbuduj C++20 narzedzia tak, aby mapowac wiecej zasobow gry prosto z RAMu za pomoca Memory-Mapped Files bez stertowego kopiowania buforow.
4. Pozytywne odrzucenia (Failing Gracefully): W `MappedFileReader`, powstrzymuj sie przed assertami na obsluge stanow. Loguj bledy `CreateFileMapping` cichym logowaniem (przy uzyciu standardowego EterBase loggera, jesli takowy istnieje) by uniknac zawieszenia gry.

**Jak debugowac i logowac:**
Z uwagi na nature IO i plikow, najlepszym sposobem na test mapowania plikow MMF jest uzycie narzedzia `Process Explorer` (Sysinternals) w Windows by sprawdzic sekcje plikowe mapowane pod procesem. Przepelnienia buffora wektorow tekstu (`SplitLine`) moga byc wylapywane narzedziem ASAN i Address Sanitizer - upewnij sie ze `std::span` i wewnetrzne string view nie zawieraja wyciekow dlugosci poza  .

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Jako ze ten modul nie wywoluje DirectX ani petli renderujacej, latwo go przetestowac w C++ unit testing (np. `doctest` / `catch2`). 
1. Stworz narzedziem pomocniczym plik testowy dysku. 
2. Utworz obiekt `MappedFileReader` podajac jego wezel. 
3. Sprawdz rozmiar. 
4. Porownaj Span wyjsciowy z bajtami wygenerowanego wczesniej pliku zewnetrznego.
5. Sprobuj skopiowac obiekt poprzez operator = w celach kompilacji i udowodnienia ze `delete` copy operator dziala.  
