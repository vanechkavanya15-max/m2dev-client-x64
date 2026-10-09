---
task_id: "atlas_c05_08_map_attr_loader"
cluster: "WLD"
module_name: "CMapAttrLoader - Ladowanie Siatki Atrybutow i Kolizji (server_attr)"
target_files:
- src/GameLib/MapAttrLoader.cpp
- src/GameLib/MapAttrLoader.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c05_08_map_attr_loader.md"
architecture_layer: "Swiat Gry, Teren, Kolizje i Nawigacja"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul MapAttrLoader to samodzielny i bezstanowy parser plikow atrybutow mapy (najczesciej majacych rozszerzenie .atr lub potocznie nazywanych server_attr).
Jego glownym zadaniem jest ladowanie bitowej siatki atrybutow, ktora okresla fizyczne wlasciwosci poszczegolnych komorek mapy - m.in. czy dana komorka to sciana (ATTR_BLOCK), woda (ATTR_WATER), strefa bezpieczna (ATTR_SAFE_ZONE) lub obiekt (ATTR_OBJECT).
Kod operuje niezaleznie od silnika graficznego (zgodnosc z ZERO-DIRECTX) oraz Pythona, bedac wylacznie struktura danych i zestawem narzedzi do jej bezpiecznego i kontrolowanego odczytu (RAII, span, vector).
Cykl zycia:
1. Alokacja bufora danych wejsciowych (LoadFromFile robi to ze strumienia bajtow).
2. Odczyt naglowka (magic number 2634, width, height) z kontrola wielkosci (sizeof MapAttrHeader).
3. Wyliczenie calkowitej wielkosci siatki (width * height).
4. Skopiowanie splaszczonej tablicy dwuwymiarowej bitowych mask do std::vector<uint8_t> znajdujacego sie w strukturze MapAttrData.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  Zwykle modul ten wywolywany jest podczas zmiany mapy i ladowania terenu przez menedzery z klastra swiata gry (np. CMapOutdoor, serwisy odpowiedzialne za navmeshe / siatki kolizji serwerowych w kliencie).
- **Zaleznosci wyjsciowe (Outbound):**
  Brak bezposrednich zaleznosci od silnika 3D (Granny, D3D9), Pythona ani EterLib, co czyni go super-czystym (pure C++ data struct).
- **Drzewo dyrektyw `#include`:**
  - `MapAttrLoader.h`: `<cstdint>`, `<span>`, `<string_view>`, `<vector>`
  - `MapAttrLoader.cpp`: `MapAttrLoader.h`, `<fstream>`, `<cstring>`
- **Model pamieciowy:**
  Silne typowanie rozmiarow (std::size_t), bezpieczne wektory bajtow (std::vector<uint8_t>), rzutowania z kontrola pamieci (std::memcpy). Operacje na zewnetrznym buforze odbywaja sie poprzez widoki typu `std::span<const uint8_t>`, eliminujac problem wskaznikow i recznego liczenia dlugosci.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

| Klasa / Struktura | Rola | Wielkosc | Watki / Typ |
|---|---|---|---|
| MapAttrHeader | Reprezentuje naglowek binarny atrybutow. Struct pakowany pragma (#pragma pack 1). Posiada magic, width, height. | 6 bajtow (2+2+2) | Dowolny watek |
| MapAttrData | Kontener z agregatem danych. Posiada strukture MapAttrHeader oraz wektor z jednowymiarowa siatka `attributes`. | Zalezy od wektora | Dowolny watek |
| MapAttrLoader | Glowna klasa, nie umozliwia instancjonowania bezposredniego. Posiada wylacznie metody statyczne. | N/A | Dowolny watek |
| MapAttrLoadError | Scisle typowany enum klasy (uint8_t) zwarty dla zglaszania bledow operacji wejscia/wyjscia. | 1 bajt | N/A |

| Metoda Publiczna | Sygnatura | Opis / Skutki / Zwracany Typ |
|---|---|---|
| LoadFromBuffer | `static MapAttrLoadError LoadFromBuffer(std::span<const uint8_t> buffer, MapAttrData& outData)` | Parsuje bezposrednio ze spanu. Wypelnia outData na sukces. Validuje wielkosci. Zwraca Success, InvalidSize, InvalidMagicNumber lub InvalidDimensions. |
| LoadFromFile | `static MapAttrLoadError LoadFromFile(std::string_view filePath, MapAttrData& outData)` | Odczyt z pliku przez ifstream binary. Konwertuje do wektora i wywoluje LoadFromBuffer. |

| Pamieciowy Layout | Offset | Typ i Rola |
|---|---|---|
| MapAttrHeader::magic | 0x00 | uint16_t - sprawdza prawidlowosc naglowka (2634) |
| MapAttrHeader::width | 0x02 | uint16_t - szerokosc binarnej mapy w komorkach atrybutow |
| MapAttrHeader::height | 0x04 | uint16_t - wysokosc binarnej mapy w komorkach atrybutow |

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Sam ten modul nie parsuje pakietow z sieci, jednak dane zaladowane (w tym atrybuty WATER i BLOCK) definiuja pozycje, ktore serwer uznaje za zablokowane i ich weryfikacja bedzie determinowac mozliwosc przepuszczenia wektorow do pakietu wysylajacego ruch - np. TPacketCGMove.
- **Metody Pythona (`PyMethodDef`):** Modul nie eksportuje do Pythona wylacznie sam z siebie zadnej metody, ale wyzsze instancje logiki gry korzystajace z CPythonBackground, MiniMap i serwerowych krotek terenu moga przesylac do interfejsu (UI) informacje o "Safe Zone" wyliczone z tego modulu.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Modul `MapAttrLoader` jest calkowicie thread-safe jezeli dwoch uzytkownikow nie modyfikuje jednoczesnie referencji `outData`. Klasa uzywa danych wylacznie do odczytu (`std::span<const uint8_t>`), wiec mozna go uzywac jako Work Item dla puli watkow.
- **Potencjalne punkty awarii:** Plik poddany manipulacji (np. prawidlowy magic number, podane gigantyczne `width` i `height`, ale uciety na srodku plik) jest z powodzeniem zatrzymywany w zabezpieczeniu `if (buffer.size() < requiredBufferSize)` rzucajac wczesnie `MapAttrLoadError::InvalidDimensions`, co chroni przed Buffer Over-read.
- **Zarzadzanie zasobami (RAII):** Kod we wnetrzu w calosci obslugiwany przez RAII, a glowny `attributes` vector bedzie sprzatany samodzielnie w trakcie zwalniania struktury `MapAttrData`. Brak tzw. pure pointers i brak C-style malokacji w ciele struktury. Pragma pack dba z kolei o cross-kompatybilnosc layoutow.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:**
  1. Jesli chcialbys wprowadzic nowy wariant zapisu do pliku (np. LoadFromVFS pakowanego LZO lub ZSTD), utworz dedykowana funkcje w `MapAttrLoader.h`, np. `static MapAttrLoadError LoadFromArchive()`.
  2. Dekompresje pliku wykonaj przed, w nowej warstwie abstrakcji.
  3. Czysty wektor zloz do spana uzywajac `std::span<const uint8_t>(uncompressedData)` i podaj standardowo do istniejacej funkcji `LoadFromBuffer`.
- **Jak debugowac i logowac:**
  Jezeli plik serwerowych atrybutow sprawia, ze gra sie crashuje poprzez `std::bad_alloc`, nalezy dodac logi przed `outData.attributes.resize` w pliku `.cpp`, bo oznacza to sfalszowany wektor naglowkowy rozmiaru (bardzo duze Width * Height w niekompletnym headerze przed `LoadFromBuffer`).
- **Jak testowac (Headless / Unit Test Harness):**
  Najlepiej zaimplementowac bezposrednio doctest. Wygenerowac wektor tablicowy o rozmiarze np. 16 bajtow, wypelnic naglowek (`header.magic=2634; header.width=2; header.height=2;`), wpisac 4 bajty w cialo tablicy i przekazac poprzez span do funkcji statycznej `MapAttrLoader::LoadFromBuffer`. Test bedzie szybki, dzialajacy na Linuxie i bez zadnych interakcji wejscia wyjscia ze strony serwera czy grafiki.
