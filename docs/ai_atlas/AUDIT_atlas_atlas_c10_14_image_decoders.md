---
task_id: "atlas_c10_14_image_decoders"
cluster: "SYS"
module_name: "Dekodery Formatow Obrazow (DDS, TGA, BMP, PNG)"
target_files:
- src/EterImageLib/DXT.cpp
- src/EterLib/DecodedImageData.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_14_image_decoders.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- Jaka jest dokladna funkcja tego modulu w architekturze klienta: Modul ten odpowiada za dekodowanie danych tekstur (glownie zoptymalizowanych formatow skompresowanych DXT1/DXT3/DXT5 oraz standardowych DDS, a takze ze wsparciem popularnych formatow obrazow via stb_image jak TGA, BMP, PNG itp.) i przygotowanie ich do wgrania na GPU (Direct3D 9). Przetwarza strumienie bajtow z systemu plikow VFS w postac nadajaca sie do utworzenia zasobow sprzetowych `LPDIRECT3DTEXTURE9`.
- W jakim momencie petli gry (OnUpdate / OnRender / Network Tick) ten kod jest wywolywany: Kod ten jest uzywany podczas ladowania zasobow w tle (np. `ResourceManager` w watku asynchronicznym) lub synchronicznie przed renderowaniem docelowego obiektu, niezaleznie od glownej petli Game Tick. Wywolywany "na zadanie" przez wczytywanie map, modeli czy GUI.
- Pelny opis przeplywu danych (Control Flow & Data Flow) krok po kroku: 
  1. Surowe bajty (VFS z `ResourceManager`) przekazywane sa do obiektu dekodujacego, np. `CImageDecoder::DecodeImage`.
  2. Dekoder identyfikuje format po naglowku (np. magic number 'DDS ' / 0x20534444 dla DDS).
  3. Dane sa parsowane – w przypadku DDS buforowana jest bezposrednia struktura naglowka, z ktorej wyodrebniana jest szerokosc, wysokosc, flagi kompresji (DXT) i ilosc mipmap do struktury posredniej `TDecodedImageData`. W przypadku innych formatow dane sa ladowane np. przy uzyciu stb_image z dekompresja w locie (do RGBA8).
  4. Nastepnie taki wektor bajtow z `TDecodedImageData` zostaje przeslany do obiektu `CGraphicImageTexture::CreateFromDecodedData`, a stamtd przy pomocy procedur ladowarki DX9 przenoszony na pamiec VRAM GPU w postac tekstury.
- Cykl zycia obiektow (Lifecycle: alokacja, inicjalizacja, reset, dealokacja):
  Obiekt `TDecodedImageData` to tymczasowy kontener Data Transfer Object alokujacy w stercie przestrzen na dane grafiki przez `std::vector<uint8_t> pixels`. Zyje tylko na czas transferu od funkcji parsujacej plik az do momentu wywolania interfejsu COM dla GPU, po czym jest dealokowany przez wyjscie z zakresu (RAII). Obiekty wyjsciowe D3D sa natomiast zarzadzane przez DirectX za pomoca zliczania referencji (`AddRef` / `Release`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** `CImageDecoder`, `CGraphicImageTexture::CreateFromDecodedData`, `CGraphicImage::OnLoadFromDecodedData`, `ResourceManager` oraz inne moduly ladujace zasoby.
- **Zaleznosci wyjsciowe (Outbound):** `Direct3D 9` (COM, obiekty z naglowkow `d3d9.h`), stdlib (`std::vector`, `std::cstdint`), moduly zewnetrzne `stb_image` i biblioteka kompresji/dekompresji (wsparcie formatow tekstur DirectX).
- **Drzewo dyrektyw `#include`:** `DecodedImageData.h` dolacza `<vector>`, `<cstdint>`, `<d3d9.h>`. Dyrektywy ladowarek obrazow naleza czesto do wewnetrznego kontekstu i obejmuja `<memory>`, naglowki DX9 oraz Windowsowe.
- **Model pamieciowy:** RAII przy uzyciu `std::vector` gwarantuje zapobieganie wyciekom danych na CPU. Z racji korzystania z COM interakcja z pamiecia VRAM (dla wlasciwych tekstur D3D) to interfejs z uzyciem wskaznikow. Nie ma skomplikowanych powiazan inteligentnych wskaznikow do samego bloku VRAM w tym kontenerze – sluzy wylacznie do bezpiecznego w czasie przerzutu i zwalniania posredniego bufora.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
#### Tabele Struktur i Klas
- **`TDecodedImageData`**: Kontener transferowy miedzy odczytanym strumieniem RAW z pliku/VFS a dekompresowanym w RAM obrazem. Posiada wektor surowych pikseli i struktury opisowe (wymiary, format, MIP). Przeplywa przez granice miedzy watkami asynchronicznymi a watkiem DX9, takiec glowny wektor `pixels` jest wlasnoscia stosu ladowarki.
- **`DDS_HEADER`**: Struktura mapowana z dysku (przez `pragma pack(1)`) opisujaca uklad danych wewnatrz pliku dds (wysokosc, szerokosc, maski RGBA, MipMapCount).
- **`TDecodedImageData::EFormat`**: Enum (FORMAT_UNKNOWN, FORMAT_RGBA8, FORMAT_RGB8, FORMAT_DDS).

#### Tabele Metod Publicznych
- `TDecodedImageData::Clear()`: Typ zwrotu `void`. Warunki wstepne: brak. Skutek: Zwalnia zaalokowany pamieciowo wektor `pixels` w srodku, zeruje rozmiary, ustawia domyslne mapowanie formatu i mip levels. Uzywany w procedurach ponownego uzycia obiektu lub bezpiecznego startu dekodowania.
- `TDecodedImageData::IsValid() const`: Zwraca `bool`. Wynik uzalezniony od niepustego wektora pikseli, `width > 0` i `height > 0`.
- `TDecodedImageData::GetDataSize() const`: Zwraca `size_t`. Typowo rozmiar tablicy pod spodem wektora `pixels`.

#### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `TDecodedImageData`:
  - `pixels`: `std::vector<uint8_t>` (typowo 24 bajty na x64 w std libc++)
  - `width`: `int` (4 bajty)
  - `height`: `int` (4 bajty)
  - `format`: `EFormat` (4 bajty, enum)
  - `d3dFormat`: `D3DFORMAT` (4 bajty, enum/DWORD d3d9)
  - `isDDS`: `bool` (1 bajt, wraz z paddingiem kompilatora na rzecz nastepnych pol do rownania z 4 bajtami / 8 bajtami)
  - `mipLevels`: `int` (4 bajty)

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul odpowiada wylacznie za warstwe graficzna client-side. Brak bezposrednich zaleznosci od protokolow TCP/IP. Ewentualne relacje moga byc jedynie przy ladowaniu zasobow z pominieciem pamieci flash/dysk podczas eventu patchowania, niemniej sam dekoder nie uzywa zadnego opcodu.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnie zakorzeniony w backendzie C++/EterLib/EterImageLib i nie wyeksportowano go do UI skryptowego. Komendy Pythonowe uzywaja `ui.ExpandedImageBox` badz `LoadImage` obudowane wyzej jako czarna skrzynka.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Obiekt posredni `TDecodedImageData` i parsowanie moze dzialac bezpiecznie w dedykowanych watkach do Background Loadingu poniewaz polegaja tylko na stosie podanym z VFS i wektorach C++. Wgrywanie juz istniejacych obiektow poprzez wylaczny kontekst `Direct3D 9` (urzadzenie) MUSI byc synchronizowane badz puszczone na glowny watek (albo wywolane majac `D3DCREATE_MULTITHREADED`), w innym razie spowoduje Race Condition z systemem renderowania w `CMapOutdoor` lub crash `D3DERR_INVALIDCALL`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **Buffer overflow** podczas parsowania zlosliwych obrazow z nadpisana w headerze iloscia pikseli wzgledem dlugosci pliku. W nowym kodzie zapobiega sie temu m.in. walidacja dlugosci bufora przed czytaniem `DDS_HEADER` w `DecodeDDS`.
  - Przepelnienie zasobow VRAM (`E_OUTOFMEMORY`) przy wczytywaniu gigantycznych tekstur (np 4096x4096p nieskompresowanych do RGBA8). Modul zaklada obsluge przez weryfikacje poprawnosci wyniosla przez COM HRESULT.
- **Zarzadzanie zasobami (RAII):** `std::vector<uint8_t>` automatycznie alokuje i dealokuje pamiec dla bufora obrazowego, zapobiegajac wyciekom znanym z tradycyjnych architektur bazujacych na `malloc`. 

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W wypadku dodania dekodowania formatu (np. WebP badz QOI):
  1. Otworz `ImageDecoder.h` i zadeklaruj `static bool DecodeWebP(...)`.
  2. Zaimplementuj `CImageDecoder::DecodeWebP` bazujac na alokacji wewnatrz `outImage.pixels` na podstawie dlugosci podanej ze zrodlowej biblioteki.
  3. Przypisz parametry, dodajac uprzednio nowa flage w `TDecodedImageData::EFormat` jak `FORMAT_WEBP`.
  4. Dodaj wywolanie sprawdzajace w `DecodeImage`.
- **Jak debugowac i logowac:** Przechwytywanie bledu ladowania warto wykonac poprzez dump headerow magic bytes. Obserwowanie struktury w VS z breakpoints w `TDecodedImageData::Clear` moze uchwycic czyszczenie tekstury bez powiazania jej na watek uploadowy GPU. Uzywaj wbudowanych makr TraceError aby wskazac uszkodzony plik.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W wypadku zastosowania frameworku Unit Testing (np. `doctest`) modul testowany bedzie poprzez utworzenie sztucznego bufora w bajtach udajacego np. prosty plik DXT1 z 1 mipmapa, puszczenie go przez `CImageDecoder::DecodeImage` i weryfikacje powstalego obiektu `TDecodedImageData` (asercja `IsValid() == true`, zgodnosc `outImage.width` z modelem). Operacje te omijaja GPU, dzieki czemu mozna je bezpiecznie uzywac jako 'Headless' test.

