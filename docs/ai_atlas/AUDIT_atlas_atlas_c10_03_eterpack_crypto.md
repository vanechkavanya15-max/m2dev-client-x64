---
task_id: "atlas_c10_03_eterpack_crypto"
cluster: "SYS"
module_name: "Szyfrowanie Archiwow i Kompresja LZO/ZSTD"
target_files:
- src/PackLib/EterPackPolicy.cpp
- src/EterBase/LzoModern.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_03_eterpack_crypto.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Audytu AI: Szyfrowanie Archiwow i Kompresja LZO/ZSTD

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul ten realizuje Wirtualny System Plikow (VFS) uzywany przez klienta gry do bezpiecznego i szybkiego wczytywania zasobow graficznych, tekstur i modeli. Zastepuje on tradycyjne narzedzia dostepu do dysku wysoce zoptymalizowanym systemem.

**Glowne zadania modulu:**
- **Szyfrowanie i ochrona wlasnosci intelektualnej:** Wykorzystuje silny algorytm strumieniowy XChaCha20 (z biblioteki libsodium) do obrony archiwow przed niepowolanym odczytem przez narzedzia 3rd party (np. boty, programy unpackujace), zastepujac tym samym slaby system algorytmow XTEA/TEA i Panthera.
- **Dekompresja w czasie rzeczywistym:** Zapewnia dekompresje algorytmami ZSTD (dla zasobow z paczek) oraz zmodernizowanym, weryfikowanym obwodowo LZO (`LzoModern.h`) uzywanym m.in. dla pakietow sieciowych lub legacy naglowkow.
- **Bezposredni dostep pamieciowy:** Mechanizm mapowania plikow (Memory-Mapped Files, `mio::mmap`) dla zerowych narzutow odczytu wejscia/wyjscia w porownaniu z API `fread()`.

**Kiedy jest to wywolywane (Cykl zycia):**
System wczytuje paczki (`CPackManager::AddPack`) glownie w fazie ladowania gry (ekran ladowania mapy). Funkcja pobierania (`GetFile`) jest wolana regularnie i wspolbieznie z roznych watkow (np. Background Terrain Loader), co wymaga precyzyjnego mechanizmu synchronizacji (dostep na read-only dla mmap_source, dekompresja watkowo-lokalna, oraz `std::mutex` na liscie wpisow).

**Przeplyw danych (Data Flow):**
1. Menedzer zasobu wola `CPackManager::GetFile("d:/ymir work/model.gr2", buf)`.
2. Zapytanie jest normalizowane do malych liter z unixowymi ukosnikami (`/`).
3. Singleton weryfikuje istnienie hasza/indeksu w tablicy mapowania `m_entries`.
4. Wywolanie instancji przypisanej paczki: `CPack::GetFileWithPool`.
5. Program tworzy okno mmap pliku z danymi od `offset`, jezeli wpis zostal oznaczony jako zaszyfrowany (`encryption == 1`), nastepuje dynamiczne odszyfrowanie przez `crypto_stream_xchacha20_xor`.
6. Wynikowy bufor jest dekompresowany z uzyciem ZSTD (gdzie slownik DCtx jest `thread_local`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** 
  - Menedzery zasobow (`CResourceManager`, `CGraphicImage`, `CMeshManager`).
  - Watki ladowania swiata i siatek terenu (`TerrainLoaderThread`).
  - Python Launcher (uruchamianie skryptow .pyc umieszczonych bezposrednio w paczce VFS).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `libsodium` (XChaCha20).
  - `zstd` (Zstandard - szybka dekompresja algorytmem slownikowym).
  - `lzo` (Tradycyjny minilzo lzo1x uzywany na warstwach sieciowych i legacy bazach).
  - `mio` (C++11 mmap_source).
  - Pule pamieci: `CBufferPool` (EterLib) dla unikania ciaglego re-alokowania.
- **Drzewo dyrektyw `#include`:** 
  - `src/PackLib/config.h`: `<sodium.h>`
  - `src/PackLib/Pack.cpp`: `<zstd.h>`, `mio/mmap.hpp`
  - `src/EterBase/LzoModern.h`: `<lzo/lzo1x.h>`, `<span>`, `<string_view>`, `<cstdint>`
- **Model pamieciowy:** 
  - Architektura zarzadzania dziala w systemie `std::shared_ptr<CPack>`, zapobiegajac wczesnemu zwolnieniu archiwum gdy jakis watek wciaz doczytuje dany plik. Bufor zasobowy wykorzystuje vectora bajtow podpietego pod wlasny system recyklingu (`CBufferPool`).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur

| Nazwa symbolu | Rola | Wielkosc (B) | Typ Watku |
|---------------|------|--------------|-----------|
| `CPackManager` | Singleton indeksujacy i grupujacy poszczegolne pakiety VFS. | N/A | Thread-safe (mutex) |
| `CPack` | Obiekt obslugujacy pojedyncze archiwum fizyczne; zarzadza mmap i offsetem. | N/A | Odczyty Read-Only (Thread-safe) |
| `TPackFileHeader` | Struktura naglowka calego pliku .pack (wejscie, dane, nonce 24B). | 40 B | Dowolny |
| `TPackFileEntry` | Meta-dane wpisu konkretnego zasobu w archiwum (rozmiary, hash, offset). | 309 B | Dowolny |
| `LzoDecompressor` | Nowoczesny dekompresator blokow chroniony przez std::span. | 0 B (Static) | Stateless |
| `LzoHeader` | Naglowek binarnego payloadu skompresowanego LZO. | 16 B | Dowolny |

### Metody Publiczne (Krytyczne interfejsy)

- **`bool CPackManager::GetFile(std::string_view path, TPackFile& result)`**
  *Argumenty:* `path` (sciezka pliku z gry, VFS), `result` (std::vector<uint8_t> jako out).
  *Zwrot:* True jesli pobrano pomyslnie.
  *Zasady:* Jesli plik znajduje sie w RAM (pula), to alokuje lub wykorzystuje bufor. Domyslnie priorytetuja wpisy paczkowe (pack), z mozliwoscia fallbacku na dysk jezeli system to akceptuje (np dla plikow /bgm).

- **`static ErrorCode LzoDecompressor::DecompressSafe(std::span<const uint8_t> input, std::span<uint8_t> output, size_t& out_written)`**
  *Argumenty:* `input` z bezpiecznym rozmiarem, bufor wyjsciowy `output`.
  *Zwrot:* Strongly-typed Enum `ErrorCode` (Success=0, InvalidInputSize=1, itp).
  *Zasady:* W pelni czysty C++20 bez efektow ubocznych, metoda sprawdzi magicy 'MCOZ', real_size wzgledem mozliwosci pojemnosciowych z std::span, minimalizujac szanse na heap-buffer-overflow.

### Pamieciowy Layout Struktur (Memory Layout & Offsets)

**TPackFileHeader (pragma pack 1):**
- Offset `0x00`: `entry_num` (uint64_t)
- Offset `0x08`: `data_begin` (uint64_t)
- Offset `0x10`: `nonce` (24 bajtow)

**TPackFileEntry (pragma pack 1):**
- Offset `0x000`: `file_name` (261 bajtow - nazwa char max + 1 null terminator)
- Offset `0x105`: `offset` (uint64_t)
- Offset `0x10D`: `file_size` (uint64_t)
- Offset `0x115`: `compressed_size` (uint64_t)
- Offset `0x11D`: `encryption` (uint8_t - 0 brak szyfrowania, 1 XChaCha20)
- Offset `0x11E`: `nonce` (24 bajtow specyficzne dla wpisu XChaCha20)

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Protokol:** LZO historycznie uzywane bylo na warstwie kompresji sieciowej gniazd, dlatego header `LzoHeader` ma magiczne liczby `LZO_MAGIC_FOURCC = 0x5A4F434D` ('MCOZ'). W protokole moga byc bezposrednie odwolania do binarnych wysylek dekompresowanych przez klase `LzoDecompressor`.
- **Mostki Python (PythonPackModule / Pack API):** CPackManager zazwyczaj udestepnia binding typu `pack.Exist(file_name)` oraz `pack.Get(file_name)` tak aby Python launcher mogl budowac ui na podstawie wczytanych czesci. Modul VFS jest sercem silnika klienta. Metody te opieraja sie w C++ bezposrednio o wywolania `CPackManager::Instance().GetFile()`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

1. **Stala `thread_local` dCtx (ZSTD):** Upewnij sie, ze w obrebie obslugiwanego watku, kontekst ZSTD jest poprawnie stworzony jednorazowo (`GetThreadLocalZSTDContext`). Zapobiega to alokacji i dealokacji stanu wezlow algorytmu slownikowego co klatke, jednak moze skutkowac drobnym wyciekiem przy naglym usunieciu watku background thread bez join'owania.
2. **Crash z Out-Of-Bounds (mmap_source):** Mapowanie uzywa bezposrednio wskaznika wirtualnej pamieci operacyjnej do pliku na dysku twardym systemu gracza. Jakikolwiek odczyt poza granice wielkosci zwroci twardy Segment Fault ze strony Systemu Operacyjnego, stad rygorystyczne checki (if size < xyz) w `Load`. Nigdy ich nie ignoruj!
3. **Konflikt Pragma Pack:** Struktury `TPackFileHeader` i `LzoHeader` sa skompresowane w pamieci by osiagnac staly padding sieciowy/plikowy 1 B. Proba zmapowania referencji bez `pack(push, 1)` spowoduje corrupt memory alignment od strony CPU.
4. **Zasada Zero-Trust na Headerze LZO:** Wartosc `header->real_size` z pliku nigdy nie moze byc zrodlem do reallokacji pamieci bez limitera rozmiaru; atakujacy serwer (w emulacji) moze wyslac miliardy do alokacji (OOM exploit). Uzywaj CBufferPool limitowanych wielkosci.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

### Instrukcja dodawania nowej funkcji (Rozbudowa Kompresji)
Wprowadzenie formatu kompresji jak LZ4:
1. Rozszerz enum typu szyfrowania lub nowa flage logiczna `compression_type` dla wpisow generowania w pakiecie, lub zaloz ze `encryption == 2` to LZ4.
2. Wejdz do `src/PackLib/Pack.cpp`, podmien lub dopisz instrukcje obslugi typu dekodowania wewnatrz switch(entry.encryption) (po de-kryptografii).
3. Wywolaj swoj blok dekompresyjny (lz4_decompress) bezposrednio do wynikowego wektora i zrob assert wzgledem `decompressed_size == entry.file_size`.

### Jak debugowac i logowac
Glowne wektory awarii to niedopasowane wielkosci plikow lub bledne wyliczenia nonce.
1. Wydrukuj `entry.file_name` uzywajac `TraceError` w sekcji decrypt (lub uzyj gdb hooka na `DecryptData`),
2. Dla weryfikacji LZO, przelicz offset magic bytes ('MCOZ'), z ktorego powodu Metin2 podwojnie umieszcza blok "MCOZ", zmuszajac nas do uzycia `header_offset = sizeof(LzoHeader) + sizeof(uint32_t)`. Wiele modow rozszerzajacych gubi sie na tym offsecie.

### Headless Unit Testing
`LzoDecompressor` idealnie testuje sie za pomoca biblioteki `doctest`. Ze wzgledu na brak polaczen z DirectX i singletonami UI, mozna wykonac na nim bezposrednie wywolanie z randomizowanym buforem wejsciowym i std::span z uzyciem mockowanego `malloc`.
CPack wymaga istnienia na dysku wygenerowanego archiwum o wlasciwej strukturze TPackFileHeader. Na potrzeby Test-Harness mozemy wygenerowac na etapie startu binarny plik z kluczem w `std::ofstream`, i nastepnie dac `CPack::Load()` polecenie odczytu sciezki.
