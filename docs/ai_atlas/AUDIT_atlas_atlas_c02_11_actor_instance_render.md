---
task_id: "atlas_c02_11_actor_instance_render"
cluster: "ACT"
module_name: "CActorInstance - Renderowanie Siatek i Poziomy Szczegolow (LOD)"
target_files:
- src/GameLib/ActorInstanceRender.cpp
- src/UserInterface/InstanceControllers/InstanceRendererImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_11_actor_instance_render.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul `CActorInstance` (a scislej jego czesc odpowiedzialna za renderowanie zawarta m.in. w `ActorInstanceRender.cpp`) jest odpowiedzialny za wizualna reprezentacje aktorow (graczy, potworow, NPC, wierzchowcow) w swiecie gry Metin2. Odpowiada za wysylanie do karty graficznej instrukcji rysowania siatki geometrycznej 3D, z uwzglednieniem roznych trybow mieszania (alpha blending), materialow (diffuse, opacity), czy kolorow zaleznosci. 

W cyklu zycia (petli gry) kod ten jest glownie wywolywany w fazie "OnRender". Gdy silnik zglasza zapotrzebowanie na narysowanie klatki (Frame), kazda widoczna instancja aktora przechodzi przez swoja funkcje `OnRender`.

**Cykl zycia obiektow (Lifecycle):**
Alokacja instancji aktora i zasobow (jak materialy) zwykle nastepuje w momencie pojawienia sie podmiotu w zasiegu widzenia.
Funkcje z rodziny `Render...` zmieniaja lokalnie stany w pipeline renderowania DirectX i po wykonaniu draw callow, przywracaja je (`RestoreRenderState`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

- **Zaleznosci wejsciowe (Inbound):** Kod z rodziny "Render" z `CActorInstance` jest wolany przez glownego zarzadce sceny / entity manager (np. `CCharacterManager::Render` / render pipeline), gdy kamera gracza nakazuje narysowanie obiektow w polu widzenia.
- **Zaleznosci wyjsciowe (Outbound):** 
    - `EterLib::StateManager` (STATEMANAGER) do manipulowania stanami Direct3D 9 (RenderState, TextureStageState).
    - `CScreen` (do renderowania wektorow pomocniczych, np. kolizji).
    - Modele/Siatki Granny 3D (obsluga blendowania, diffuse).
- **Drzewo dyrektyw `#include`:** 
    - `StdAfx.h` - podstawowy prekompilowany naglowek projektu, zapewniajacy podstawowe struktury i funkcje API Windows.
    - `EterLib/StateManager.h` - Singleton kontrolujacy stany DirectX.
    - `ActorInstance.h` - Glowne naglowki z deklaracja samej klasy.
- **Model pamieciowy:** W klasie powszechnie uzywane sa "czyste" wskazniki (raw pointers), jak `m_pkHorse`, `m_pkCurRaceData`, `m_pAttributeInstance`. Wskazuje to na reczne zarzadzanie pamiecia blokowa i mozliwe ryzyka uzycia po zwolnieniu (use-after-free).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CActorInstance`: Glowna klasa aktora. Dziedziczy po `CGraphicThingInstance` badz pokrewnych klasach rdzenia. Zarzadzana zazwyczaj na Glownym Watku (Main Thread).
- `CScreen`: Uzywana sporadycznie (czesto definiowana statycznie `static CScreen s_kScreen`) do renderowania prostych elementow takich jak linie kierunkowe (`RenderLine3d`) i sfery kolizji (`RenderCircle3d`).

**Tabela Metod Publicznych (`CActorInstance` z renderowania):**
- `OnRender()`: Rysuje aktora uwzgledniajac przypisany `m_iRenderMode` (NORMAL, BLEND, ADD, MODULATE). Efektem ubocznym sa masowe zmiany w `STATEMANAGER`. Zwraca `void`.
- `SetMaterialColor(DWORD dwColor)`: Ustawia kolor materialu (`m_dwMtrlColor`) i propaguje ten sam kolor dla wierzchowca (`m_pkHorse`).
- `BeginDiffuseRender()` / `EndDiffuseRender()`: Zmienia TextureStageStates na operacje `D3DTOP_MODULATE` miedzy TEXTURE a DIFFUSE.
- `BeginOpacityRender()` / `EndOpacityRender()`: Konfiguruje D3DRS_ALPHATESTENABLE dla kanalow przezroczystosci.
- `BeginBlendRender()` / `EndBlendRender()`: Ustawia `D3DRS_ALPHABLENDENABLE` i typy blendowania na SRCALPHA i INVSRCALPHA.
- `SetRenderMode(int iRenderMode)`: Przelacza tryb i zapisuje go w `m_kBlendAlpha.m_iOldRenderMode` jesli akurat nastepuje blending.
- `RenderCollisionData()`: Renderuje strefy kolizji ciala postaci (atak, obrona, body) w celach analitycznych/debugowania.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

- **Pakiety Sieciowe:** Kwestie czystego renderowania zazwyczaj nie wysylaja, ani bezposrednio nie pakuja danych CG/GC. Reaguja one natomiast na modyfikacje z sieci - zmiany ekwipunku, poziomu zdrowia czy polozenia powoduja odswiezenie flag i trybow renderowania (np. postac, ktora zostala zaatakowana przez potwora i otrzymala "trucizne" moze wymusic ustawienie odpowiedniego `AddColor` lub efektu wizualnego).
- **Metody Pythona:** Istnieje potencjal powiazania, gdzie instancja aktora moze zostac modyfikowana poprzez chr.SetBlendRenderMode (teoretyczna nazwa metody pythona uzywajacej instancji `CActorInstance`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Wszystkie wywolania `OnRender()`, zmiany state'ow DirectX (`STATEMANAGER`) musza bezwzglednie wykonywac sie na WATKU GLOWNYM (Main Thread), ktory utrzymuje Device Direct3D. Zmiana ich z innego watku to natychmiastowy Crash z winy drivera.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
    - Wykonanie `OnRender` gdy `m_pkCurRaceData` jest NULLem (obecnie zabezpieczone: `if (!m_pkCurRaceData) return;`).
    - Niewlasciwe zapisanie lub pominiecie odtworzenia stanow DirectX (np. zapomnienie `STATEMANAGER.RestoreRenderState`), co powoduje tzw. "wylewki stanow na reszte UI/mapy" (grafika wariuje, czernieje ekran). Koniecznie uzywaj klas RAII typu `RenderStateGuard` dla nowego kodu.
    - Uzywanie niezainicjalizowanych wskaznikow na wierzchowce (`m_pkHorse`).
- **Zarzadzanie zasobami (RAII):** Kod legacy opiera sie na zaglebionych wywolaniach `Begin...` i `End...`, w ktorych brak wyjatkow. Dla wspolczesnych ulepszen zalecane jest opakowywanie modyfikacji stanow przez RAII zeby wyjatki lub `early return` nie zatruwaly stetow urzadzenia graficznego.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
    1. Aby dodac nowy parametr wizualny, dodaj pole w deklaracji `CActorInstance` (w pliku `ActorInstance.h`).
    2. Zainicjalizuj go w konstruktorze `ActorInstance.cpp` / `Initialize`.
    3. W `ActorInstanceRender.cpp` wejdz w cykl `OnRender()` lub dedykowanej funkcji `Begin/End` i wstaw swoj mechanizm (np. unikalny tryb specular). 
    4. Pamietaj o dodaniu `SaveRenderState` przed i `RestoreRenderState` po, ewentualnie uzyj nowych mechanizmow RAII.
- **Jak debugowac i logowac:**
    Jesli chcesz zobaczyc obwiednie i szkielet kolizyjny aktora, mozesz prawdopodobnie wlaczyc / uzyc `RenderCollisionData()`. W logice wizualnej latwo dodac `ms_isDirLine = true` poprzez `ShowDirectionLine(true)` dla debugowania katow widzenia potworow/postaci.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
    Testy w C++ musza obslugiwac sztuczny `STATEMANAGER`. Nalezy w naglowku testu pominac inclusion realnego DirectX (np. `#define __CSTATEMANAGER_H`), wymockowac `EterLib::StateManager` oraz zwiazane typy, a nastepnie recznie zainicjalizowac puste makiety (mocki) Device D3D, wywolujac metody wizualne na dummy obiekcie `CActorInstance`.


**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Poniewaz nie badamy pelnego naglowka, dokladne offsety nie sa tu podane w 100%, natomiast kluczowe pola dla wstrzykiwania kodu to:
- `m_iRenderMode`: Flaga decydujaca o sciezce rysowania, czesto powiazana bezposrednio ze sciezka wykonania.
- `m_fAlphaValue`: Zmiennoprzecinkowa wartosc (0.0 do 1.0) decydujaca o przezroczystosci. Wymagane przy efektach pojawiania sie (fade in/out).
- `m_AddColor`: Struktura D3DXCOLOR (lub podobna) modyfikujaca stany np. dla trucizny czy zaklec.
