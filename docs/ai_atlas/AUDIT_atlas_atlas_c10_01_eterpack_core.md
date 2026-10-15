---
task_id: "atlas_c10_01_eterpack_core"
cluster: "SYS"
module_name: "CPack - Nowoczesny Format Archiwow Klienta VFS (ZSTD + XChaCha20)"
target_files:
- src/PackLib/Pack.cpp
- src/PackLib/Pack.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_01_eterpack_core.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT RAPORT - Jules Swarm ATLAS (136/150)

## 1. Wstep i Kontekst Refaktoryzacji
Architektura wirtualnego systemu plikow (VFS) zostala zmodernizowana w projekcie x64 2026. Przestarzaly format `EterPack` (.epk/.eix oparty na TEA i LZO) zostal zastapiony wysokowydajnym formatem archiwow implementowanym przez klase [`CPack`](file:///E:/m2dev-client-src-mainOryginalx64/src/PackLib/Pack.h) z biblioteki `PackLib`. Nowy format bazuje na mapowaniu pamieci `mio::mmap`, kompresji Zstandard (ZSTD) oraz nowoczesnej kryptografii libsodium (XChaCha20).

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Klasa `CPack` odpowiada za:
1. **Bezzwloczne montowanie archiwow (Memory-Mapped I/O):** Za posrednictwem biblioteki `mio::mmap_source` plik archiwum jest mapowany bezposrednio w przestrzen adresowa procesu, co eliminuje koniecznosc buforowania gigabajtowych plikow w pamieci RAM.
2. **Indeksowanie plikow w pamieci:** Odczyt naglowka `TPackFileHeader` oraz tabeli indeksowej `m_index` (`std::vector<TPackFileEntry>`). Wyszukiwanie plikow odbywa sie na podstawie sumy kontrolnej lub hasza nazwy sciezki.
3. **Deszyfrowanie strumieniowe XChaCha20:** Metoda `DecryptData(uint8_t* data, size_t len, const uint8_t* nonce)` uzywa `crypto_stream_xchacha20_xor` z biblioteki `libsodium`, zapewniajac odpornosc na ataki kryptoanalityczne (eliminacja podatnosci klasycznego TEA).
4. **Wielowatkowa dekompresje ZSTD:** Rozpakowywanie zasobow (tekstur, modeli, skryptow) w locie z uzyciem wydajnych alokatorow `CBufferPool`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  - [`CPackManager`](file:///E:/m2dev-client-src-mainOryginalx64/src/PackLib/PackManager.h) - glowny zarzadca archiwow w VFS, wyszukujacy pliki wsrod zamontowanych paczek.
  - `CEterPackManager` - warstwa fasady zgodnosci wstecznej dla starszych modulow silnika.
- **Zaleznosci wyjsciowe (Outbound):**
  - `mio/mmap.hpp` - mapowanie plikow w standardzie C++20/C++23.
  - `libsodium` - procedury kryptograficzne `crypto_stream_xchacha20_xor`.
  - `zstd` - dekompresor formatu Zstandard.
  - `EterBase/BufferPool.h` - pula buforow pamieci podrzednej.
- **Drzewo dyrektyw `#include`:**
  - `Pack.h`: `<string>`, `<vector>`, `<memory>`, `<mio/mmap.hpp>`, `"config.h"`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Klasa `CPack`
- Dziedziczy: `std::enable_shared_from_this<CPack>`
- Pola prywatne:
  - `TPackFileHeader m_header` - metadane paczki (sygnatura, wersja, liczba plikow).
  - `std::vector<TPackFileEntry> m_index` - tablica wpisow plikow (hash, offset, compressedSize, realSize, flags, nonce).
  - `mio::mmap_source m_file` - obiekt zmapowanego pliku na dysku.
- Metody publiczne:
  - `bool Load(const std::string& path)` - otwiera plik i mapuje go przez `m_file.map(path)`. Wczytuje i weryfikuje naglowek oraz indeks.
  - `const std::vector<TPackFileEntry>& GetIndex() const` - dostep do indeksu dla wyszukiwania dwudzielnego.
  - `bool GetFile(const TPackFileEntry& entry, TPackFile& result)` - alokuje pamiec i dekompresuje plik do kontenera wynikowego.
  - `bool GetFileWithPool(const TPackFileEntry& entry, TPackFile& result, CBufferPool* pPool)` - wariant bezalokacyjny z uzyciem wspoldzielonej puli buforow.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- Modul VFS dziala autonomicznie w procesie klienta.
- Skrypty Pythona korzystaja z `CPack` posrednio poprzez integracje z `CPythonLauncher` oraz funkcje `__LoadScript` ladujace pliki `.py` bezposrednio z pamieci VFS.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
1. **Cykl zycia pamieci zmapowanej:** Obiekt `mio::mmap_source` musi pozostawac zywy tak dlugo, jak dlugo uzywane sa jakiekolwiek wskazniki surowe odwzorowujace dane z paczki.
2. **Bezwzgledna ochrona nonce:** Kazdy plik w archiwum posiada unikalny 24-bajtowy nonce zapisany w strukturze `TPackFileEntry`. Ponowne uzycie tego samego nonce przy tym samym kluczu narusza bezpieczenstwo XChaCha20.
3. **Odpornosc na pliki zewnetrzne:** Jesli plik o danej sciezce istnieje fizycznie w lokalnym katalogu gry i wlaczone sa flagi developerskie, VFS moze preferowac plik lokalny (Hot-Reloading).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Otwieranie pliku z paczki:**
  ```cpp
  auto pPack = std::make_shared<CPack>();
  if (pPack->Load("pack/root.pck"))
  {
      for (const auto& entry : pPack->GetIndex())
      {
          if (entry.dwFilenameHash == targetHash)
          {
              TPackFile fileData;
              pPack->GetFile(entry, fileData);
              // fileData.data zawiera odszyfrowane i zdekompresowane bajty
          }
      }
  }
  ```
- **Wspolpraca z PackMaker:** Pliki `.pck` tworzone sa za pomoca narzedzia CLI w [`src/PackMaker/PackMaker.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/PackMaker/PackMaker.cpp).
