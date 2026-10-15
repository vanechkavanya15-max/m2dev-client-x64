---
task_id: "atlas_c09_15_frozen_modules_runtime"
cluster: "PY"
module_name: "Frozen Python Modules - Wbudowana Biblioteka Standardowa"
target_files:
- src/PythonModules/frozen_modules.c
- src/PythonModules/frozen_modules.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c09_15_frozen_modules_runtime.md"
architecture_layer: "Mostek Pythona, Moduly C-API i Skrypty Gry"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul ten pelni role kontenera dla zamrozonych (frozen) modulow standardowej biblioteki Pythona wkompilowanych bezposrednio w plik wykonywalny klienta gry Metin2. Zamiast ladowac biblioteke standardowa Pythona (np. math, os, sys, copy, string, types) z zewnetrznych plikow .py lub .pyc na dysku (.zip), klient ma je zaszyte jako tablice bajtow (byte arrays) wygenerowane przez narzedzie `Tools/freeze/generate_legacy_frozen.py`.
- **Wywolywanie:** Kod ten jest wywolywany we wczesnej fazie inicjalizacji gry, zazwyczaj przed wejsciem w glowna petle gry, w momencie inicjalizacji maszyny wirtualnej Pythona (CPython) przez moduly takie jak `script` czy bezposrednio glowne wejscie programu (np. `WinMain` uruchamiajace interpreter).
- **Control Flow & Data Flow:** 
  1. Funkcja wejsciowa klienta wola `InitStandardPythonModules()`.
  2. Funkcja ta przypisuje wskaznik tablicy `_PyImport_FrozenModules` (zawierajacej nazwy modulow, wskazniki na ich kod bajtowy oraz rozmiary) do globalnego wskaznika CPython API `PyImport_FrozenModules`.
  3. Kiedy skrypt Pythona wykonuje instrukcje `import math` lub `import os`, mechanizm importu Pythona w pierwszej kolejnosci przeszukuje tablice `PyImport_FrozenModules`.
  4. Nastepuje wczytanie modulu prosto z pamieci operacyjnej (bez dostepu do systemu plikow).
- **Lifecycle:** 
  - Alokacja: Tablice bajtowe sa alokowane statycznie w sekcji `.data` lub `.rodata` pliku wykonywalnego.
  - Inicjalizacja: Wywolanie `InitStandardPythonModules()` przed startem srodowiska Pythona.
  - Dealokacja: Brak. Jest to pamiec statyczna (read-only data), zwolniona dopiero przy zamknieciu calego procesu klienta.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Kod wczesnej inicjalizacji silnika, odpowiedzialny za bootowanie interpretera Pythona (najprawdopodobniej `CPythonWindow` / `CPythonApp` lub odpowiednik w warstwie EterPythonLib).
- **Zaleznosci wyjsciowe (Outbound):** 
  - Wewnetrzne API CPythona (`<python/Python.h>`). Modyfikacja zmiennej globalnej struktury `_frozen * PyImport_FrozenModules`.
- **Drzewo dyrektyw `#include`:** 
  - `<python/Python.h>` - Glowne wlaczenie C-API Pythona.
  - Ryzyka cyklicznych zaleznosci: Minimalne/Brak. Modul jest wyizolowanym liscie w drzewie zaleznosci.
- **Model pamieciowy:** Czyste wskazniki C (`unsigned char *`) i tablice prekompilowanych danych wezlow C (`struct _frozen`). 

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `struct _frozen`: Struktura CPython C-API uzywana do definiowania zamrozonego modulu.
  - Rola: Przechowuje metadane modulu dla import_frozen.
  - Wielkosc: Zalezy od architektury (zazwyczaj 16 lub 32 bajty na wpis; `char* name`, `unsigned char* code`, `int size`, `int is_package`).
  - Wlasciciel watku: Modul CPython. Odczyt bez zamkow, poniewaz struktura jest statyczna i readonly.

**Tabela Metod Publicznych:**
- `void InitStandardPythonModules()`
  - Sygnatura: `void InitStandardPythonModules()`
  - Warunki wstepne: Maszyna wirtualna Pythona musi byc na etapie przed lub we wczesnym procesie inicjalizacji (najlepiej przed `Py_Initialize()`).
  - Skutki uboczne: Przypisuje statyczna tablice lokalna do globalnej zmiennej C-API CPythona `PyImport_FrozenModules`.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Brak specyficznych, ukrytych offsetow narazonych na hooking, jednakze `_PyImport_FrozenModules` moze stanowic wektor ataku, by "podmienic" skompilowany kod biblioteki standardowej na wlasny z poziomu cheatu/bota, przez podmiane adresow w tablicy (Arthion Hooking).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Brak powiazania. Modul niskopoziomowej inicjalizacji lokalnego Pythona.
- **Metody Pythona (`PyMethodDef`):** Modul eksponuje ponad 200 standardowych wbudowanych modulow miedzy innymi: `math`, `os`, `sys`, `socket`, `threading`, `asyncio`, `json`, `sqlite3`, `zipfile`, `email`, `html`, `http`. Brak eksportu wlasnych funkcji.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcja `InitStandardPythonModules()` MUST byc wywolana z Glownego Watku Inicjalizacji. Zmiana wskaznika globalnego Pythona przez inny watek moglaby spowodowac wyciek (race condition) lub nieprzewidywalne zachowanie podczas startu wielowatkowego, lecz zwykle odbywa sie to jednowatkowo na starcie.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** 
  - Przepelnienia buforow na starcie sa unikane, bo dane sa readonly (ROM).
  - Uszkodzenie tabeli w pamieci RAM (cheat/hook) spowoduje natychmiastowy SEGFAULT przy `import` w Pythonie.
- **Zarzadzanie zasobami (RAII):** Brak dynamicznych alokacji. Brak ryzyka wyciekow RAM. Tablice w `_PyImport_FrozenModules` sa stalymi (const) tablicami C na poziomie binarnego obrazu dyskowego (.rdata).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Ten modul (`frozen_modules.c`) jest oznaczony jako generowany automatycznie: `/* Auto-generated by Tools/freeze/generate_legacy_frozen.py */`.
  2. NIE NALEZY recznie dodawac nowych zewnetrznych tablic bajtow.
  3. Aby zamrozic nowy modul Pythona, modyfikuj narzedzie `Tools/freeze/generate_legacy_frozen.py` lub jego zrodla wejsciowe.
  4. Narzedzie wygeneruje aktualna wersje `frozen_modules.c`.
- **Jak debugowac i logowac:**
  - W razie bledu importu we wbudowanym module upewnij sie, ze rozmiar (`size`) w `_PyImport_FrozenModules` zgadza sie z faktycznym wkompilowanym `M_*` extern char array.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Wywolac mocka testowego, uzywajacego standardowego CPython C-API: napisac program C uruchamiajacy `InitStandardPythonModules()`, po czym `Py_Initialize()` a nastepnie `PyRun_SimpleString("import os; print('Success')");`. Jesli sie nie wysypie, zamrozenie zadzialalo.
