# WORKBOOK POSTEPU: STRUMIEN C - NOWOCZESNY DATA PIPELINE PROTO (DATA PIPELINE & NO-DUMPPROTO)
## Odpowiedzialny: Agent Lead C (Data Architecture & Serialization Specialist)

**Cel Strumienia:** Calkowite zastapienie archaicznego narzedzia `DumpProto` z 2004 roku nowoczesnym, deklaratywnym potokiem danych (Data Pipeline) opartym o Zstandard (`ZSTD`), dynamiczne mapowanie CSV/JSON oraz zaawansowany walidator semantyczny.  
**Standard:** C++23, zero `#pragma pack(1)` UB na architekturach x64/ARM64, wersjonowanie schematow (Schema Hash FNV-1a).

---

## 1. DOKLADNA MAPA ZAGROZEN ARCHAIZMU DUMPPROTO

1. **`src/DumpProto/`:**
   - Wykorzystuje niebezpieczne funkcje `sscanf`, `strtok` i twardo zakodowane tablice bitmask.
   - Posiada sztywne ograniczenia tablic w strukturach `TItemTable` (`aLimits[2]`, `aApplies[3]`, `alValues[6]`), co uniemozliwia dodanie nowej mechaniki bez zerwania kompatybilnosci.
   - Uzywa kompresji LZO podatnej na uszkodzenia naglowkow i brak sumy kontrolnej.
2. **Nowy potok docelowy:**
   - Zastapienie LZO przez nowoczesny ZSTD (`vendor/zstd-1.5.7`).
   - Bezpieczny parser z dynamicznym naglowkiem kolumn (brak wrazliwosci na kolejnosc kolumn w CSV).
   - Silnik walidacji: sprawdzanie poprawnosci VNUM-ow, flag wyposazenia, limitow poziomow przed serializacja.

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Obszar | Plik Zrodlowy | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **C-01** | [x] | Architektura formatu binarnego `ZPRT` (Zstd Proto Table) | `C:\JULES\plan_obszar_C_data_pipeline.md` | Dok. specyfikacji | Agent Lead C |
| **C-02** | [ ] | Parser naglowka i dekompresor ZSTD w pamieci klienta | `Client/Platform/ProtoModernReader.h/.cpp` | `test_c26_proto_modern_reader.cpp` | Jules Worker #46 |
| **C-03** | [ ] | Dynamiczny parser CSV z automatycznym dopasowaniem kolumn | `Client/Platform/DynamicCsvParser.h/.cpp` | `test_c26_dynamic_csv_parser.cpp` | Jules Worker #47 |
| **C-04** | [ ] | Deklaratywny walidator semantyczny rekordow przedmiotow | `Client/Gameplay/ItemDataValidator.h/.cpp` | `test_c26_inventory_item_validator.cpp` | Jules Worker #48 |
| **C-05** | [ ] | Deklaratywny walidator semantyczny rekordow mobow/NPC | `Client/Gameplay/MobDataValidator.h/.cpp` | `test_c26_mob_data_validator.cpp` | Jules Worker #49 |
| **C-06** | [ ] | Narzedzie CLI `ProtoCompiler2026` kompilujace CSV/JSON do binarnego ZSTD | `src/BuildTools/ProtoCompiler/main.cpp` | Test kompilacji pelnego `item_proto` | Jules Worker #50 |
| **C-07** | [ ] | Adapter wsteczny: transparentny odczyt starego formatu LZO 2004 | `Client/Platform/LegacyProtoAdapter.h/.cpp` | `test_c26_legacy_proto_adapter.cpp` | Jules Worker #51 |
| **C-08** | [ ] | Zastapienie `CItemManager::LoadItemTable` nowoczesnym loaderem | `src/GameLib/ItemManager.cpp` | Test zaladowania 5000+ itemow w <20ms | Jules Worker #52 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA C (DEFINITION OF DONE)
1. Czas ladowania kompletnej bazy `item_proto` i `mob_proto` z binarnego blobu ZSTD wynosi ponizej 25 ms.
2. Zaden rekord z blednymi flagami (np. ujemny VNUM, niepoprawny limit) nie przechodzi przez walidator.
3. Klient jest w pelni odporny na dodawanie nowych kolumn i rozszerzanie struktur przedmiotow.
