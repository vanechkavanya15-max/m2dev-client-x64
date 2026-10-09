---
task_id: "atlas_c10_04_packmaker_tool"
cluster: "SYS"
module_name: "PackMaker - Narzedzie Tworzenia Paczek Gry"
target_files:
- src/PackMaker/PackMaker.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_04_packmaker_tool.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** PackMaker jest zewnetrznym narzedziem linii polecen (CLI) dzialajacym w oderwaniu od glownego procesu klienta gry Metin2. Sluzy do pakowania (z kompresja i opcjonalnym szyfrowaniem) plikow zasobow gry, tworzac archiwa w formacie `.pck`. Narzedzie przetwarza rekursywnie wskazany katalog wejsciowy, generuje tabele indeksow plikow i zapisuje je w zunifikowanym archiwum.
- **Moment wywolania:** Wywolywany poza petla gry (offline tool), podczas procesu budowania zasobow (Resource Build Pipeline / CI-CD) przez deweloperow lub zautomatyzowane skrypty builda.
- **Data Flow:** Katalog wejsciowy -> Rekursywne przejscie (`std::filesystem::recursive_directory_iterator`) -> Normalizacja sciezek wirtualnych (`ymir work/` na `d:/ymir work/`) -> Utworzenie wpisow `TPackFileEntry` z rozmiarem pliku -> Generowanie Nonce (`randombytes_buf`) dla naglowka -> Zapis naglowka (tymczasowy, rezerwacja miejsca) -> Odczyt surowego pliku -> Kompresja ZSTD (`ZSTD_compress`) -> Zapis skompresowanych danych -> (Opcjonalne szyfrowanie XChaCha20 dla `.py`) -> Zapis zaktualizowanej tabeli wpisow z zaszyfrowanym naglowkiem -> Zapis archiwum wyjsciowego na dysk.
- **Cykl zycia obiektow:** Aplikacja ma charakter jednoprzebiegowy. Alokuje pamiec dla buforow wejscia i kompresji (re-uzywane bufory ze slowem kluczowym `static std::vector<char>`), struktur `TPackFileEntry` (utrzymywane w `std::map`), wykonuje IO, po czym konczy dzialanie zwalniajac zasoby. Inicjalizuje libsodium na samym poczatku i przechodzi od razu do przetwarzania.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Skrypty budujace, deweloper odpalajacy z CLI (parametry `--input`, `--output`).
- **Zaleznosci wyjsciowe (Outbound):**
  - **Biblioteki zewnetrzne:** `argparse` (parsowanie CLI), `libsodium` (`crypto_stream_xchacha20_xor`, `randombytes_buf`), `ZSTD` (`ZSTD_compressBound`, `ZSTD_compress`, `ZSTD_isError`).
  - **Standardowa Biblioteka C++23:** `<filesystem>`, `<map>`, `<fstream>`, `<iostream>`.
  - **Kod wlasny:** `PackLib/config.h` (struktury `TPackFileHeader`, `TPackFileEntry` oraz stala `PACK_KEY`).
- **Drzewo dyrektyw `#include`:**
  - `<map>`, `<fstream>`, `<iostream>`, `<filesystem>`
  - `<zstd.h>`
  - `<argparse.hpp>`
  - `<sodium.h>`
  - `"PackLib/config.h"`
  Brak ryzyka cyklicznych zaleznosci, program jest liniowy i zalezny wylacznie od stalych naglowkow konfiguracyjnych i zewnetrznych zaleznosci.
- **Model pamieciowy:** Wektory z STL (`std::vector<char>`), wskazniki `char*`/`uint8_t*` uzywane w wywolaniach interfejsu C (libsodium i zstd). Brak zlozonych wlasnosci - wlascicielem pamieci wektorow jest stos/statyczny scope.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

- **Tabela Klas i Struktur:**
  - `TPackFileHeader` (w `PackLib/config.h`): Naglowek archiwum (packed #1). Posiada `entry_num` (uint64_t), `data_begin` (uint64_t), `nonce` (24 bajty). Rozmiar: 40 bajtow.
  - `TPackFileEntry` (w `PackLib/config.h`): Wpis pliku w indeksie (packed #1). Posiada `file_name` (char[FILENAME_MAX+1]), `offset` (uint64_t), `file_size` (uint64_t), `compressed_size` (uint64_t), `encryption` (uint8_t), `nonce` (24 bajty). Typowy rozmiar zalezy od `FILENAME_MAX`.
  - Brak klas bezposrednio definiowanych w `main.cpp`, glowna logika miesci sie w `main()`.

- **Tabela Metod Publicznych:**
  - `static void EncryptData(uint8_t* data, size_t len, const uint8_t* nonce)`
    - **Argumenty:** Pointer na surowe dane, dlugosc danych, 24-bajtowy nonce (wektor inicjujacy).
    - **Wartosc zwracana:** `void`. Modyfikuje `data` w miejscu.
    - **Pre-conditions:** `data` musi wskazywac na poprawny bufor o rozmiarze co najmniej `len`. `nonce` musi byc zrodlem z losowych danych o rozmiarze `PACK_NONCE_SIZE`. `PACK_KEY` (zdefiniowany w `config.h`) uzyty zostaje jako klucz.
    - **Skutki uboczne:** Szyfruje/Odszyfrowuje algorytmem XChaCha20 XOR zawartosc bufora.
  - `int main(int argc, char* argv[])`
    - Glowny punkt wejscia wykonujacy operacje w CLI.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  Z powodu dyrektywy `#pragma pack(push, 1)` w `config.h`, dostep do struktur jest nie wyrownany, na styku z systemami C (ffi/hooking). Offsety to dokladna suma wielkosci typow.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul ten jest aplikacja standalone (PackMaker). Nie komunikuje sie z serwerem i nie wysyla pakietow sieciowych (GC/CG).
- **Metody Pythona (`PyMethodDef`):** Modul nie eksportuje do Pythona. Wylacznie szyfruje pliki ze skryptami Pythona (`.py`), ktore moga byc odczytywane przez klienta pozniej. Szyfrowanie to identyfikowane jest za pomoca flagi `encryption = 1` w strukturze `TPackFileEntry`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Aplikacja jest calkowicie jednowatkowa (ang. single-threaded). Nie stwarza zagrozen data race, iteruje po plikach sekwencyjnie w petli glownej.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Inicjalizacja Sodium:** Jesli `sodium_init() < 0`, konczy przerwaniem (`EXIT_FAILURE`).
  - **Przetwarzanie duzych plikow:** Bufory pamieciowe sa alokowane i rozszerzane do rozmiaru kompresowanego pliku. Wielkie pliki GB+ moga spowodowac OOM, uzywane sa `std::vector::resize()` dla calego pliku na raz (wczytywanie jednym blikiem).
  - **Z STD Errors:** W wypadku nieudanej kompresji `ZSTD_isError`, narzedzie rzuca komunikat i konczy z bledem.
- **Zarzadzanie zasobami (RAII):** Wykorzystuje mechanizmy `std::ifstream`, `std::ofstream`, `std::vector`, obslugujace sprzatanie automatycznie. Obiekty `static std::vector<char>` uzyte jako optymalizacja by nie alokowac pamieci dla kazdego pliku od nowa - moze prowadzic do duzego uzycia peak-RAM, ale w CLI to bez znaczenia dla stabilnosci gry.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zmodyfikuj `argparse::ArgumentParser program` w `main.cpp` by dodac nowy parametr.
  2. Pobierz go uzywajac `program.get<T>("--flaga")`.
  3. Zastosuj nowa logike bezposrednio w glownej petli for (np. pomijanie plikow, rozne algorytmy kompresji).
  4. Skompiluj cel przez `cmake --build build --target PackMaker`.
- **Jak debugowac i logowac:** Narzedzie to uzywa standardowego strumienia na bledy `std::cerr << ...`. Mozna dopisywac komunikaty debugowania do `std::cout`. Narzedzie najlepiej debugowac wywolujac je bezposrednio w CLI np. `bin/PackMaker --input sciezka_testowa/`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Jest to narzedzie bez interfejsu graficznego (CLI). Wymaga tylko skryptu basha, ktory przygotuje testowy folder (np. utworzy pliki binarne i `.py`), uruchomi binarke `PackMaker`, a nastepnie programem do dekompresji zweryfikuje, czy rozpakowane zasoby sumuja sie poprawnie pod katem skrotow SHA-256 w relacji ze zrodlowymi plikami. Z STD i XChaCha20 powrotna odszyfrowuje poprawnie.
