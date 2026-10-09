---
task_id: "atlas_c08_15_ime_input_system"
cluster: "UI"
module_name: "Obsluga Klawiatury IME i Schowka Tekstowego"
target_files:
- src/EterLib/IME.cpp
- src/EterLib/IME.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_15_ime_input_system.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CIME` implementuje obsluge edytora metod wprowadzania (IME - Input Method Editor) oraz schowka systemowego w kliencie gry Metin2. Zapewnia poprawna obsluge wprowadzania miedzynarodowego tekstu, w szczegolnosci znakow dla jezykow azjatyckich (CJK) oraz znakow ze znakami diakrytycznymi (np. polskie znaki przez AltGr). Obsluguje rowniez kompozycje tekstu, systemowe okna czytania znakow i listy wyboru kandydatow, co ma krytyczne znaczenie dla chatu i okienek dialogowych (UI).
Modul posiada wewnetrzne instancje `CTsfUiLessMode` dla integracji nowszego Text Services Framework (TSF) stosowanego od Windows Vista w zwyz, jak rowniez kompatybilnosc z legacy bibliotekami IMM32 (`imm.h`) poprzez hakowanie starych interfejsow.

**Flow i Cykl Zycia:**
1. **Inicjalizacja (`Initialize`):** Wywolywana na etapie startu glownego okna. Laduje biblioteke `imm32.dll` dynamicznie (`GetProcAddress`) i laduje wewnetrzne `_ImmLockIMC` / `_ImmUnlockIMC`. Konfiguruje tryby interfejsu (UiLess vs okienkowe) oraz podlacza nasluchiwanie COM (TSF Sink).
2. **Aktualizacja (`WM_*` Message Pump):** Kiedy gra odbiera komunikaty np. `WM_IME_COMPOSITION` czy `WM_CHAR`, sa one dekodowane i konwertowane na bufor wewnetrzny `m_wText`. Nastepnie odpalane sa callbacki na glownym wskazniku `ms_pEvent` typu `IIMEEventSink`. 
3. **Zamykanie (`Uninitialize`):** Odtwarza poprzedni kontekst i wylacza hooki TSF / wyrejestrowuje interfejs COM (`CoUninitialize` wywolywane z klasy pomocniczej `CDisableCicero`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** 
  - Pompa Komunikatow (Message Pump): Komunikaty takie jak `WM_INPUTLANGCHANGE`, `WM_IME_STARTCOMPOSITION`, `WM_IME_COMPOSITION`, `WM_IME_NOTIFY` i `WM_CHAR` sa bezposrednio trasowane od obslugi Win32 okna do metod klasy `CIME`.
  - Pythonowa warstwa UI (`ui.py`, `PythonApplication`): Poprzez interfejs C-API zczytuje zbuforowane znaki do aktualizacji graficznych etykiet wprowadzania tekstu.
  
- **Zaleznosci wyjsciowe (Outbound):**
  - **Win32 API:** `imm.h` (IMM32 API, `ImmGetContext`, `ImmAssociateContext`), API Schowka (`OpenClipboard`, `GetClipboardData` pod format `CF_UNICODETEXT`), User32.dll.
  - **COM / TSF:** `msctf.h` (`ITfThreadMgrEx`, `ITfUIElementSink` w instancjach wewnetrznych).
  - **EterLib/EterBase:** `EterBase/Utils.h`, tagi tekstu (`TextTag.h`).
  - **Konwersje znakowe:** `utf8.h` (`WideCharToMultiByte`).

- **Drzewo dyrektyw `#include`:**
  - `<imm.h>`, `"DIMM.h"`, `msctf.h`, `<oleauto.h>`, `<utf8.h>`.
  - Niebezpieczenstwa cyklicznych zaleznosci: brak, `CIME` jest silnie zamknietym w sobie, low-level wrapperem dla Win32.

- **Model pamieciowy:** 
  - Stan globalny (Singleton w konwencji zmiennych statycznych): `CIME::ms_wText`, `CIME::ms_undo`, `CIME::ms_redo`. Znaczna ilosc stanow wpisywania jest przechowywana na zadeklarowanych statycznie buforach `MAX_CANDLIST`, co nie pozwala na istnienie wiecej niz jednego aktywnego edytora na raz w calym procesie klienta.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

- **`IIMEEventSink` (Interfejs bazowy):**
  - Definiuje funkcje wirtualne dla zewnetrznego podlaczenia akcji UI: `OnWM_CHAR`, `OnUpdate`, `OnOpenCandidateList`, `OnCloseCandidateList`, `OnOpenReadingWnd`, `OnCloseReadingWnd`.

- **`CIME` (Glowna klasa-wrapper IME):**
  - **Wlasciciel Watku:** Glowny watek aplikacji Win32 (odrzucane sa proby aktualizacji z innych watkow ze wzgledu na bariery API systemu Windows i zaleznosc od okna HWND).
  - **`Initialize(HWND hWnd)`:** Przejmuje HWND, ustawia TSF sinks i IMM. Zwraca flage sukcesu. 
  - **`GetText(std::string & rstrText)`:** Konwertuje aktualnie edytowany tekst (std::wstring / wchar_t) na UTF-8 i zwraca do warstwy wyzszej (Python UI).
  - **`PasteTextFromClipBoard()` / `CopySelectionToClipboard(HWND)`:** Wykonuje czyste pobieranie/wstawianie stringu w UTF-16 z wykorzystaniem API `GlobalLock(hData)`.
  - **`WMComposition(...)` / `WMChar(...)`:** Funkcje Dispatching.
  
- **`CTsfUiLessMode` (Wewnetrzna klasa zarzadcy trybu TSF):**
  - **Rola:** Integruje interfejs okienek z najnowszymi standardami TSF. Rejestruje nasluchiwacze na instancji wewnetrznej typu `CUIElementSink` dziedziczacej od `ITfUIElementSink`.
  
- **`CDisableCicero`:**
  - Odpowiedzialne za bezposrednie deaktywowanie systemowego Cicero Text Service na oknie klienta w celu wymuszenia legacy IMM (dla starszych wersji SO).

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Poniewaz silnik opiera sie na zmiennych statycznych, odniesienie z pamieci przez inne moduly odbywa sie najczesciej za posrednictwem statycznego dostepu `CIME::GetText()`. Podpiecie pod bufor wprowadzania dla narzedzi reverse engineeringowych to bezposrednio `CIME::m_wText`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Modul bezposrednio nie posiada zdefiniowanych packet_id. Otrzymany tekst jest przekazywany w gore do skryptow `ui.py` ktore opakowuja go w pakiety `CG_CHAT` lub `CG_WHISPER`.
- **Python C-API:** Brak bezposredniego powiazania w tym module (nie eksportuje modulow via `PyMethodDef`). Powiazanie realizowane poprzez klase okna C++ np. `CUIWindow` co rozciaga sie na Pythona jako moduly interfejsu.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystkie `Imm*` API musza byc wolane na tym samym watku co petla komunikatow (Thread Affinity). Niedozwolone jest przelaczanie stanu CIME w watku sieciowym, doprowadzi do twardego locka z TSF.
- **Potencjalne punkty awarii (Gotchas):** 
  - Zmienne statyczne, takie jak `ms_curpos`, `ms_compLen` czy bufor historii `ms_undo` sa re-uzywane przez wszystkie proby wprowadzania. Brak prawidlowego `Clear()` pomiedzy zmiana pol tekstowych w grze sprawi ze uzytkownik skopiuje/odtworzy tekst pomiedzy zupelnie innymi okienkami (leak logiki).
  - Uzywanie niezabezpieczonych mechanizmow Win32 API przy zamykaniu okna np. zapomnienie `ImmAssociateContext` na null spowoduje memory leak uchwytu.
- **Zarzadzanie Zasobami:** Context IMM32, `ITfThreadMgrEx` z `msctf.h` to zasoby COM, musza uzywac `CoInitialize/CoUninitialize` i parowac metody `.Release()`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja modyfikacji:** 
  Aby dodac nowa metode czyszczenia znakow, nalezy poslugiwac sie funkcja `__IsWritable`. Jesli musimy dodac filtr wulgaryzmow (bad word filter), nalezaloby go dodac wyzej w hierarchii (UI lub Chat), NIE W TYM MODULE (ten modul jest transparentny na same znaki).
  Modyfikacje w kodowaniu musza zwracac uwage na `utf8::append()` i ewentualne `WideCharToMultiByte(CP_UTF8...)` (nie pomylic z CP_ACP!).
- **Jak debugowac i logowac:** 
  W wypadku problemow ze zgodnoscia IME, przelacz systemowe opcje jezyka miedzy "Tryb zgodnosci dla starszych programow" i podepnij punkt przerwania na `CTsfUiLessMode::SetupSinks()`. Analiza logow jest uboga, poniewaz wywolanie petli komunikatow moze byc rzedu 1000/s podczas przesuwania myszki (nalezy warunkowac log na `WM_IME_COMPOSITION`).
- **Jak testowac Headless:**
  Srodowiska Linuxowe beda calkowicie wycinac `CIME` albo zastepowac go no-op mockami ze wzgledu na Windowsowe `imm.h`. W mockowanym tescie nastepuje podstawienie HWND jako nullptr. Nalezy bezposrednio wywolywac statyczne obslugi komunikatow `WMChar` rzutujac wartosci testowe z `WPARAM`.

