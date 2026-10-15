---
task_id: "atlas_c04_09_fly_manager_traces"
cluster: "CBT"
module_name: "Menedzer Smug i Trajektorii Broni (Fly Trace & Weapon Trail)"
target_files:
- src/GameLib/FlyTrace.cpp
- src/GameLib/ActorInstanceWeaponTrace.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_09_fly_manager_traces.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")

Modul "Fly Trace & Weapon Trail" w kliencie Metin2 odpowiada za generowanie wizualnych smug swiatla za szybko poruszajacymi sie obiektami 3D (tzw. motion trails). System ten jest wykorzystywany w dwoch glownych scenariuszach:
1. Smugi pozostawiane za lotem pociskow, strzal i efektow magicznych (`CFlyTrace`).
2. Efekty ciec i uderzen bronia zreczna (`CWeaponTrace` zintegrowane w `CActorInstance`), generujace wstege na koncu ostrza.

Architektura opiera sie na zapisywaniu historycznych pozycji obiektu w kolejce (np. `std::deque` z danymi czas-pozycja). Na podstawie tych archiwalnych punktow i czasu zycia, w fazie OnRender budowany jest pasek wielokatow (D3DPT_TRIANGLESTRIP). Modul modyfikuje stany mieszania alfa w `STATEMANAGER` aby osiagnac efekt plomiennego znikajacego ogona (Fade-Out) lub plynnego smuzenia przez transparentnosc (Additive Blending).

Cykl zycia:
- Alokacja i inicjalizacja zachodzi bardzo czesto, wiec obiekty (`CFlyTrace`, `CWeaponTrace`) korzystaja z customowej puli prealokacyjnej pamicci w postaci `CDynamicPool`.
- Obiekt rejestruje swoje wspolrzedne swiata co klatke podczas funkcji `Update()` i przechowuje je zaleznie od timera (EterLib/Timer `CTimer::Instance().GetCurrentSecond()`). Wektory starsze niz dopuszczalny prog (np. m_fTailLength) sa usuwane w czasie `UpdateNewPosition()`.
- W czasie zakonczenia trajektorii obiekty wracaja do puli poprzez statyczne metody `Delete()`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

Zaleznosci Wejsciowe (Inbound):
- `CActorInstance` aktualizuje wspolrzedne broni za pomoca `TraceProcess()`. Wola rowniez odpowiednio `__ShowWeaponTrace`, `__HideWeaponTrace` na zdarzeniach ataku w kliencie.
- System latajacych obiektow (Pociski/Strzaly), np. `CFlyingInstance` wezwie `CFlyTrace::UpdateNewPosition()` a nastepnie wyrenderuje je przez delegata.

Zaleznosci Wyjsciowe (Outbound):
- Czas rzeczywisty klienta: EterLib / `CTimer`.
- Zarzadca kamery i macierze rzutowania 3D: EterLib / `CCamera`, `CScreen`.
- Zarzadca stanow graficznych silnika DirectX 9: EterLib / `STATEMANAGER`.
- Abstrakcje operacji matematycznych Direct3D: `D3DXVECTOR3`, `D3DXMATRIX`.
- Alokacja pamieci: struktury zdefiniowane przez `CDynamicPool<T>`.

Drzewo dyrektyw `#include`:
- `FlyTrace.cpp`: `stdafx.h`, `EterLib/StateManager.h`, `EterLib/Camera.h`, `FlyingData.h`, `FlyTrace.h`.
- Brak groznych cyklicznych zaleznosci miedzy systemem renderowania a aktorem; jednak `ActorInstanceWeaponTrace.cpp` musi znac definicje `ActorInstance.h` oraz `WeaponTrace.h`.

Model Pamieciowy:
- Modul silnie unika alokacji na stercie wewnatrz petli aktualizacji; `TFlyVertexSet` przemieszcza wektory i tablice alokowane na stosie przed wywolaniem `DrawPrimitiveUP` z wierzcholkami uzywanymi do renderowania.
- Kontrola cyklu zycia dziala wylacznie w oparciu o manualne zarzadzanie pamiecia (czyste wskazniki `CWeaponTrace *`, `CFlyTrace *`) pobierane i czyszczone za pomoca pool-allocatora `ms_kPool`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur**
| Nazwa | Rola | Wielkosc | Wlasciciel Watku |
|---|---|---|---|
| `CFlyTrace` | Klasa zarzadzajaca smuga od pociskow. Oblicza bilbordy z krzywych i zarzadza renderingiem przez TriangleStrip. | Ok. 100 bajtow. | Watek Glowny Renderingu |
| `CWeaponTrace` | Klasa zarzadzajaca smuga wygenerowana ruchem broni bialej (miecz/wlocznia). | Ok. 150 bajtow. | Watek Glowny Renderingu |
| `TFlyVertex` | Struktura opakowujaca wierzcholek trajektorii `[Pozycja (12b), Kolor (4b), UV (8b)]` gotowy dla DirectX. | 24 bajty | Stos / Render Pipeline |
| `TFlyVertexSet` | Zbior (zazwyczaj 6) pre-kalkulowanych wierzcholkow tworzacych segment smugi gotowy do narysowania jako trojkat. | 144 bajty | Stos |

**Tabela Metod Publicznych (`CFlyTrace`)**
| Sygnatura | Opis / Skutki |
|---|---|
| `void Create(const CFlyingData::TFlyingAttachData &)` | Inicializuje grubosc, kolor i limit czasowy smugi w oparciu o template. |
| `void UpdateNewPosition(const D3DXVECTOR3 &)` | Zapisuje obecny wektor przemieszczenia i czas w kolejce `std::deque`. Czysci zdezaktualizowane segmenty. |
| `void Render()` | Transformuje historyczne pozycje wzgledem kamery w obiekty prostopadle uzywajac cross produkts. Wolania D3D (AlphaBlend). Skutek uboczny: modyfikuje transformacje w `STATEMANAGER`. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
- `CFlyTrace`: Pierwsze pule dziedzicza z `CScreen`. Trzyma flage prostokatnego wektora `bool m_bRectShape`, kolor smugi `DWORD m_dwColor`, i dwie wielkosci flotujace `float m_fSize` oraz `float m_fTailLength`. Potem struktura drzewiasta (deque) trzymajaca zrodla offsetow. Dla FFI latwo nadpisac wektory uzywajac prostego modyfikowania `m_dwColor`.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

Pakiety Sieciowe (Network Packets):
Brak bezposrednich powiazan z ruchem sieciowym. Ten podsystem dziala zupelnie ofline na stacji roboczej klienta. Pozycje strzaly generuje interpolacja punktow ze stacji A do stacji B - o pakietach wie inna wyzsza warstwa, zrzucajac polecenie animacji do GameLib. Zmiany sieciowe widoczne przez ping i teleportacje natychmiast wygaszaja lub zaklocaja ciaglosc smugi poprzez znaczne wahania czasowe i przerwane Update'y.

Metody Pythona:
- UI uruchamiane ze skryptow zlecaja uderzenia (`player.SetAttackKeyState()`). Skrypty Pythona NIE maja mozliwosci ingerowania pojedynczymi poleceniami w zarzadzanie smuga (Trace) per wierzcholek. Odsylane w `CPythonPlayer` zadania wyzwalaja dzialanie `CActorInstance`. Modele `chrmgr` pozwalaja jedynie wywolac odpowiednie animacje (.msa) ktore wyzwalaja sygnal wewnetrznego eventu motion `__ShowWeaponTrace()` podczas zdefiniowanej klatki ciachniecia mieczem.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Wielowatkowosc (Thread Safety):** Obiekty te zyja calkowicie i bez wyjatkow w watku glownym aplikacji ze wzgledu na korzystanie z globalnego dostepu do stanu Direct3D w `Render()`. `STATEMANAGER` nie jest w zadnym stopniu przystosowany do dzialania wspolbieznego.
- **Wycieki Pamieci i RAII:** Instancje alokowane z `ms_kPool` musza byc absolutnie usuwane przez `Delete()`. Wyrzucenie wskaznika w `CActorInstance::__DestroyWeaponTrace` bedzie bezpowrotna utrata pamieci alokatora dynamicznego, obiekty `std::deque` nie zdealokuja sterty nalezacej do pool managera.
- **Mieszanie stanow D3D:** Metoda `Render()` w `CFlyTrace` wymusza `AlphaBlendEnable`, `AlphaTestEnable`, zienia ColorOps, i ustawia ZFUNC na LESS (nie zapisujac glebi ZB, uzywajac Additive Blending - src alpha, dest one). Jesli na zakonczeniu `Render()` stan powrotny jest uszkodzony, wszystkie dalsze interfejsy UI lub tekstury beda niespodziewanie puste lub przeswitujace.
- **Pulapki Trygonometrii Kamery:** Generowanie szerokosci smugi dziala uzywajac iloczynow wektorowych pomiedzy punktami toru a pozycja okularu kamery z `CCamera::GetEye()`. W skrajnych rzutach (np. patrzac idealnie w os z gory wzdluz strzaly) wektory potrafia byc rownolegle co sprawia ze iloczyn da wektor zerowy. Powoduje to narysowanie polygonu z zerowym polem lub artefakty.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

**Instrukcja implementacji nowej logiki (np. Smooth Catmull-Rom na szlakach miecza):**
1. Otworz `src/GameLib/WeaponTrace.h`. Wprowadz zmienne interpolacyjne lub zmodyfikuj strukture punktow zapisujaca nie tylko pozycje, ale tez kierunki styczne do wezlow kostnych modelu, jak zalozone w niezaimplementowanym wczesniej komentarzu "// change CatMull to cubic spline".
2. W `ActorInstanceWeaponTrace.cpp` usun ograniczenie na uzywanie jedynie prostej pary wektorow z jednego Bone, do podawania 2-3 punktow powiazanych z mieczem.
3. Oblicz klatki w przestrzeni sferycznej przed wywolaniem `STATEMANAGER.DrawPrimitiveUP` aby zredukowac katowe przeskoki wierzcholkow miedzy frame'ami (zapobiega "kanciastej smudze" przy niskim FPS).

**Debugowanie systemu:**
- Wszystkie systemowe trace logi uzywaja (obecnie zakomentowanych) wywolan `Tracenf`. Mozesz odkomentowac bloki na przyklad w `FlyTrace.cpp::Render()` - `//for(i=0;i<6;i++) Tracenf("#%d:%f %f %f", i, v[i].p.x,v[i].p.y,v[i].p.z);` do analizy dokladnych obrotow polygonow. Zrekompiluj klient, aby ogladac wartosci trajektorii wpisywane do glownego piku `syserr.txt`.

**Testowanie Headless:**
- Podczas weryfikacji dzialania w Linux / Headless, zakazane jest includowanie glownego renderera. Nalezy zasymulowac (mock) dzialanie klasy `STATEMANAGER` i metody `DrawPrimitiveUP` po to zeby zwrocila na stdout uzyte pozycje wierzcholkow w formacie weryfikowalnym asercjami. Mock wektorowy np. biblioteka `glm` ulatwi weryfikacje poprawnosci algorytmu CrossProduct w porownaniu z Direct3DX.
