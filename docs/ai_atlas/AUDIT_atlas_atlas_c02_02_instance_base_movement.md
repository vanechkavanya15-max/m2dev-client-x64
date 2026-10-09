---
task_id: "atlas_c02_02_instance_base_movement"
cluster: "ACT"
module_name: "CInstanceBase - Kinematyka Ruchu i Interpolacja Pozycji"
target_files:
- src/UserInterface/InstanceBaseMovement.cpp
- src/UserInterface/InstanceBase.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_02_instance_base_movement.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT_atlas_atlas_c02_02_instance_base_movement

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Podsystem `InstanceBaseMovement` odpowiada za calosc kinetyki, przemieszczania, rotacji i orientacji przestrzennej bytow (`CInstanceBase`) w trojwymiarowym swiecie gry Metin2.
- **Rola w silniku**: Implementuje wektorowa mechanike poruszania sie postaci gracza, potworow (mobs), NPC, zwierzat oraz wierzchowcow (`m_kHorse`).
- **Przeplyw danych (Data Flow)**:
  1. Zadanie ruchu (np. klikniecie mysza w terenie, wcisniecie WASD lub pakiet sieciowy `TPacketGCCharacterMove`):
     - Wywolywane metody wysokiego poziomu: `NEW_MoveToDestPixelPositionDirection()`, `NEW_Goto()` lub `NEW_MoveToDirection()`.
  2. Obliczenia trygonometryczne i orientacja:
     - Wyznaczanie wektora kierunkowego miedzy pozycja zrodlowa (`SrcPixelPosition`) a docelowa (`DstPixelPosition`).
     - Normalizacja wektora 2D/3D i rzutowanie katowe za pomoca `D3DXVec3Normalize`, `D3DXVec3Dot`, `acosf` i `D3DXToDegree`.
     - Wyznaczenie kata natarcia (`AdvancingRotation`) oraz kierunku rotacji bryly (`m_iRotatingDirection` - lewo/prawo).
  3. Integracja z warstwa graficzna (`CActorInstance` / `CGraphicThingInstance`):
     - Metody `m_GraphicThingInstance.Move()`, `Stop()`, `SetRunMode()`, `SetWalkMode()`.
     - Odpowiednie skalowanie predkosci ruchu (`uMovSpd/100.0f`) oraz predkosci ataku (`uAtkSpd/100.0f`).
  4. Efekty wizualne powiazane z ruchem (Buffy / Aury):
     - Automatyczne podpinanie efektow czaru `AFFECT_GYEONGGONG` (GyeongGong - Lekki Krok Ninja) oraz `AFFECT_KWAESOK` (Szybkosc Szamana) podczas startu marszu/biegu (`StartWalking`) oraz ich odpinanie przy zatrzymaniu (`EndWalking`).

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound)**:
  - `PythonPlayer.cpp` / `PlayerMovementController.cpp` - polecenia ruchu gracza lokalnego.
  - `PythonNetworkStreamPhaseGameActor.cpp` - odbieranie pakietow przesuniecia innych graczy i potworow (`TPacketGCCharacterMove`).
  - `InstanceBaseBattle.cpp` - doskoki w trakcie ataku, odrzucenie (Knockback/Fly).
- **Zaleznosci wyjsciowe (Outbound)**:
  - `m_GraphicThingInstance` (`CActorInstance`) - bezposrednia synchronizacja szkieletu Granny i transformacji bazy obiektu.
  - `m_kHorse` (`CHorseInstance`) - synchronizacja predkosci wierzchowca.
  - `D3DX9` (`d3dx9math.h`) - operacje wektorowe i macierzowe (`D3DXVec3TransformCoord`, `D3DXMatrixRotationZ`, `D3DXVec3Scale`).
  - `PythonBackground` - odpytywanie o kolizje i wysokosc terenu przy sprawdzaniu mozliwosci ruchu (`NEW_CanMoveToDestPixelPosition`).
- **Drzewo dyrektyw `#include`**:
  - `StdAfx.h`, `InstanceBase.h`, `PythonBackground.h`, `EterLib/GrpMath.h`.
- **Model pamieciowy**:
  - `CInstanceBase` jest obiektem zlozonym, alokowanym dynamicznie na stercie w `CPythonCharacterManager`. Metody ruchu operuja na strukturach referencyjnych `TPixelPosition` (wektory 3D float x, y, z) bez dodatkowych alokacji heapowych w petli klatki.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabela Metod Publicznych Kinematyki:
| Metoda | Sygnatura C++ | Typy argumentow | Zwracana wartosc | Opis i Efekty Uboczne |
|---|---|---|---|---|
| `SetMoveSpeed` | `void SetMoveSpeed(UINT uMovSpd)` | `UINT uMovSpd` | `void` | Ustawia predkosc ruchu. Wartosci > 1100 sa zerowane (anty-speedhack). Skaluje instancje graficzna oraz konia. |
| `SetAttackSpeed` | `void SetAttackSpeed(UINT uAtkSpd)` | `UINT uAtkSpd` | `void` | Ustawia predkosc animacji ataku. Limit 1100. |
| `NEW_Goto` | `bool NEW_Goto(const TPixelPosition& c_rkPPosDst, float fDstRot)` | `const TPixelPosition&, float` | `bool` | Glowna metoda zlecenia marszu do celu. Sprawdza locki, stan skilli i bariery kolizji. |
| `NEW_Stop` | `void NEW_Stop()` | `void` | `void` | Zatrzymuje instancje, konczy marsz, wstrzymuje ruch w silniku graficznym. |
| `NEW_MoveToDirection` | `void NEW_MoveToDirection(float fDirRot)` | `float fDirRot` | `void` | Ruch ciagly w zadanym kierunku katowym (klawisze WASD). Wyznacza cel przesuniety o 300 jednostek. |
| `SetAdvancingRotation` | `void SetAdvancingRotation(float fRotation)` | `float` | `void` | Ustawia docelowy kat natarcia i predkosc obrotu (`m_fRotSpd`) w zaleznosci od odchylenia (>45 st.). |
| `StartWalking` | `void StartWalking()` | `void` | `void` | Wprowadza model w stan ruchu (`Move()`) i podpina aury predkosci. |
| `EndWalking` | `void EndWalking(float fBlendingTime = 0.0f)` | `float` | `void` | Zatrzymuje ruch i odczepia aury predkosci. |
| `IsWalking` | `BOOL IsWalking()` | `void` | `BOOL` | Zwraca TRUE gdy model jest w trakcie przemieszczania. |
| `IsGoing` | `BOOL IsGoing()` | `void` | `BOOL` | Flaga logiczna `m_isGoing` oznaczajaca podroz do wyznaczonego punktu docelowego. |

### Pamieciowy Layout Struktur (Memory Layout & Offsets):
- `TPixelPosition`: struktura z `EterLib` skladajaca sie z `float x, y, z` (12 bajtow).
- Pola stanu w `CInstanceBase`:
  - `m_isGoing`: `BOOL` (4 bajty)
  - `m_fDstRot`: `float` (4 bajty) - docelowa rotacja w stopniach
  - `m_fRotSpd`: `float` (4 bajty) - biezaca predkosc obrotu bryl
  - `m_fMaxRotSpd`: `float` (4 bajty) - maksymalna dopuszczalna predkosc obrotu
  - `m_iRotatingDirection`: `int` (4 bajty) - flaga kierunku obrotu (zgodnie / przeciwnie do wskazowek zegara)

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe**:
  - Odbior: `TPacketGCCharacterMove` (opcode 0x03) przekazuje `bFunc` (MOVE, RUN, STOP), koordynaty X, Y oraz kat rotacji `fRot`. Dyspozytor sieciowy wywoluje bezposrednio `NEW_Goto()` lub `NEW_Stop()`.
  - Wysylanie: Ruch lokalnego gracza generuje `TPacketCGMove` (opcode 0x03) wysylany w petli z `PythonPlayer.cpp`.
- **Python C-API (`mod_chr` / `mod_player`)**:
  - `player.SetAttackSpeed(val)`, `player.SetMoveSpeed(val)`
  - `chr.MoveToDestPosition(vid, x, y)`

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
1. **Zabezpieczenie przed przepelnieniem predkosci (Anti-Overspeed Clamp)**:
   - Metody `SetAttackSpeed` oraz `SetMoveSpeed` sprawdzaja `if (uAtkSpd > 1100) uAtkSpd = 0;`. Przekroczenie 1100 jednostek powoduje calkowite zatrzymanie animacji.
2. **Inwariant Locka i Uzywania Skilli**:
   - `NEW_Goto()` i `NEW_Stop()` natychmiast przerywaja dzialanie jesli `__IsSyncing()` lub `isLock()` lub `IsUsingSkill()` zwraca prawde. Zmiana pozycji w trakcie ladowania animacji skilla bez flagi moving skill spowoduje desynchronizacje z serwerem.
3. **Uklad Wspolrzednych DirectX vs Silnik Swiata**:
   - W silniku klienta osie ekranu i D3D maja odwrocona os Y:
     `kD3DVt3Cur(kPPosCur.x, -kPPosCur.y, kPPosCur.z)` oraz `kPPosDst.y = -kD3DVt3Dst.y`.
     Bledne podstawienie znaku prowadzi do natychmiastowego cofania postaci (tzw. rubberbanding).

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
1. **Modyfikacja zachowania ruchu**:
   - Jesli implementujesz plynna interpolacje (Prediction / Lerp), nie modyfikuj bezposrednio funkcji `NEW_Goto()`. Skorzystaj z kontrolera `Client::Gameplay::MovementInterpolationEngine`.
2. **Dodawanie nowych efektow buffow biegowych**:
   - W `StartWalking()` oraz `EndWalking()` dodaj obsluge kolejnego affectu w analogii do `AFFECT_GYEONGGONG` i `AFFECT_KWAESOK`.
3. **Weryfikacja w trybie Headless**:
   - Matematyke wektorowa mozna w 100% testowac bez okna DirectX przez wywolywanie funkcji z `GrpMath.h` i prosty test jednostkowy sprawdzajacy `CInstanceBase_GetDegreeFromPosition`.