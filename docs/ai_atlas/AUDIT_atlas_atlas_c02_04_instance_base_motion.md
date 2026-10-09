---
task_id: "atlas_c02_04_instance_base_motion"
cluster: "ACT"
module_name: "CInstanceBase - Przelaczanie Animacji i Maszyna Ruchu"
target_files:
- src/UserInterface/InstanceBaseMotion.cpp
- src/UserInterface/InstanceControllers/InstanceAnimationControllerImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_04_instance_base_motion.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul jest odpowiedzialny za obsluge maszyny stanow postaci (Idle, Walk, Run, Attack, Skill, Damaged, Dead), zarzadzanie animacjami oraz ich blending (plynne przejscia miedzy klatkami). W kontekscie gry odpowiada miedzy innymi za kontrole animacji poruszania sie, czynnosci specjalnych (wedkarstwo) czy emotek, wraz z prawidlowa orientacja przestrzenna (wyliczanie rotacji do lowienia).
Architektura w kliencie rozwarstwia sie na dwie klasy-interfejsy:
* `CInstanceBase` - Glowna fasada aktora w grze, implementujaca bezposrednia integracje miedzy elementami domeny (wedkarstwo, emocje) a warstwa graficzna (silnik renderowania i sterowanie instancja poprzez `m_GraphicThingInstance`). Zwykle funkcje te wywolywane sa podczas glownej petli aktualizacji gry (Update) dla instancji modyfikowanych przez wejscie uzytkownika badz odpowiedzi sieciowe.
* `InstanceAnimationController` - Nowoczesny, zalezny od C++23, odizolowany kontroler odpowiadajacy za sterowanie stanami maszyny animacji poprzez model Event-Bus (`UserInterface::Core::EventBus`). Zapewnia to bezstanowe powiadamianie reszty systemu o zmianach animacji bez silnego sprzezenia z interfejsem graficznym.
* Cykl zycia obiektow opiera sie na ciaglym przypisywaniu jednorazowych badz zapetlonych animacji na instancji graficznej (alokacja animacji) oraz uwalnianiu/anulowaniu w przypadkach takich jak zakonczenie, zablokowanie lub restart stanu lokalnego.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Funkcje modulu powolywane sa miedzy innymi przez menedzery stanow sieci (odbior pakietow np. z informacja o stanie akcji innych graczy) oraz przez system kontroli UI (skrypty klienta pythona np. rozpoczecie wedkowania, aktywacja emocji). Klasa kontrolera animacji (`IInstanceAnimationController`) moze byc wykorzystywana przez wyzsze maszyny stanow w celu zmiany aktualnie renderowanego ruchu.
- **Zaleznosci wyjsciowe (Outbound):** Z poziomu `CInstanceBase` wychodza zaleznosci m.in do narzedzi srodowiskowych z `EterLib` (wyliczanie fizyki tla np. `__Background_GetWaterHeight`), `GameLib` (operacje sieciowe pakietow jak `PushTCPStateExpanded` lub bezposrednio fasady z silnikiem np. `CRaceMotionData`). Kontroler animacji opiera sie glownie na `EterBase::ModernLogger` (logowanie) i na `UserInterface::Core::EventBus` do publikowania eventow (`AnimationPlayedEvent`, `AnimationBlendStartedEvent`, `AnimationStateChangedEvent`).
- **Drzewo dyrektyw `#include`:** W rdzeniu `CInstanceBase` obowiazuje zaleznosc od `StdAfx.h` (podstawa prekompilacji Windowsa) oraz powiazania domeny `InstanceBase.h`, `AbstractPlayer.h`, `GameLib/ActorInstance.h`. Kontroler uzywa nowszej biblioteki `IInstanceAnimationController.h`, `EterBase/ModernLogger.h`, `UserInterface/Core/EventBus.h` uzywajac inteligentnych wskaznikow z naglowka `<memory>`. Ryzyko cyklicznych zaleznosci jest tu zniwelowane w kontrolerze przez separacje definicji zdarzen we wnetrzu pliku `.cpp`.
- **Model pamieciowy:** `CInstanceBase` uzywa bezposredniego obiektu-skladowej `m_GraphicThingInstance` do odpytywania funkcji API grafiki oraz odniesien const referencyjnych dla matematyki graficznej (np. `TPixelPosition&`). Nowoczesny kontroler instancji tworzy instancje poprzez wzorzec fabryki zwracajacy `std::unique_ptr<IInstanceAnimationController>`.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
| Nazwa | Rola | Wlasciciel Watku |
|-------|------|------------------|
| `CInstanceBase` | Glowna fasada uzytkownika i potworow | Watek Glowny/UI |
| `InstanceAnimationController` | Menedzer stanow (maszyna stanu) dla animacji | Watek Glowny/Zdarzenia |
| `MotionConfig` | Czysta konfiguracja uzywana do parametryzacji odtwarzania w C++23 | Dowolny |

**Tabela Metod Publicznych (`CInstanceBase` z `InstanceBaseMotion.cpp`):**
| Sygnatura | Wartosc zwracana | Warunki wstepne i skutki |
|-----------|------------------|--------------------------|
| `SetMotionMode(int iMotionMode)` | `void` | Ustawia ogolny tryb animacji na `m_GraphicThingInstance` |
| `PushOnceMotion(WORD wMotion, float fBlendTime, float fSpeedRatio)` | `void` | Wysyla polecenie odegrania jednej klatki. |
| `PushLoopMotion(WORD wMotion, float fBlendTime, float fSpeedRatio)` | `void` | Ustawia animacje zapetlona. |
| `StartFishing(float frot)` | `void` | Zmienia stan i rotacje, przelicza polozenie nad woda i aktywuje wezlowe zapytania we wlasnym silniku graficznym (rzut wedka) |
| `GetFishingRot(int * pirot)` | `BOOL` | Wyszukuje po promieniu `180.0f` wstepnych kordynat wody uzywajac raycastingu na tle i zapisuje kat `pirot`. |
| `ActDualEmotion(CInstanceBase & rkDstInst, WORD wM1, WORD wM2)` | `void` | Wymusza stany `WAIT` na dwoch obiektach, odczytuje wektor odleglosci i wola serwer `PushTCPStateExpanded` op-code'm `FUNC_EMOTION`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
* `InstanceAnimationController`: Posada 4 wazne stany wlasne:
  * `uint32_t m_currentMotionKey;` (Klucz animacji odwolujacy sie do np. `CRaceMotionData`)
  * `float m_speed;` (Predkosc - modyfikator)
  * `bool m_isLooping;` (Sprawdzanie stanu skoku w petle)
  * `MotionState m_currentState;` (Status zgodny z `MotionState` Enum, tj. enum C++11 class, size 1 byte)

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kody wywolan przez TCP m.in. dla systemu Emocji wywolywane z API uzywaja protokolu `FUNC_EMOTION` wraz z kluczami dla dwoch animacji - `MAKELONG(wMotionNumber1, wMotionNumber2)`. Do potwierdzania opcji wysylane jest tez identyfikacyjne pole `GetVirtualID()`.
- **Python C-API:** Brak bezposredniego bindowania w tych plikach. Wartosc wywolywana jest posrednio. Odniesienia, np. `StartFishing` lub operacje na wedkowaniu moga byc odpalane z UI w C-API (np. `player.StartFishing`).
- **Mosty C++23 EventBus:** Zdarzenia C++ wykorzystywane przez subskrybentow sieci / logiki (np. `AnimationPlayedEvent`, `AnimationBlendStartedEvent`, `AnimationStateChangedEvent`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie wywolania bezposrednie na `m_GraphicThingInstance` powinny przebiegac glownie z glownego watku D3D z powodu ryzyka bledow przy renderowaniu transformacji (np. rzut wedka wyliczajacy glebokosc Z dla renderu).
- **Potencjalne punkty awarii:** `CInstanceBase::ActDualEmotion` wymaga referencji i dostepu do lokalizacji dwoch instancji; jesli wektor pomiedzy obiema postaciami jest $0.0f$, instrukcja `kDirection / fDistance` z dzieleniem przez zero doprowadzi do bledu (NaN w wektorze). Nowoczesny kontroler prawidlowo operuje identyfikatorami zerowymi odrzucajac probki (np. rzuca `EterBase::PacketError::UnknownOpcode`).
- **Zarzadzanie zasobami:** Animacje zlecane sa przez referencje i wzorce fasady; kontroler zarzadza wylacznie informacyjna logika wywolywana w systemach (np. `EterBase::EventBus`), wiec alokacja i deallokacja RAM opiera sie o poprawnosc zewnetrznego EventBus'a.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli dodajesz nowy typ zachowania (np. stan plywania), po pierwsze zaktualizuj enum `MotionState` o nowy wpis.
  2. W pliku `InstanceAnimationControllerImpl.cpp` dodaj zaleznosc nowej stalej (np. `CRaceMotionData::NAME_SWIM`) do switch case z `DetermineState`.
  3. W klasie `CInstanceBase` dodaj fasade nowej metody w interfejsie graficznym i powiaz wysylke przez EventBus.
- **Jak debugowac i logowac:** Kontroler wspiera nowoczesne formatowanie - mozesz podgladac przelaczanie stanu subskrybujac `AnimationStateChangedEvent` lub analizujac output na konsoli narzedzia `EterBase::ModernLogger` wywolywany poziomami Info/Debug.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W przypadku unit-testow zmockuj `UserInterface::Core::EventBus` by nasluchiwac instancji zdarzen przesylanych z wnetrza `InstanceAnimationController`. Kontroler jest narzedziem calkowicie odseparowanym od kodu Windows-Specific (`D3D9`) wiec mozesz go testowac na dowolnej maszynie przez zwykla asercje wartosci powracajacej w strukturach eventowych.
