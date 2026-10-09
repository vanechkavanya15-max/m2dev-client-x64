---
task_id: "atlas_c07_08_effect_manager"
cluster: "MOD"
module_name: "CEffectManager - Glowny Menedzer Efektow Specjalnych (.mse)"
target_files:
- src/EffectLib/EffectManager.cpp
- src/EffectLib/EffectManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_08_effect_manager.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport AI Atlas - CEffectManager

## Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
`CEffectManager` jest centralnym Singletonem w kliencie Metin2 zarzadzajacym ladowaniem, tworzeniem oraz renderowaniem efektow specjalnych (wizualnych i dzwiekowych), ktore sa zdefiniowane w plikach skryptowych `.mse`.
Obsluguje on calkowity cykl zycia efektow - od alokacji definicji danych efektu (`CEffectData`), tworzenia aktywnych instancji efektow (`CEffectInstance`), po ich usuniecie, po tym jak zakonczy sie ich odtwarzanie.

Modul ten dziala zintegrowany scisle z glowna petla gry:
1. **OnUpdate:** Metoda `Update()` jest wywolywana, iterujac przez aktywne efekty w mapie `m_kEftInstMap`. Aktualizuje klatke dla kazdego z nich i sprawdza czy obiekt nadal "zyje" metoda `isAlive()`. Jezeli efekt wygasl, jest on bezpiecznie zwalniany z pamieci.
2. **OnRender:** Metoda `Render()` naklada tekstury i wykonuje wyrysowanie dla kazdego aktywnego efektu, sortujac najpierw powloki przy uzyciu `CEffectManager_LessEffectInstancePtrRenderOrder`, by zachowac prawidlowe dzialanie przezroczystosci uzywajac algorytmu malarza (Painter's Algorithm).

Architektura wykazuje silne oparcie na globalnym stanie (State Machine pattern) na rzecz edycji - wybiera sie konkretna instancje (`SelectEffectInstance`), a dopiero potem uzywa np. `SetEffectInstanceGlobalMatrix`, ktora modyfikuje stan aktualnie wybranego obiektu (`m_pSelectedEffectInstance`).

## Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Modul jest silnie zalezny od menedzerow UI i obiektow na mapie (np. `CPythonCharacterManager`, skille), ktore triggeruja tworzenie efektow w przestrzeni 3D za pomoca kluczy identyfikatorow tworzonych przez skroty CRC32 sciezki pliku.
- **Zaleznosci wyjsciowe (Outbound):** Zaleznosci na `STATEMANAGER` (z EterLib) w celach zarzadzania stanami renderowania Direct3D 9, `CEffectData` jako model dla parsowanych plikow MSE i `CEffectInstance` ktory realnie uderza w zasoby GPU. Zastosowano biblioteke D3DX (`D3DXMATRIX`, `D3DXVECTOR3`) do pozycjonowania 3D i obrotow.
- **Drzewo dyrektyw `#include`:** W pliku C++ mamy dolaczane `StdAfx.h` i `EffectInstance.h` dla definicji logiki instancji efektow.  
- **Model pamieciowy:** Calosc bazuje na surowych wskaznikach (`CEffectData*`, `CEffectInstance*`) przechowujacych zasoby, a alokacja zarzadzana jest poza new/delete przy uzyciu wyspecjalizowanych pooli np. wywolan `CEffectInstance::New()` i `CEffectInstance::Delete()`. Glowna indeksacja do identyfikacji odbywa sie przez wartosciowanie skrotami `DWORD` (CRC32).

## Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|-------|------|------------------|
| `CEffectManager` | Singleton narzucajacy cykl zycia, update, render oraz cache dla .mse. | Watek Glowny (Render/Logic) |
| `EEffectType` | Enum typow efektu m.in. PARTICLE, ANIMATION_TEXTURE, MESH, SIMPLE_LIGHT. | N/A |

### Tabela Metod Publicznych (wybrane)
| Sygnatura | Wartosc Zwracana | Warunki Wstepne & Skutki Uboczne |
|-----------|------------------|----------------------------------|
| `RegisterEffect(const char* c_szFileName, bool isExistDelete, bool isNeedCache)` | `BOOL` | Tworzy hash CRC32 i rejestruje w `m_kEftDataMap`. |
| `CreateEffect(DWORD dwID, const D3DXVECTOR3& pos, const D3DXVECTOR3& rot)` | `int` (Index) | Tworzy na mapie nowa instancje CEffectInstance. Uzywa globalnego zablokowania selectora. |
| `Update()` | `void` | Usuwa efekty kiedy `pEffectInstance->isAlive()` zwroci `false`. |
| `Render()` | `void` | Opcjonalnie sortuje i renderuje aktywne wezly. Wywoluje modyfikacje stanow wewnetrznych renderowania. |
| `SelectEffectInstance(DWORD dwInstanceIndex)` | `BOOL` | Ustawia pole `m_pSelectedEffectInstance` (Stateful manipulation). |
| `CreateUnsafeEffectInstance(DWORD, CEffectInstance**)` | `void` | Zwraca nowy wskaznik niezaleznie od zarzadcy, stosowane w CMapOutdoor. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `bool m_isDisableSortRendering;` 
- `TEffectDataMap m_kEftDataMap;` -> Przechowuje wzorce wytypowane z plikow skryptowych.
- `TEffectInstanceMap m_kEftInstMap;` -> Aktywne efekty kontrolowane przez klienta.
- `TEffectInstanceMap m_kEftCacheMap;` -> Zapamietane instancje.
- `CEffectInstance * m_pSelectedEffectInstance;` -> Slaby wskaznik reprezentujacy maszyne stanow w edycji obiektu.

## Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** `CEffectManager` bezposrednio nie parsuje ruchu sieciowego. Dziala na nim warstwa wierzchnia, jak obsluga pakietow bitewnych serwera (`TPacketGCCharacterAdditionalInfo` lub `TPacketGCDamageInfo`), ktora decyduje jakie CRC przekazac menedzerowi zadan.
- **Metody Pythona:** Most z Pythonem odbywa sie za pomoca wywolan np. z `app` badz `chr`, gdzie funkcje takie jak `app.RegisterEffect` beda uzywac bezposrednio menedzera aby zaladowac wstepnie bufory `.mse` przed wyrenderowaniem postaci/npc.

## Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Funkcje alokacji i zwalniania jak i `Render()` musza bezwzglednie dzialac w watku renderowania (Main Thread). Rozne mechanizmy tworzenia (z mapy i Unsafe) uniemozliwiaja bezproblemowa wielowatkowosc bez refaktoryzacji muteksami.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Uzaleznienie manipulacji od `m_pSelectedEffectInstance`. Wywolanie `SetEffectInstanceGlobalMatrix` z pominieciem, lub wplywem hookow pobocznych na wywolanie `SelectEffectInstance` grozi nieobslugiwalnym cichym nadpisywaniem pamieci innego aktywnego efektu w razie przelaczenia kontekstu sterowania. 
  - Przekroczenie `iMaxIndex` z random index powoduje reset w liczbie pow. 2.1 mld i zapobieganie kolizjom wyszukiwaniem inkrementalnym - niesamowicie kosztowne operacyjnie, jezeli index bylby mocno wypelniony.
- **Zarzadzanie zasobami (RAII):** Kod zarzadza recznie operacjami pool. W kazdym przypadku manualnej alokacji przez `CreateUnsafeEffectInstance`, programista bezwzglednie ma zadanie samodzielnego uwolnienia poprzez `DestroyUnsafeEffectInstance`. Inaczej wystapi pewny memory leak na VRAM.

## Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jezeli chcesz dodac nowa metode modyfikacji np. `SetEffectAlpha(float)`, musisz dodac takowa wpierw w `CEffectInstance`.
  2. W `CEffectManager` dodaj publiczna metode `void SetEffectAlpha(float alpha)`.
  3. W metodzie zastosuj sprawdzian wstepny `if (!m_pSelectedEffectInstance) return;`.
  4. Nastepnie przekieruj wywolanie: `m_pSelectedEffectInstance->SetAlpha(alpha);`.
- **Jak debugowac i logowac:**
  Do debugowania sluzy API biblioteki modulu logiki (`TraceError`, `Tracef`). Szczegolnie kluczowe logi to nieodnalezienie wpisow na mapach, podczas prob manipulacji id ktorym menedzer juz nie zarzadza (juz je zwolnil w Update).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** 
  Do przeprowadzenia testow bez GPU (`doctest` na Linuxie), trzeba uzyc flagi `-DTEST_MOCK_D3D9`, wstrzyknac puste D3DMATRIX do naglowkow z `include_dummy` badz wylaczyc w makrach odwolania na metody Direct3D podczas budowania makr. Konieczne jest mockowanie `CEffectData` przez proste stubs.
