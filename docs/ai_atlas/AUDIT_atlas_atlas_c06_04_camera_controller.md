---
task_id: "atlas_c06_04_camera_controller"
cluster: "RND"
module_name: "CCamera - Kamera Trzecioosobowa i Matematyka Widoku"
target_files:
- src/EterLib/Camera.cpp
- src/EterLib/Camera.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_04_camera_controller.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# Raport Analityczny modulu CCamera (Metin2 Client)

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CCamera` oraz zarzadzajacy nim `CCameraManager` stanowia glowne jadro logiki kamery trzecioosobowej w silniku renderujacym klienta (EterLib). Podstawowym celem biznesowym jest dostarczenie graczowi plynnego, stabilnego i wolnego od bledow matematycznych widoku na swiat gry. Modul ten:
- Oblicza i utrzymuje macierze widoku (View Matrix - `m_matView`) oraz odwrotne macierze widoku (`m_matInverseView`) i macierze billboardowania (`m_matBillboard`) dla potoku renderowania Direct3D 9.
- Wykonuje matematyke rzutowania i transformacji wektorowych, konwertujac input gracza (Pitch, Roll z myszy) na prawidlowe wektory Eye (kamera), Target (postac/fokus) oraz Up, Cross i View.
- Generuje promienie (Rays) wykorzystywane pozniej przez inne systemy (np. Terrain, Area, fizyka) do detekcji kolizji z terenem, budynkami (Screen Building) oraz obiektami (Object Collision Rays).
- Posiada system stanow (`eCameraState`), np. blokowanie kamery przed wejsciem pod ziemie (`CAMERA_STATE_CANTGODOWN`) lub przed wejsciem w obiekt (`CAMERA_STATE_CANTGOLEFT`).

**Wywolanie w petli gry:** 
Kamera aktualizowana jest co klatke (OnUpdate/OnRender). Klasa `CCamera` przetrzymuje swoj stan lokalnie, a `CCameraManager` jako Singleton (dziedziczacy po `CSingleton<CCameraManager>`) zapewnia dostep do aktywnej kamery z kazdego miejsca glownego watku EterLib (Render/D3D Thread). 

**Przeplyw danych (Data Flow) i Cykl Zycia:**
1. Alokacja nastepuje w konstruktorze `CCameraManager`, gdzie automatycznie tworzone sa `DEFAULT_PERSPECTIVE_CAMERA` oraz `DEFAULT_ORTHO_CAMERA` trzymane w `std::map<BYTE, CCamera*>`.
2. Otrzymuje komendy wejsciowe (input) poprzez metody typu `Drag()`, `Wheel()`, ktore przeliczaja ruch myszy na predkosc katowa (`m_v3AngularVelocity`), aplikujac opor (Resistance).
3. Przed samym renderowaniem wywolywana jest metoda `SetViewMatrix()`, ktora odswieza wszystkie powiazane macierze w oparciu o wektory Eye, Target, Up. Wykonuje tam wazna obrone przed NaN i dzieleniem przez zero (`FLT_EPSILON`).
4. Wynik (np. `GetViewMatrix()`) pobierany jest przez Dispatchery Renderowania, CStateManager oraz jednostki sortujace (SortKeyBuilder).
5. Dealokacja (kasowanie wzkaznikow) nastepuje w destruktorze `CCameraManager`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywany przez klase `CApplication` (glowna petla aplikacji), `CPythonApplication` (inicjalizacja grafiki), `CActorInstance` (centrowanie kamery na glownym bohaterze).
  - Skrypty Pythona (przez C-API wrappery w modulach systemowych) wplywajace posrednio na limity zoomu lub restrykcje wysokosci.
- **Zaleznosci wyjsciowe (Outbound):**
  - **DirectX 9 Math (D3DX):** Potezne poleganie na funkcjach z `<d3dx9math.h>`: `D3DXVec3Cross`, `D3DXVec3Normalize`, `D3DXMatrixLookAtRH`, `D3DXMatrixInverse`.
  - **EterBase/Singleton.h:** Abstrakcja Singletonu dla `CCameraManager`.
  - **EterBase/Utils.h:** Makra matematyczne i narzedzia (`fMAX`, `fMIN`).
  - **Ray.h:** Definicja prostej linii (poczatek, kierunek) sluzacej do testowania kolizji z modelem/terenem.
- **Drzewo dyrektyw `#include`:**
  - `Camera.h` laduje `<map>`, `"EterBase/Singleton.h"`, `"Ray.h"`.
  - `Camera.cpp` laduje `"StdAfx.h"`, `"EterBase/Utils.h"`, `"Camera.h"`, `<cmath>`. (Brak groznych zaleznosci cyklicznych, kod C++ pre-C++11 z modernizacjami).
- **Model pamieciowy:** 
  - Glownie obiekty zarzadzane starymi wskazinikami raw (`CCamera *`) i przechowywane w `std::map`. Z uwagi na to, ze instancje zyja caly cykl programu w obiekcie singletonu, wycieki sa marginalne.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
| Nazwa | Rola | Wlasciciel Watku |
|-------|------|-------------------|
| `CCamera` | Agregat stanu (macierze widoku, katy Pitch/Roll, pozycja, zasieg rzutowania kolizji). | Glowny Watek (D3D) |
| `CCameraManager` | Singleton rejestrujacy i przelaczajacy referencje do instancji `CCamera`. | Glowny Watek (D3D) |
| `eCameraState` | Enum opisujacy dozwolone kierunki ruchu po kolizji (np. `CAMERA_STATE_CANTGODOWN`). | N/A |

**Tabela Metod Publicznych**
| Sygnatura | Wartosc Zwracana | Skutki Uboczne / Uwagi |
|-----------|-------------------|-------------------------|
| `void SetViewMatrix()` | `void` | Nadpisuje `m_v3View`, `m_matView`, `m_matInverseView`, `m_ViewRay`, itd. Chroni przed NaN. |
| `void SetViewParams(const D3DXVECTOR3&, const D3DXVECTOR3&, const D3DXVECTOR3&)` | `void` | Zmienia Eye, Target, Up i odswieza macierze. |
| `void RotateEyeAroundTarget(float, float)` | `void` | Obraca Eye wokol Target uzywajac D3DXMatrixRotationZ, aktualizuje m_fRoll, zmienia m_v3Eye. |
| `bool Drag(int, int, LPPOINT)` | `bool` | Pobiera i aktualizuje predkosc katowa X/Z; skutek uboczny: modyfikuje m_v3AngularVelocity. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
Kluczowe wektory wewnatrz `CCamera`:
- `m_v3Eye` (`D3DXVECTOR3`): Reprezentuje X,Y,Z swiata gdzie "wisi" kamera.
- `m_v3Target` (`D3DXVECTOR3`): Reprezentuje X,Y,Z swiata na co patrzy kamera.
- `m_matView` (`D3DXMATRIX` - 64 bajty): Wyliczona macierz widoku uzywana dla shadera jako `View`.
- Promienie (`CRay` m_kCameraBottomToTerrainRay itp.): Prealokowane wektory dla fizyki. 
- *Hooking Notes*: Do zrobienia drona latajacego/bota obserwatora wstrzyknac mozna sie pod instrukcje modyfikujace `m_v3Eye` lub zamrozic blokade `Lock()` / `m_isLock`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Modul `CCamera` dziala scisle po stronie logiki klienckiej (Client-side) w architekturze prezentacyjnej.
- **Pakiety Sieciowe:** Brak bezposredniego mapowania na pakiety GC czy CG. Ruch kamery nie jest weryfikowany przez serwer Metin2; serwer obchodzi jedynie koordynaty (X,Y) i kierunek gracza (`CActorInstance`).
- **Python C-API:** Brak bezposredniego eksportu metod typu `PyMethodDef` bezposrednio w `Camera.cpp`. Istnieja zaleznosci posrednie w `CPythonApplication` udostepniajace Pythonowi narzedzia takie jak `app.SetCameraMaxDistance()` ktora wola `CCamera::SetCameraMaxDistance(float)`.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

1. **Zasady wielowatkowosci:** Funkcje musza byc wywolywane z glownego watku D3D. Zmiana macierzy widoku podczas renderowania innych obiektow na innym watku spowoduje niespojnosc klatki (race condition).
2. **Crash Points (NaN Safety):** Kiedy `m_v3Target` pokrywa sie dokladnie z `m_v3Eye`, `m_v3View` ma dlugosc 0. Funkcja `SetViewMatrix` ma wdrozone zabezpieczenia (`std::isnan` oraz test `> FLT_EPSILON` dla `D3DXVec3Length`). Nie wolno usuwac tych if'ow, inaczej `D3DXVec3Normalize` wygeneruje float NaN, i `D3DXMatrixDeterminant` zarazi macierz inwersji i zepsuje potok renderowania (Czarny / Bialy Ekran Otwchlani).
3. **Funkcje Trygonometryczne:** `acosf` wymaga aby wartosc `.dot()` pomiedzy znormalizowanymi wektorami nie przekraczala przedzialu [-1, 1]. Kod zawiera explicit clamping `if (m_fPitch >= 1.0f) m_fPitch = 1.0f;` zapobiegajacy crashom CPU FPU.
4. **Zarzadzanie zasobami (RAII):** Kod operuje na zywych wskaznikach w slowniku (std::map `m_CameraMap`). Podczas usuwania pamieci destruktor `CCameraManager` uzywa golego `delete`. Nalezy w przyszlosci przepisac na `std::unique_ptr<CCamera>` w celu poprawy higieny zasobow, ale aktualnie trzeba dbac o wywolywanie RemoveCamera jesli dynamicznie dodaje sie instancje.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja dodawania nowej funkcji (np. Free Camera / Debug Cam):**
1. W `CCamera.h` dodaj nowa wartosc `ECameraNum`, np. `DEBUG_FLY_CAMERA`.
2. W miejscu, w ktorym zarzadasz inputem (np. nacisniecie klawisza 'F3'), wywolaj `CCameraManager::Instance().SetCurrentCamera(DEBUG_FLY_CAMERA)`.
3. Skonfiguruj te nowa kamere wywolujac jej `Unlock()` oraz ignorujac zasady `m_fPitch + fPitchDegree > 80.0f` dla pelnych fikolkow myszka.

**Jak debugowac i logowac:**
Z uwagi na to, ze funkcja `Update` i manipulacje `Move` wywolywane sa setki razy na sekunde (TickRate FPS), uzywanie `LogBox` lub printf zatka I/O.
Do debugu uzywaj rzutowania na ekran: przekaz `m_v3Eye` i `m_fPitch` / `m_fRoll` do zewnetrznego UI. Szukaj zmian we flagach stanow kamery (`m_eCameraState` na `CAMERA_STATE_CANTGODOWN` itp.).

**Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
Aby przetestowac rzuty katow lub konwersje matryc (`SetViewMatrix`):
1. Zbuduj maly izolator (`#define TEST_MODE_DISABLE_STDAFX` zgodny z notatkami).
2. Dolacz mocki dla `D3DXVECTOR3` i `D3DXMATRIX` (zgodnie z `DirectXMath` lub malym shimem D3DX).
3. Skompiluj test narzedziem `doctest` tworzac instancje `CCamera`, wywolujac `SetEye`, `SetTarget` i asercje dla `.GetPitch()` bez potrzeby startowania calej gry ani serwera X11.
