---
task_id: "atlas_c03_14_dump_proto_pipeline"
cluster: "ITM"
module_name: "Pipeline DumpProto i Formaty Binarne item_proto / mob_proto"
target_files:
- src/DumpProto/dump_proto/ItemCSVReader.cpp
- src/DumpProto/dump_proto/CsvFile.cpp
- src/DumpProto/dump_proto/ItemCSVReader.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c03_14_dump_proto_pipeline.md"
architecture_layer: "Ekwipunek, Przedmioty, Handel i Gospodarka"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT_atlas_atlas_c03_14_dump_proto_pipeline

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu**: Narzedzie DumpProto dziala poza petla gry (offline tool) i sluzy do transformacji czytelnych dla czlowieka plikow tekstowych (CSV/TXT np. item_names.txt, mob_names.txt) na zoptymalizowane, szyfrowane i skompresowane pliki binarne `item_proto` oraz `mob_proto`, ktore sa nastepnie doczytywane przez wlasciwego klienta gry w trakcie fazy inicjalizacji (loading screen). Zapewnia to integralnosc i chroni przed prosta modyfikacja danych przedmiotow i potworow.
- **Wywolywanie**: Nie jest wywolywane w petli gry. Jest to narzedzie wiersza polecen generujace zasoby przed uruchomieniem klienta.
- **Data Flow**: Pliki tekstowe -> wczytywane przez `cCsvFile` / `cCsvRow` -> struktury w pamieci (ItemCSVReader parsuje teksty na enumi, wartosci numeryczne) -> bufor bajtow -> kompresja (LZO) -> szyfrowanie (XTEA, choc uzyto nowszego libsodium / lub custom keys: `g_adwItemProtoKey`, `g_adwMobProtoKey`) -> zapis binarnego pliku (np. MIPX / MMPX format).
- **Lifecycle**: Alokacja danych pliku CSV do wierszy (`cCsvRow`), zamiana stringow na enumy (`get_Item_Type_Value` itp.), budowa drzewa struktur proto, serializacja i natychmiastowa dealokacja zniszczenie struktury cCsvFile.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe**: Samodzielny proces `dump_proto.exe` parsujacy argumenty uruchomieniowe (nie wola tego zaden proces w grze).
- **Zaleznosci wyjsciowe**: Biblioteki zewnetrzne `lzo` (do kompresji) i `libsodium` (inicjalizowane przez `sodium_init()`). Moduly STL: `<iostream>`, `<fstream>`, `<vector>`, `<map>`, `<string>`, `<algorithm>`, `<unordered_map>`.
- **Drzewo dyrektyw `#include`**: `CsvFile.h` dolacza standardowe naglowki. `ItemCSVReader.h` dolacza `<iostream>`. `dump_proto.cpp` integruje je razem. Ryzyko cyklicznych zaleznosci jest zerowe.
- **Model pamieciowy**: W klasie `cCsvFile` wykorzystane sa wektory czystych wskaznikow na `cCsvRow*` zarzadzane recznie (`new` i `delete` w `Destroy()`). Parser stringow `StringSplit` zwraca raw array alokowany przez `new string[30]`, potencjalny memory leak.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

### Tabela Klas i Struktur:
| Nazwa | Rola | Wielkosc w bajtach | Wlasciciel watku |
|---|---|---|---|
| `cCsvFile` | Klasa zarzadzajaca calym plikiem CSV, przechowuje wektor wierszy. | Zalezne od wektora | Watek glowny. |
| `cCsvRow` | Dziedziczy po `std::vector<std::string>`, reprezentuje wiersz. | Zalezne od wektora | Watek glowny. |
| `cCsvAlias` | Wsparcie indeksowania kolumn. Zamienia nazwe na indeks. | Zalezne od mapy | Watek glowny. |

### Tabela Metod Publicznych:
| Metoda | Sygnatura C++ | Typy argumentow | Wartosc zwracana | Warunki wstepne i skutki |
|---|---|---|---|---|
| `get_Item_Type_Value` | `int get_Item_Type_Value(std::string inputString)` | `std::string` | `int` | Wymaga poprawnego stringa. Zwraca indeks z arType. |
| `get_Item_SubType_Value` | `int get_Item_SubType_Value(int type_value, std::string inputString)` | `int, std::string` | `int` | Zwraca sub_type. |
| `Load` | `bool cCsvFile::Load(const char* fileName, const char seperator, const char quote)` | `const char*, const char, const char` | `bool` | Odczyt z dysku. Alokuje wiersze. |
| `Save` | `bool cCsvFile::Save(const char* fileName, bool append, char seperator, char quote) const` | `const char*, bool, char, char` | `bool` | Zapis na dysk pliku CSV. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets):
`cCsvFile` pamieta wektor wskaznikow: `std::vector<cCsvRow*> m_Rows`. `cCsvRow` to std::vector stringow, wiec wewnetrzna dynamiczna alokacja STL na heapie. Trudne do podpiecia bezposrednio dla FFI z powodu niestandardowego wektora STL (rozne implementacje).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**: Modul ten jest narzedziem kompilujacym assety. Nie posiada zadnych pakietow sieciowych (CG/GC) w trakcie gry.
- **Metody Pythona (PyMethodDef)**: Nie mapuje zadnych wywolan do Pythona w srodowisku gry. Czysty C++ budujacy pliki danych (`item_proto`, `mob_proto`). Generowane binarne pliki docelowo ladowane w module GameLib przy pomocy magicznych wartosci kluczy `g_adwItemProtoKey` i `g_adwMobProtoKey`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**: Kod dziala calkowicie jednowatkowo. Wszystko dzieje sie synchronicznie w watku glownym narzedzia konsolowego.
- **Potencjalne punkty awarii (Crash Points & Edge Cases)**: Reczne zarzadzanie pamiecia w `ItemCSVReader.cpp` (`StringSplit` zwracajace tablice `new string[30]`) jest narazone na `std::bad_alloc` i wycieki pamieci (brak `delete[]` i ograniczenie do 30 elementow sztywno wpisane w kodzie, `cutAt` edge-cases).
- **Zarzadzanie zasobami (RAII)**: Parser `cCsvFile` nie wykorzystuje wektorow smart pointerow (`unique_ptr`). Reczne wywolanie `Destroy()` jest obowiazkowe. Wymaga refaktoryzacji, aby objac to semantyka RAII.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide)**: 
  1. Jesli dodajesz nowy typ przedmiotu do bazy danych (np. nowy armor), otworz `ItemCSVReader.cpp`.
  2. Dopisz nazwe (np. `"ITEM_NEW_TYPE"`) do tablicy w funkcji `get_Item_Type_Value`.
  3. Jesli to podtyp, zmien `get_Item_SubType_Value`.
  4. Skompiluj nowa wersje `dump_proto`. Uruchom to narzedzie w terminalu z odpowiednim `item_names.txt`, aby wygenerowac docelowy plik `item_proto`. 
- **Jak debugowac i logowac**: Narzedzie pisze logi standardowym wyjsciem `cout` oraz `printf`. Wlacz flagi `-DDEBUG` by uzywac `Assert` zdefiniowanych w `CsvFile.cpp`.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness)**: Narzedzie naturalnie dziala Headless jako CLI. Aby przetestowac parsing bazy danych bez klienta, wygeneruj krotki plik CSV zawierajacy nowo dodane itemy i wywolaj binarke narzedzia na tym pliku. Zweryfikuj strukture wyjsciowa dowolnym plynacym un-packerem.
