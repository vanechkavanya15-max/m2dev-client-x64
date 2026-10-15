---
task_id: "atlas_c10_02_eterpack_manager"
cluster: "SYS"
module_name: "CEterPackManager - Wirtualny System Plikow (VFS)"
target_files:
- src/PackLib/EterPackManager.cpp
- src/PackLib/EterPackManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_02_eterpack_manager.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

**Cel Biznesowy:**
Modul implementuje wirtualny system plikow (VFS) umozliwiajacy hermetyzacje fizycznej struktury katalogow i czytanie bezposrednio z archiwalnych plikow `.pck`. Dzieki niemu klient moze odczytywac dekompresowane na w locie modele, teksty i grafiki w zoptymalizowany pamieciowo sposob. Obsluguje zarowno dekompresje algorytmem ZSTD, jak i szybkie w locie rozszyfrowywanie narzutem klucza (algorytm XChaCha20, z kluczem zawartym binarnie w `config.h`). Dodatkowym mechanizmem obronnym jest wsparcie "fallback" umozliwiajace zaladowanie narzutowych lub niezakodowanych zasobow (jak BGM czy sciezki patche). 

**Wywolanie w Petli Gry:**
Modul alokowany jest na wczesnym etapie startowym klienta (`CPackManager packMgr` w `UserInterface.cpp`) podczas operacji `OnInitialize`. Jest uzywany "na zadanie" (On-Demand), ilekroc dany subsystem potrzebuje zaladowac zasoby podczas dzialania, loginu lub przelaczania mapy, czesto poprzez watki ladujace (np. `EterLib/FileLoaderThread`).

**Pelny Przeplyw Danych (Data Flow) i Sterowania (Control Flow):**
1. Na etapie uruchomienia system mapuje dane z wielu plikow archiwow za pomoca `AddPack(std::string path)` uzywajac wewnatrz mmap-u (`mio::mmap_source`). Wczytywany jest `TPackFileHeader` po czym generowany jest indeks paczki `m_index`, obrabiajac go algorytmem deszyfrujacym `DecryptData`.  
2. Kod, chcac pozyskac plik, uzywa `CPackManager::Instance().GetFile(path, result_buffer)`.
3. Sciezka zostaje znormalizowana (`NormalizePath`). Jezeli modul pracuje w trybie PackLoad (`m_load_from_pack == true`), poszukuje hash mapy `m_entries` ze wszystkimi zarejestrowanymi paczkami.
4. Gdy w archiwum zlokalizowany zostanie `TPackFileEntry`, CPack odczytuje wskazany offset pamieci, kopiuje surowe dane do bufora (`Acquire` pobierajacy pamiec z `CBufferPool`).
5. Przechodzi nastepnie przez proces deszyfrowania xchacha20 (jesli entry.encryption == 1) i ostatecznie nastepuje dekompresja przy uzyciu wlasnego dla danego watku kontekstu `ZSTD_DCtx` (`thread_local`).
6. Wynikowe zdekodowane bajty sa formatowane na `TPackFile` (pod spodem `std::vector<uint8_t>`) wracajac na zewnatrz do nadawcy. Gdy zadany plik nie znajduje sie w VFS, uruchamiany jest warunek awaryjny - proste ladowanie pliku z dysku przy uzyciu `std::ifstream`.

**Cykl zycia (Lifecycle):**
CPackManager uzywa formy instancji `CSingleton` powolywanej recznie. Przy inicjalizacji tworzy wlasny `CBufferPool`. Cykl de-alokacji zachodzi automatycznie, gdy klasa matka wkracza w faze niszczenia pod koniec zamkniecia okna gry zwalniajac pule buforow.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
Szeroki wachlarz systemow zasobow korzysta z CPackManager: 
- UI Pythona i ladowacze paczek: `PythonPackModule`, `UserInterface`, `PythonNetworkStreamPhaseLoading`.
- Mechanizmy Swiata: `MapManager`, `AreaTerrain`, `MapOutdoorLoad`, `RaceManager`, `SpeedTreeForest`.
- Elementy logiczne: `ItemManager`, `PropertyManager`.
- Rendering i Obiekty 3D: `EffectMesh`, `GrpImageTexture`, `Terrain`.
- Core i Utilitaria: `ResourceManager`, `FileLoaderThread`, `TextFileLoader`, `AudioLib/SoundEngine`.

**Zaleznosci wyjsciowe (Outbound):**
- Biblioteka bezpieczenstwa i deszyfracji: `<sodium.h>` (`crypto_stream_xchacha20_xor`).
- Kompresja: `<zstd.h>` (dekompresja danych ze slowkami slownika ZSTD).
- Memory mapping: `<mio/mmap.hpp>` dbaly o wczytywanie mapowanych plikow do RAM bez posredniego otwierania jako strumienia.
- EterLib: `CBufferPool` (optymalizacja zwiekszajaca recykling pamieci VRAM w pamieci operacyjnej klienta przy pobieraniu vectorow).

**Drzewo dyrektyw `#include` (PackManager.cpp i Pack.cpp):**
- W `PackManager.h`: `<unordered_map>`, `<mutex>`, `EterBase/Singleton.h`, `Pack.h`.
- W `Pack.h`: `<string>`, `<mio/mmap.hpp>`, `config.h`.
- W `config.h`: `<cstdint>`, `<array>`, `<vector>`, `<string>`, `<memory>`, `<unordered_map>`, `<sodium.h>`.
- W plikach zrodlowych: `EterLib/BufferPool.h`, `<fstream>`, `<filesystem>`, `EterBase/Debug.h`, `<zstd.h>`.

**Model pamieciowy:**
Zastosowany mechanizm wiaze alokacje inteligentne (shared pointers) dla rejestrow wpisow pliku (`std::shared_ptr<CPack>`). Wyniki w postacie struktur to referencje i wywolywane z pamieci vector. Pliki sa trzymane stale w RAM-ie jako okna po `mmap`. Wystepuja klasyczne wskazniki do zarzadzania instancjami pul buforowych (BufferPools).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
| Nazwa (Klasa/Strukt.) | Rola | Wielkosc/Offset | Watkowosc |
|-----------------------|------|----------------|-----------|
| `CPackManager`        | Glowny Singleton zrzeszajacy wszystkie paczki w VFS. | ~Niesprecyzowana, w wiekszosci pointery i mutex. | Mieszany, synchronizacja `m_mutex` na `AddPack`. |
| `CPack`               | Instancja pojedynczej paczki `.pck`, laczy plik dyskowy mmap. | Niewielka + duze okna pamieci przez mio_mmap. | Thread-safe dla odczytow (metody konstatne). |
| `TPackFileHeader`     | Poczatek bloku .pck. Zawiera liczbe elementow, offset danych i globalny klucz Nonce. | `24 bajty` (`pragma pack(1)`). | - |
| `TPackFileEntry`      | Cechy kazdego poszczegolnego pliku w paczce (wielkosc spakowana, niespakowana, nonce, sciezka stringowa do 260 znakow). | `260` + `24` + `8+8+8+1` = ~`309 bajtow`. | - |

**Tabela Metod Publicznych (`CPackManager`)**
| Sygnatura Metody C++ | Zwraca | Skutki Uboczne i Warunki wstepne |
|----------------------|--------|----------------------------------|
| `bool AddPack(const std::string& path)` | boolean status | Dodaje do tabeli hashujacej wpisy, chronione lock_guard(m_mutex). |
| `bool GetFile(std::string_view path, TPackFile& result)` | status odczytu | Alokuje bajty na wewnetrznej referencji TPackFile korzystajac z poola. |
| `bool GetFileWithPool(std::string_view path, TPackFile& result, CBufferPool* pPool)`| status odczytu | Zewnetrznie podany memory pool (unikniecie malloc-ow). |
| `bool IsExist(std::string_view path) const` | stan wirtualny | Prosta odpowiedz czy wirtualna instancja pliku istnieje, ma fallback na `filesystem::exists`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
- **TPackFileHeader (Offsety)**
  - `0x00`: `uint64_t entry_num`
  - `0x08`: `uint64_t data_begin`
  - `0x10`: `uint8_t nonce[24]`
- **TPackFileEntry (Offsety)**
  - `0x00`: `char file_name[261]` (maksymalnie)
  - `0x105` (przyblizenie): `uint64_t offset`
  - `0x10D`: `uint64_t file_size`
  - `0x115`: `uint64_t compressed_size`
  - `0x11D`: `uint8_t encryption`
  - `0x11E`: `uint8_t nonce[24]`

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe (Network Packets):**
Ten zrestrukturyzowany z C++23 zero-conflict VFS nie narzuca pakietow sieciowych. Caly proces zachodzi przy lokalnej warstwie plikow, przed interakcjami po siec socketowa.

**Metody Pythona (`PyMethodDef` w `PythonPackModule.cpp`):**
1. `"Exist"` -> `packExist(PyObject*, PyObject*)`
   - Sprawdza z pomoca `IsExist` istnienie wewnatrz wirtualnej instancji.
   - Zwraca staly identyfikator calkowity "i" (1 dla istnienia, 0 w przeciwnym razie).
2. `"Get"` -> `packGet(PyObject*, PyObject*)`
   - Odbiera sciezke (char*) i sprawdza zewnetrzny warunek dla rozszerzenia koncowki (`.py`, `.pyc`, `.txt`). Tylko dla powyzszych pozwala na odczyt z `GetFile()`.
   - Zwraca bezposrednio binarny wektor danych jako Pythonowski format zapisu "s#".

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

**Zasady Wielowatkowosci:**
Zasoby odczytywane sa rownolegle przez watki obslugujace rendering i asynchronicznego ladowania modelu. Dla optymalizacji, watkowy dostep w `CPackManager::AddPack()` uzywa mutexu. Sam odczyt pliku w `CPack::GetFileWithPool` nie uzywa zadnych blokad (paczki sa read-only), uzywa rowniez dla optymalizacji alokacji ZSTD `thread_local ZSTD_DCtx* g_zstdDCtx`, co calkowicie usuwa bottleneck obslugi lockow w trakcie grania.

**PotencjalnePunkty Awarii (Crash Points & Edge Cases):**
1. Przeplot systemowy i roznice sciezek: system dba o stala obsluge normalizacji (zamienia backslashe '\\' na foreward slashe '/' oraz sprowadza na male litery) - jednak w warstwie fallback zaleznej na srodowisku `std::ifstream` wciaz moga wystepowac braki wielkosci liter w Linux.
2. Zepsute lub obciete formaty `.pck` (tzw Out-Of-Bounds): System posiada silne weryfikacje pamieci `(file_size < sizeof(TPackFileHeader))` oraz badanie dlugosci `file_size < m_header.data_begin + entry.offset + entry.compressed_size` chroniace VFS przed wektorem ataku celujacym na odczyt losowych plikow z pamieci.
3. System Fallback `std::filesystem::exists` bezposrednio lapie wyjatki systemu plikow posilkujac sie parametrem bledu `std::error_code ec;`, aby uniknac fatalnych awarii bezposredniego rzucania throw-ow.

**Zarzadzanie Zasobami (RAII):**
Biblioteka do plikow VFS nie operuje posrod standardowego dealokatora (poza glownym std::shared_ptr trzymajacym archiwum). Mapowanie do pamieci uzywajac narzedzia biblioteki MIO `mio::mmap_source` zwolni mapowane pliki operacyjne w momecie skasowania singletona przez zniszczenie wskaznikow inteligentnych dla kazdego CPack.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja Dodawania Nowej Funkcji (Algorytm rozszerzania enkapsulacji np dla AES):**
1. Otworz `src/PackLib/config.h` i zaktualizuj nowe wartosci (dodaj nowe NONCE_SIZE klucza oraz zadeklaruj rozmiar).
2. Otworz `src/PackLib/Pack.cpp`. Modyfikuj blok `CPack::GetFileWithPool(..)` by obslugiwal nowy algorytm (odnajdz `switch (entry.encryption)` i zdefiniuj przypadek np. `case 2`).
3. Zastosuj nowy rodzaj szyfrowania poprzez wziecie zmapowanych danych pliku i przeslania deszyfrowania do zdekodowanego buffera przed przekazaniem do biblioteki dekompresyjnej ZSTD.
4. Upewnij sie, aby testy kompilatora zostaly uruchomione i zaadaptowane dla nowej stalej dekompresyjnej (Headless). 

**Jak debugowac i logowac:**
Zazwyczaj glowny problem podczas odczytu plikow pojawia sie z zlymi lub blednie zapisanymi sciezkami w string_view path. Nalezy postawic w miare elastyczny breakpoint w linii `CPackManager::NormalizePath` aby zweryfikowac o jaka sciezke dopytuje sie gra z pamieci cache. 

**Jak testowac (Headless Unit Test Harness):**
Rozpoczynajac budowe testow na gtest lub doctest, unikaj instancjowania calego systemu Direct3D - cala subkolekcja modulowa CPackManager jest calkowicie wyzwolona z interfejsu D3D9 oraz Python (opierajac sie jedynie na STL, zstd i sodium). 
Wykonaj instancje mocka w formie czystego pustego pliku, skompiluj recznie z naglowkami VFS (napisz binarny narzut) oraz zaladuj za posrednictwem `CPackManager mgr; mgr.AddPack()`, a pozniej przeprowadzaj jednostkowe zapytania `IsExist` sprawdzajac czy system dziala.
