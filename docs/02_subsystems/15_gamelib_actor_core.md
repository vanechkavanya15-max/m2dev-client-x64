# Podsystem GameLib: CActorInstance - Rdzen Postaci, Kosci i Dolaczanie Modeli

## 1. Cel Architektoniczny i Rola Modulu
`CActorInstance` to centralny silnik logiki postaci (graczy, mobow, NPC, metinow) w kliencie gry. Odpowiada za integracje pomiedzy warstwa graficzna (EterLib/DirectX), systemem animacji i kosci (Granny3D), oraz warstwa fizyczna (kolizje terenu - CPythonBackground).
Glowne obowiazki:
- Zarzadzanie ruchem i blendingiem animacji (pomiedzy postawa, biegiem, a atakiem).
- Attachowanie (dolaczanie) pancerzy, wlosow, broni, i efektow czasteczkowych do konkretnych kosci szkieletu (`Bone Index`) uzywajac Granny.
- Aktualizowanie wektorow kolizji (Bounding Sphere) wzgledem terenu.
- Synchronizacja z jazda konna (przesuniecie srodka ciezkosci obiektu `m_pkHorse`).

### Zaleznosci
- **Kto wywoluje:** `CPythonCharacterManager` zarzadza lista aktorow. Skrypty Python (`root/player.py`) poprzez API C++ (`CPythonPlayer`) wysylaja polecenia np. ataku czy przemieszczenia (MOVE_TO z pakietow sieciowych).
- **Biblioteki:** `EterLib` (render, shadery, DirectX), `EterGrnLib` (animacje .msa, kosci postaci .gr2), `PhysicsObject` (Sphere collision).

## 2. Diagram Architektury i Przeplywu Danych (Mermaid)
```mermaid
graph TD
    subgraph Warstwa Zewnetrzna
        NET[Siec / Pakiety Serwera]
        PY[Python C-API / Skrypty]
    end

    subgraph CActorInstance (Rdzen)
        AI_Render[Renderowanie i Transformacja]
        AI_Motion[Zarzadzanie Ruchem i Blending]
        AI_Attach[Zarzadzanie Ekwipunkiem i Kosci]
        AI_Event[Zdarzenia i Dzwieki]
    end

    subgraph Moduly Pomocnicze
        GRN[Granny3D - Skeletal Mesh]
        D3D[EterLib - D3D8 Device]
        TER[AreaTerrain - Wysokosc Terenu]
    end

    NET -->|Aktualizacje Pozycji, Atak| AI_Motion
    PY -->|Zmiana Broni, Umiejetnosci| AI_Attach
    
    AI_Motion -->|Pobieranie Klatek (Frames)| GRN
    AI_Attach -->|Mapowanie Kosci (Bip01)| GRN
    
    AI_Render -->|Pobieranie Z-Pos| TER
    AI_Render -->|Wysylanie Macierzy| D3D
    AI_Event -->|Czasteczki/Efekty| D3D
```

## 3. Rejestr Struktur Danych i Pamieci (Memory & Struct Layout)

Ponizsze struktury posiadaja wyrownanie zgodne z domyslnym kompilatorem x86 (MSVC, 4 bajty).

### `enum EType`
Klasyfikuje glowny byt obiektu.
- `TYPE_ENEMY` (0): Potwory agresywne.
- `TYPE_NPC` (1): Przyjazne postacie, handlarze.
- `TYPE_STONE` (2): Kamienie Metin (brak rotacji ciala, specjalne efekty rozpadu).
- `TYPE_WARP` / `TYPE_DOOR` / `TYPE_BUILDING`: Elementy otoczenia mapy traktowane jako pseudo-aktorzy.
- `TYPE_PC` (6): Gracz (Player Character).
- `TYPE_HORSE` (8): Wierzchowiec, posiada wlasna logike animacji LOOP.

### `struct SState`
Wektor fizyczny biezacego stanu, przekazywany do EventHandlerow.
- `TPixelPosition kPPosSelf;`: Struktura 3x Float (x, y, z) koordynatow na mapie.
- `FLOAT fAdvRotSelf;`: Kat obrotu postaci w stopniach (float).

### `struct SReservingMotionNode (TReservingMotionNode)`
Wezel animacji rezerwowej (nastepna animacja do odtworzenia po zakonczeniu biezacej).
- `EMotionPushType iMotionType;`: Typ (Loop, Once).
- `float fStartTime;`: Czas rozpoczecia blendingu w milisekundach.
- `float fBlendTime;`: Czas na jaki uruchomiony jest algorytm mieszania klatek (interpolacja).
- `float fDuration;`: Calkowity czas trwania animacji z pliku .msa.
- `float fSpeedRatio;`: Wspolczynnik szybkosci ataku/ruchu wplywajacy na mnoznik czasu klatki.
- `DWORD dwMotionKey;`: Hasz Vnum identyfikujacy nazwe akcji.

### `struct SCurrentMotionNode`
Wezel aktualnie odtwarzanej akcji.
- Posiada `dwcurFrame` (Biezaca klatka Granny) oraz `dwFrameCount` (calkowita ilosc klatek w animacji), ktore sa aktualizowane na podstawie szybkosci klatek gry `g_fGameFPS`.

### `struct TAttachingEffect`
Pojedynczy instancjonowany efekt wizualny (.mse) zamocowany na ciele postaci.
- `DWORD dwEffectIndex;`: ID efektu zarejestrowane w CEffectManager.
- `int iBoneIndex;`: Indeks kosci Granny (np. R_Hand, Head) na ktorym zespawnowano wezel.
- `DWORD dwModelIndex;`: Identyfikator czesci ciala bazowego.
- `D3DXMATRIX matTranslation;`: Lokalna macierz offsetu dla wycentrowania efektu (np. by swiecenie miecza nie bylo na rekojesci, a na ostrzu).
- `BOOL isAttaching;`: Flaga stanu zycia.

## 4. Rejestr Klas i Metod (API Reference)

### `void CActorInstance::INSTANCEBASE_Transform()`
- **Logika:** Jest to glowny wezel aktualizacji fizycznej wykonywany co klatke (`Update()`). Jesli aktor jedzie na koniu (`m_pkHorse != NULL`), funkcja wywoluje transformacje na wierzchowcu, a nastepnie kradnie jego wspolrzedne (`m_x = m_pkHorse->...x`, `m_y = -m_pkHorse->...y`). Nastepnie odpala obsluge trzesienia ekranu `ShakeProcess()`, przelicza `UpdateBoundingSphere()` i aktualizuje statystyki materialu (alpha blend).

### `void CActorInstance::INSTANCEBASE_Deform()`
- **Logika:** Aplikuje transformacje CPU (Software Skinning). Wywoluje metode `Deform()` odswiezajaca wezly Granny i `TraceProcess()` sprawdzajacy slad pozostawiany przez bron (np. efekt ciosu mieczem w powietrzu). Wynikiem jest zdeformowana siatka wierzcholkow gotowa dla Direct3D.

### `void CActorInstance::MotionProcess(BOOL isPC)`
- **Logika:** Sterownik maszyny stanow animacji. Uruchamia dzwieki oraz zdarzenia (.msa events) za pomoca `__MotionEventProcess`. Nastepnie interpoluje przejscia miedzy stara a nowa animacja (`ReservingMotionProcess`), przeliczajac proporcje klatek w oparciu o `fSpeedRatio` (Speed Hack protection and calculation). Wierzchowce sa omijane i kierowane do funkcji specjalnej `HORSE_MotionProcess`, wymuszajacej odtwarzanie petli bez blokowania jezdzca.

### `DWORD Vietnam_ConvertWeaponVnum(DWORD vnum)` (Global Helper)
- **Logika:** Wietnamska konwersja identyfikatorow przedmiotow. Hardkodowany switch ucinajacy pierwsza liczbe (zmieniajacy bazowe vnumy broni wedlug wzoru modulo 10). Modyfikuje wyswietlanie wizualne modelu broni nie naruszajac prawdziwych statystyk na serwerze (Client-Side hack).

### `void CActorInstance::AttachWeapon(DWORD dwItemIndex, DWORD dwParentPartIndex, DWORD dwPartIndex)`
- **Logika:** Funkcja wczytuje z `CItemManager` pliki `.gr2` i `.tga` dla podanego ID (Vnum) broni. Odnajduje kosc odpowiadajaca prawemu nadgarstkowi (`Bip01 R Hand`). Uzywa `m_pkCurRaceData->GetItemAttachPosition` aby przesunac i obrocic model broni wzgledem reki. Zapisuje to do wlasnej struktury attach-ow, konczac operacje zbudowaniem relacji pomiedzy szkieletem broni a ciala gracza.

### `void CActorInstance::UpdatePointInstance()`
- **Logika:** Sluzy optymalizacji kolizji Hitbox. Odczytuje uaktualniona globalna macierz macierzysta gracza (`matWorld`) i przemieszcza srodki (center-points) dynamicznych sfer (`CDynamicSphereInstanceVector`). Dzieki temu gra wie, czy uderzenie mieczem sieglo przeciwnika w danym cyklu odswiezania `ELTimer_GetMSec()`.

## 5. Punkty Styku (Cross-Subsystem Integration)

- **DirectX (EterLib/Direct3D):** Kod wymusza bezposrednie operacje na buforach poprzez `GetGraphicsDevice()->SetTransform`. Operuje na flagach renderowania `ERenderMode` by przelaczac sie miedzy `RENDER_MODE_BLEND` (Znikajace postacie/niewidzialnosc) a `RENDER_MODE_NORMAL`.
- **Python:** Istnieje most C-API wyeksponowany w module `player`. Na przyklad `player.SetAttackKeyState()` triggeruje zmiane kolejki `SReservingMotionNode`, co narzuca animacje ataku i uruchamia testy promieniowe (Raycast) w kierunku wroga.
- **Granny3D:** To caly fundament animacyjny. Konwersja na wewnetrzne struktury kosci Granny. Klatki kluczowe i zgiecia stawow pobierane sa z `GetBoneMatrix`, wymagane by np. wlosy poruszaly sie ze zmiana pozycji glowy postaci.
- **Sieciowosc:** Logika postaci opiera sie na `PushPath`. Zamiast teleportowac postac, serwer wysyla pakiety trasy ruchu (wielokatne). CActorInstance zjada te punkty kontrolne (`m_kPPosSelf`) zmuszajac postac do fizycznego spaceru po podanych kosciach z predkoscia Movement Speed z serwera.

## 6. Pulapki, Antywzorce i Ograniczenia

1. **Bottleneck Skalowania CPU (Software Skinning):** Operacja `INSTANCEBASE_Deform()` na silniku Granny 3D jest realizowana przez procesor. W miejscach zatloczonych (np. eventy, sklepy w miescie), powoduje to ze watek glowny (Single Core) dlawi sie calkowicie wykonujac przeliczenia wierzcholkow na macierzach, drastycznie upuszczajac FPSy. Klient nie wspiera Hardware Skinningu shaderami na GPU.
2. **Hardcoding Koni:** Kod zawiera nieeleganckie obejscia w postaci `m_y = -m_pkHorse->NEW_GetCurPixelPositionRef().y`. Odwracanie znaku wspolrzednej osi Y przy transformacji na wierzchowcu jest nastepstwem bledow konwertera 3DSMAX -> GR2. Wymusza to uwazne modelowanie przyszlych zwierzat aby nie biegaly "tylem".
3. **Ghosting / Sliding (Blend Bugs):** Niestabilny odczyt klatek w polaczeniu ze zmiennoprzecinkowym czasem interpolacji `fBlendTime` rodzi desynchronizacje. Powoduje to glitche: gracz moze sie przemieszczac, lecz animacja utknela - jego stopy slizgaja sie po terenie.
4. **Pamiec TAttachingEffect:** Cykl zycia czasteczek przypietych do aktora czesto przezywa destrukcje samej postaci jesli `ClearAttachingEffect()` zawiedzie (np. podpieta poswiata gm'a). Tworzy to ukryte wycieki do momentu zawalenia VRAMu i zamkniecia klienta kodem `D3DERR_DEVICELOST`.

---
*Dokumentacja zostala wygenerowana wg norm inzynierii wstecznej by stanowic Single Source of Truth dla systemow sztucznej inteligencji (bez stosowania znakow diakrytycznych).*
