---
task_id: "atlas_c10_07_hashing_checksums"
cluster: "SYS"
module_name: "Algorytmy Sum Kontrolnych i Hashowania w Czasie Kompilacji"
target_files:
- src/EterBase/CRC32.cpp
- src/EterBase/CRC32Constexpr.h
- src/EterBase/FNV1aHash.h
- src/EterBase/XXHash64Constexpr.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_07_hashing_checksums.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT REPORT: Algorytmy Sum Kontrolnych i Hashowania w Czasie Kompilacji

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul ten zapewnia fundamentalne narzedzia kryptograficzne i algorytmiczne do identyfikacji zasobow, walidacji danych i zarzadzania pakietami w zmodernizowanym kliencie Metin2. Udostepnia trzy podstawowe rodziny algorytmow: CRC32, FNV-1a oraz XXHash64. Implementacje oparte na dyrektywach `constexpr` pozwalaja na ewaluacje skrotow i sum w czasie kompilacji, co znacznie redukuje koszty obliczeniowe podczas dzialania programu (np. prekompilowane ID zasobow, kody komend, identyfikatory pakietow). CRC32 jest dostepne takze w wersji operujacej na mapowanych plikach za pomoca Windows API.

- **Faza wykonania:** Kod ten nie jest przypisany do konkretnej petli (OnUpdate / OnRender) poniewaz ma charakter uzytkowy (utility). Wersje `constexpr` sa w wiekszosci ewaluowane na etapie kompilacji klienta. Funkcje narzutowe i plikowe (np. `GetFileCRC32`) sa typowo wywolywane przy ladowaniu zasobow VFS w czasie dzialania watkow ladowania lub w momecie inicjalizacji (OnInitialize). Hashowanie XXHash64, z racji na szybkosc i wydajnosc na 64-bitach, uzywane jest podczas pracy sieci i obslugi pakietow, zwlaszcza przy weryfikacji i rutowaniu.
- **Data & Control Flow:** Dane wejsciowe to bufory `std::span<const uint8_t>`, ciagi znakow `std::string_view`, klasyczne stringi C `const char*` badz uchwyty do plikow Windows (`HANDLE`). Wartosciami wyjsciowymi sa skroty (hash) o stalej szerokosci 32-bit (uint32_t/DWORD) lub 64-bit (uint64_t). Przeplyw danych charakteryzuje sie sekwencyjnym, bezstanowym pobieraniem bajtow z wejscia, aplikowaniem odpowiednich przeksztalcen (kxor, mnozenie, przesuniecia bitowe) i zwroceniem nowej zhashowanej wartosci.
- **Cykl zycia obiektow (Lifecycle):** Klasy haszujace (`FNV1aHash`, `EterBase::XXHash64`) i funkcje z `CRC32Constexpr.h` sa zupelnie bezstanowe (statyczne, constexpr). Nie alokuja pamieci, nie posiadaja konstruktorow instancji i sa calkowicie thread-safe. Natomiast starszy kod Windows w `CRC32.cpp` posiada lokalne cykle mapowania pliku (otwarcie za pomoca `CreateFile`, mapowanie przez `CreateFileMapping` i `MapViewOfFile`, odczyt CRC32, i odpowiednie czyszczenie przez `UnmapViewOfFile` i `CloseHandle`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Z racji bazowej natury tych algorytmow (`EterBase`), polegaja na nich praktycznie wszystkie inne podsystemy. Dotyczy to zarzadzania zasobami (VFS, `PackManager`), systemow pakietow sieciowych (gdzie `EterBase::XXHash64` zwraca kody bledow z `PacketError`), walidacji ladowanych map i konfiguracji string-to-ID w calej architekturze.
- **Zaleznosci wyjsciowe (Outbound):** 
  - Standardowa biblioteka C++ (`<cstdint>`, `<string_view>`, `<span>`, `<array>`, `<expected>`, `<format>`).
  - Specyficzne struktury projektu dla bledow i zwracania wynikow: `StrongTypes.h`, `Result.h`.
  - Przestarzale zaleznosci (dla `CRC32.cpp`): `StdAfx.h`, `utf8.h`, oraz funkcje systemu Windows (`CreateFileW`, `MapViewOfFile`, `GetFileSize`, etc.).
- **Drzewo dyrektyw `#include`:** 
  - Bez ryzyka zaleznosci cyklicznych w nowoczesnych, naglowkowych modulach.
  - `#include "StdAfx.h"` w plikach zrolowych jest reliktem wymagajacym standardowej konfiguracji prekompilowanych naglowkow.
- **Model pamieciowy:** W zmodernizowanym kodzie (C++23) dominuje uzycie bezpiecznych widokow pamieci tj. `std::span` i `std::string_view` (referencje, brak zarzadzania dlugoscia zycia obiektu po stronie modulu docelowego). W starych wywolaniach uzywane sa surowe wskazniki `const char*` zarzadzane zewnetrznie. W systemie mapowania plikow pojawia sie reczne zarzadzanie pamiecia przy `MapViewOfFile` gdzie wskaznik to `LPVOID`. Zwracane hashe to proste typy `uint32_t`/`uint64_t`/`DWORD` przesylane przez kopiowanie.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### CRC32 (Proceduralny w `CRC32.cpp`)
- **Tabela Metod Publicznych:**
  - `DWORD GetCRC32(const char * buf, size_t len)` - Tradycyjna, proceduralna weryfikacja sumy kontrolnej. Zwraca skrot, bazuje na buforach nielimitowanych.
  - `DWORD GetCaseCRC32(const char * buf, size_t len)` - To samo, lecz traktuje ciag jako wielkie litery przed haszowaniem.
  - `DWORD GetHFILECRC32(HANDLE hFile)` - Pobiera rozmiar, mapuje plik w pamieci uzywajac FileMapping (Windows API) i generuje CRC32.
  - `DWORD GetFileCRC32(const wchar_t* c_szFileName)` - Tworzy uchwyt do pliku pod Windows i generuje CRC32 przez `GetHFILECRC32`.
  - `DWORD GetFileCRC32(const char* fileUtf8)` - Otwiera plik po przekonwertowaniu sciezki utf-8 do wstring-a.

### EterBase::CRCTable & EterBase::GetCRC32 (`CRC32Constexpr.h`)
- **Tabela Metod Publicznych:**
  - `constexpr uint32_t GetCRC32(std::string_view text)` - Oblicza 32-bit CRC dla std::string_view w kompilacji.
  - `constexpr uint32_t GetCRC32(std::span<const uint8_t> buffer)` - Oblicza 32-bit CRC dla binarnego bufora.
- **Pamieciowy Layout Struktur:** Tabela tablicy lut (Look-Up Table) `CRCTable` jest ewaluowana jako `constexpr std::array<uint32_t, 256>` - koszt pamieciowy 1KB.

### FNV1aHash (`FNV1aHash.h`)
- **Tabela Klas i Struktur:** `FNV1aHash` - Statyczna, bezstanowa klasa narzedziowa. Wlasciciel watku - jakikolwiek watek (brak danych niestattycznych).
- **Tabela Metod Publicznych:**
  - `static constexpr uint64_t Hash(std::string_view text) noexcept` - Szybkie 64 bitowe haszowanie stringow. Zwraca niezbednie wynik haszowania.
  - `static constexpr uint64_t Hash(std::span<const uint8_t> data) noexcept` - Szybkie 64 bitowe haszowanie buforow z wykorzystaniem FNV magic. 

### EterBase::XXHash64 (`XXHash64Constexpr.h`)
- **Tabela Klas i Struktur:** `EterBase::XXHash64` - Statyczna, bezstanowa klasa narzedziowa realizujaca wydajne XXHash64.
- **Tabela Metod Publicznych:**
  - `static constexpr std::expected<uint64_t, PacketError> Hash(std::string_view text, uint64_t seed = 0) noexcept` - Funkcja frontowa obslugujaca teksty z rzucaniem "expected" jesli bufor pusty, lub zwraca wyliczony hash.
  - `static constexpr std::expected<uint64_t, PacketError> Hash(std::span<const uint8_t> buffer, uint64_t seed = 0) noexcept` - Funkcja operacyjna dla bajtow. Oczekuje danych i zwraca wyliczony skrot z XXHash64.
- **Pamieciowy Layout Struktur:** Klasa calkowicie bezstanowa - operuje na statycznych `constexpr uint64_t PRIME64_*` co nie generuje runtime data segment memory layout bloat.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Kod zawarty w zmodernizowanym EterBase, szczegolnie zalezacy od XXHash64 posiada zaleznosc od typu zwracanego: `std::expected<uint64_t, PacketError>`. Sugeruje to mocna symbioze XXHash z warstwa rutowania pakietow `Network::Routers` badz identyfikacji struktur sieciowych przed ich zdekodowaniem.
- **Metody Pythona (`PyMethodDef`):** W omawianych zrodlach narzedzi nie ma mostkow do Pythona bezposrednio. Tego typu bazowe operatory z reguly sa zamykane zewnetrznym wrapperem. 

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystkie klasy naglowkowe typu constexpr (`FNV1aHash`, `EterBase::XXHash64`, `EterBase::GetCRC32`) operuja wylacznie na parametrach funkcji (lokalnych wartosciach na stosie). Nie dziela pamieci wspoldzielonej, dlatego wywolywanie ich gdziekolwiek jest bezpieczne watkowo. Starszy mechanizm obslugi uchwytow do pliku uzywajacy `CreateFileW` rowniez uzywa lokalnych zmiennych (bez synchronizacji mutexami), dlatego samo dzialanie funkcji `GetFileCRC32` jest izolowane o ile zaden inny proces lub watek nie blokuje zapisu do tego samego pliku (Wymusza `FILE_SHARE_READ`).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - **`CRC32.cpp:GetHFILECRC32`:** Jesli `MapViewOfFile` zwroci nullptr dla pustych plikow i nie bedzie poprawnej obslugi bledu przy 0 bajtowym pliku. Aktualny patch od `//tw1x1:` zabezpiecza przed crashem przy nullpointerze.
  - **`XXHash64` i `PacketError::BufferUnderflow`:** C++23 wersje XXHash chronia uzytkownika uzywajac std::expected uodporniajac kod na puste zrodlo wskaznikowe. Trzeba zadbac, zeby poprawnie uzywac `has_value()` lub `operator bool` na zaleznosciach zwracajacych expected przed rozpakowaniem.
- **Zarzadzanie zasobami (RAII):** Wspolczesne implementacje sa zwolnione z wlasnosci pamieci (`std::span`). Jednak systemy z `CRC32.cpp` to archaiczne zrodla. Wszelkie file mappingi powinny w pelni byc opakowane w C++ RAII zamiast powielac `CloseHandle`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Zaplanuj jaka wartosc chcesz haszowac i za pomoca ktorego algorytmu (wydajnosc XXHash64 jest optymalna dla logiki, FNV1a dla unikalnych krotkich kluczy constexpr).
  2. Dodaj deklaracje / klase i stworz instancje `std::span`, nie uzywaj raw pointerow (zakazane).
  3. Wywolaj uzywajac typow `expected` i rozpakuj uzywajac `.value()`.
- **Jak debugowac i logowac:**
  Jezeli pojawiaja sie problemy z wielkoscia wielkich/malych liter przy mapowaniu zasobow klienta, pamietaj o istnieniu `GetCaseCRC32`. Podgladanie zmiennych `std::span` podczas runtime i porownywanie wyniku z narzedziami sieciowymi mozesz testowac poprawnosc zdan.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** 
  Te moduly sa wysoce adaptowalne, stanowia zupelne jednostkowe narzedzia bez zadnych dependencies na reszte infrastruktury. Mozesz zalaczac uzywajac Doctest proste testy i asercje uzywajac `static_assert(FNV1aHash::Hash("Test") == XYZ)` (dzieki constexpr).
