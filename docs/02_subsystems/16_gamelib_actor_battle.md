# GameLib: CActorInstance - Walka, Kolizje i Obsluga Obrazen

## 1. Cel Architektoniczny i Rola Modulu

Klasa `CActorInstance` (a scislej jej podsystem zamkniety w plikach `ActorInstanceBattle.cpp`, `ActorInstanceCollisionDetection.cpp` i `ActorInstanceData.cpp`) to centralny silnik obslugi walki (Battle Engine), rejestracji danych wygladu oraz detekcji kolizji dla kazdego aktora (gracza, potwora, NPC) w kliencie gry (Metin2).

Glowna odpowiedzialnoscia modulu jest:

- Obliczanie i procesowanie atakow fizycznych, ataku obszarowego (Splash) i weryfikacja zasiegu wzgledem przeciwnikow.
- Aktualizacja i ladowanie wlasciwosci aktora: ksztalty (Shape), materialy (Materials), czesci wizualne (Parts np. wlosy), i obiekty dodatkowe (efekty ataku i uderzen).
- Obliczanie fizycznych wlasciwosci ataku (odrzut, pchniecie/Pushing, trzesienie/Shake, stan ogluszenia/Stun).
- Sprawdzanie dynamicznych kolizji typu Sfera vs Sfera miedzy walczacymi instancjami (HitData, przeliczanie ramki animacji ataku, uderzenia i powstrzymywania nadmiernej liczby trafien na klatke).

**Zaleznosci i Integracje:**

- Modul wywolywany jest glownie przez glowny obieg aktualizacji (Update) instancji aktora i menedzer instancji postaci.
- Korzysta bezposrednio z `EterLib`/`EterGrnLib` (granny animacje `CGraphicThingInstance`), menedzera efektow `CEffectManager`, menedzera zasobow `CResourceManager` oraz systemu ras/modeli `CRaceManager`, `CRaceData`.

## 2. Diagram Architektury i Przeplywu Danych (Mermaid)

```mermaid
sequenceDiagram
    participant MainLoop as Petla Gry (Update)
    participant Actor as CActorInstance (Atakujacy)
    participant Collision as ActorInstanceCollisionDetection
    participant Battle as ActorInstanceBattle
    participant Victim as CActorInstance (OFiara)

    MainLoop->>Actor: Update / OnAttack()
    Actor->>Collision: AttackingProcess(Victim)

    alt Typ ataku: Obszarowy
        Collision->>Collision: __SplashAttackProcess(Victim)
    else Typ ataku: Zwykly
        Collision->>Collision: __NormalAttackProcess(Victim)
    end

    Collision->>Collision: Sprawdzenie dystansu (Distance <= 300 / 500)
    Collision->>Collision: CheckCollisionDetection(DynamicSphere VS DynamicSphere)

    opt Jesli Kolizja wystapila
        Collision->>Battle: __ProcessDataAttackSuccess(AttackData, Victim, v3HitPosition)
        Battle->>Victim: InsertDelay (StiffenTime / InvisibleTime)

        alt Odpychanie dozwolone (ExternalForce > 0)
            Battle->>Victim: __PushCircle(Victim) / IncreaseExternalForce
        end

        Battle->>CEffectManager: CreateEffect (Hit Effect)

        alt Typ ofiary: Kamien/Drzwi
            Battle->>Victim: __HitStone() -> __Shake()
        else Trafienie Dobre (HIT_TYPE_GOOD)
            Battle->>Victim: __HitGood() -> InterceptOnceMotion(DAMAGE)
        else Trafienie Swietne (HIT_TYPE_GREAT)
            Battle->>Victim: __HitGreate() -> InterceptOnceMotion(DAMAGE_FLYING)
        end

        Battle->>Actor: __OnHit(uiSkill, Victim, isSendPacket)
    end
```

## 3. Rejestr Struktur Danych i Pamieci (Memory & Struct Layout)

Ponizej znajduja sie kluczowe struktury definiujace operacje z tych plikow, czesto rezydujace w naglowkach, na ktorych operuja procedury (typowe wyrownanie to `#pragma pack(push, 4)` domyslne dla kompilatora MSVC x86, choc tu explicite nie zostalo wymuszone przez `#pragma pack` w analizowanym kodzie, polega wiec na wyrownaniu strukturalnym C++ std).

**`THittedInstanceMap` (Typedef / Kontener)**

```cpp
typedef std::map<CActorInstance*, float> THittedInstanceMap;
```

- Klucz: `CActorInstance*` (Wskaznik na ofiare - `DWORD`, 4 bytes na arch. 32-bit).
- Wartosc: `float` (Czas, oznaczajacy `GetLocalTime() + pad->fInvisibleTime` do ktorego instancja jest "niewidzialna" na ponowne hity, 4 bytes).

**`THitDataMap` (Typedef / Kontener)**

```cpp
typedef std::map<const NRaceData::THitData *, THittedInstanceMap> THitDataMap;
```

- Klucz: Wskaznik na stale dane uderzenia (`THitData*`, 4 bytes).
- Wartosc: Powyzsza mapa `THittedInstanceMap`.
- Kontener zapobiega uderzaniu tych samych ofiar wiele razy w tej samej sekundzie tym samym ciosem z danej ramki animacji ataku.

**`CActorInstance::SSetMotionData`**

```cpp
struct SSetMotionData {
    MOTION_KEY dwMotKey;   // DWORD (4 bytes)
    float      fSpeedRatio; // float (4 bytes)
    float      fBlendTime;  // float (4 bytes)
    int        iLoopCount;  // int (4 bytes)
    UINT       uSkill;      // UINT (4 bytes)
}; // Rozmiar calkowity: 20 bajtow. Packing: standardowy dword.
```

- Uzywane przy wyzwalaniu animacji.

## 4. Rejestr Klas i Metod (API Reference)

### Plik: ActorInstanceBattle.cpp

`void CActorInstance::__ProcessDataAttackSuccess(const NRaceData::TAttackData & c_rAttackData, CActorInstance & rVictim, const D3DXVECTOR3 & c_rv3Position, UINT uiSkill, BOOL isSendPacket)`

- **Parametry**: `c_rAttackData` (dane konfiguracyjne ataku - czasy sztywnosci, typ), `rVictim` (cel), `c_rv3Position` (pozycja efektu hit), `uiSkill` (ID skilla), `isSendPacket` (czy poinformowac serwer).
- **Logika**: Najpierw dodaje `InsertDelay` (animacja zesztywnienia). Sprawdza czy cel moze byc odepchniety (`__CanPushDestActor`), jezeli ma `fExternalForce > 0`, wywoluje `__PushCircle` i aplikuje sile fizyczna do `m_PhysicsObject`. Nastepnie przyznaje ofierze `m_fInvisibleTime` (niewrazliwosc na ciosy - obsluguje specyfike party / potworow oraz latki regionalne np. M2KR). Uruchamia graficzne `Hit Effect` na pozycji `vec3Effect`. Jesli `rVictim` to kamien (Metin) -> wywoluje `__HitStone`. Jesli zwykly gracz/mob: `__HitGood` lub `__HitGreate` w zaleznosci od `iHittingType`. Ostatecznie zglasza fakt uderzenia do reszty systemu wywolujac `__OnHit`.

`void CActorInstance::OnShootDamage()`

- **Logika**: Krotka procedura aplikujaca otrzymanie "strzalu". Jesli instancja jest ogluszona (Stun), od razu umiera (`Die()`). W przeciwnym razie uaktywnia `__Shake(100)` trzesienie ekranu/modelu. Zmienia animacje na DAMAGE i kolejkuje WAIT.

`void CActorInstance::__Shake(DWORD dwDuration)` / `void CActorInstance::ShakeProcess()`

- **Logika**: `__Shake` ustawia `m_dwShakeTime`. `ShakeProcess` (wywolywane w petli) modyfikuje pozycje macierzy swiata (`m_worldMatrix._41, _42, _43`) losowymi wartosciami typu rand()%10 (skoki X, Y, Z o niewielki wektor), tworzac wizualny efekt wstrzasu.

`void CActorInstance::__HitStone(CActorInstance& rVictim)`

- **Logika**: Dedukowana obsluga struktur stalych (np. Kamienie Metin). Jezeli ma status stun umiera, w przeciwnym razie ulega tylko `__Shake(100)` (bez animacji upadania i knock-backu).

`void CActorInstance::__HitGood(CActorInstance& rVictim)`

- **Logika**: Typowy cios (Zwykle obrazenia). Oblicza kat (dot product 2D z wektorow Normal z sin/cos rotacji aktora ataktujacego i ofiary). Jesli Skalar < 0 (atak od przodu/tylu determinowany wzgledem wektorow patrzacych na siebie) uzywa animacji `DAMAGE`, jak skalar > 0 stosuje `DAMAGE_BACK`. Wywoluje wstrzas `__Shake`.

`void CActorInstance::__HitGreate(CActorInstance& rVictim)`

- **Logika**: Zamaszysty/potezny cios, najczesciej konczacy combo. Ignoruje knock-down (Jezeli lezy - pomija). Oblicza pozycje dot-product by ustalic przod/tyl. Wywoluje potezne animacje odrzucenia (Flying): `DAMAGE_FLYING` lub `DAMAGE_FLYING_BACK`, plynnie pushujac nastepnie animacje `STAND_UP` i `WAIT`.

`void CActorInstance::__PushCircle(CActorInstance & rVictim)`

- **Logika**: Promieniowy odrzut ofiary. Oblicza znormalizowany wektor od atakujacego do ofiary i wywoluje na ofierze `__SetFallingDirection(v3Direction.x, v3Direction.y)`.

### Plik: ActorInstanceCollisionDetection.cpp

`BOOL CActorInstance::AttackingProcess(CActorInstance & rVictim)`

- **Zwracana wartosc**: `TRUE` w przypadku wykrycia zadanego uderzenia, inaczej `FALSE`.
- **Logika**: Najpierw sprawdza `rVictim.__isInvisible()`. Potem kolejno probuje `__SplashAttackProcess` a potem `__NormalAttackProcess`. Zwraca boolean od uderzenia.

`BOOL CActorInstance::__SplashAttackProcess(CActorInstance & rVictim)`

- **Logika**: Atak obszarowy (AoE). Optymalizacja dystansu wstepna w kwadracie (LengthSq) >= 1000^2 (odrzuca ewaluacje dalekich modeli). Limituje maksymalna liczbe "trafien na klatke" przez `iHitLimitCount` (default: 16 trafien na combo, zapobiega zjawisku zabicia servera ogromna iloscia requestow uderzen). Szuka sfer `CheckCollisionDetection`, dodaje hit do mapy `rHittedInstanceMap` uniemozliwiajac spam, i wywoluje `__ProcessDataAttackSuccess`.

`BOOL CActorInstance::__NormalAttackProcess(CActorInstance & rVictim)`

- **Logika**: Standardowy atak "target-based". Wstepny distance-check: 500x500 dla duzych potworow (HUGE_RACE), 300x300 dla standardowych z uzyciem odleglosci `(rVictim.m_x - m_x, rVictim.m_z - m_z)`. Iteruje przez kolekcje ciosow w pamieci (HitDataContainer). Odrzuca zbyt wczesne/pozne ciosy na osi czasu animacji `CTimer::Instance().GetElapsedSecond()`. Testuje styk precyzyjny cylindrow/sfer `DetectCollisionDynamicZCylinderVSDynamicZCylinder`. Po sukcesie rejestruje czas uderzenia dla wlasciwosci niewidzialnosci i aplikuje hit. Ograniczenie twarde do 16 osob na hit normalny.

`BOOL CActorInstance::TestActorCollision(CActorInstance & rVictim)`

- **Logika**: Mechanizm blokowania postaci przez postac (Walking blocker). Jesli aktory (Body spheres) nakladaja sie na siebie (DynamicSphereVSDynamicSphere), zapobiega to swobodnemu przenikaniu sie aktorow w trybie ruchu, bazujac na odleglosci miedzy glownymi sferami.

### Plik: ActorInstanceData.cpp

`void CActorInstance::SetRace(DWORD eRace)`

- **Logika**: Ladowanie i wiazanie glownego profilu (RaceData). Rezerwuje miejsce na `PART_MAX_NUM` elementow dla Gracza (PC), zas tylko 1 element dla potworow. Mapuje setki wektorow ruchowych (MotionVectorMap) dla kazdego trybu aktora i wczytuje klatki `CGraphicThingInstance::RegisterMotionThing`.

`void CActorInstance::SetShape(DWORD eShape, float fSpecular)` / `SetHair(DWORD eHair)`

- **Logika**: Ustawienie wygladu zewnetrznego (Zbroja/Szata/Wlosy). Wczytuje i przypisuje zzewnetrzne zrodla modelu Granny 3D (.gr2). Automatycznie probuje podladowac siatki poziomu detali LOD (\_lod_01.gr2, \_lod_02.gr2). Jezeli obiekt to dzrewo (Tree) uzywa CTree instancing. Wykonuje takze wczytywanie shaderow materialu - jezeli `fSpecular > 0.0f` odpala nakladke refleksyjna `SMaterialData` mapiac textury i wlanczajac `isSpecularEnable`. Dodatkowo, doczepia odpowiednie efekty czasteczkowe predefiniowane dla wygladu.

## 5. Punkty Styku (Cross-Subsystem Integration)

- **Powiazanie z Systemem Pakietow Sieciowych**: Uderzenie aktora ma flage `isSendPacket` w funkcji `__ProcessDataAttackSuccess`, nastepnie dociera do `__OnHit`, ktora uaktywnia sieciowe zgloszenie zdarzenia `SendShootPacket` lub podobne w ukladzie CPythonPlayer/CPythonNetworkStream (choc samo zapakowanie opkodu odbywa sie poziom wyzej w hierarchii, poza podsystemem). Limit uderzen (max 16) rowniez chroni bufor serwera (zapobieganie DDoS przez klient).
- **Zasoby i EterLib**: Renderowanie fizyki, efektow wizualnych oraz wstrzasy kamery sa scisle sprzezone z macierzami D3DXMATRIX z EterGrnLib oraz DirectX 8/9. W `SetShape` menedzer `CResourceManager` odpowiada za dostarczenie plikow DDS/GR2 a `CEffectManager` tworzy duszki trafien.
- **Wlasciwosci Regionalne**: Widoczne specyficzne modyfikacje dla Korei (`#0000794: [M2KR]`, `#0000780: [M2KR]`). Duze moby uzywaja np. innych algorytmow celowania wektorow Hit Position (IS_HUGE_RACE).

## 6. Pulapki, Antywzorce i Ograniczenia

1. **Floating Point Inaccuracy (Distance Spikes):** Testy kolizji, z racji optymalizacji kwadratow odleglosci np. `fDistance >= 300.0f*300.0f` w `__NormalAttackProcess`, nie uzywaja `sqrt`. Moze to prowadzic do desynchronizacji uderzen jesli client mocno tnie (lag), bo pozycja ofiary (Victim) a atakujacego zalezy tylko od dwoch osi (ignoruje pion w pre-checach w `v3Distance(rVictim.m_x - m_x, rVictim.m_z - m_z, rVictim.m_z - m_z)` - tutaj w the original source jest tak naprawde blad logiczny/litera C++ bo napisane bylo `rVictim.m_z - m_z` dwukrotnie w pre-chekach dystansow uderzen - ominieto os Y!).
2. **Hardcoded Max Hit Limit:** Uderzenie zwykle i obszarowe (bez limitu konfiguracyjnego) limitowane jest z palca w kodzie do `16` na klatke ataku. Limit ten stanowi antywzorzec ukryty w kodzie cpp (zamiast plikach konfiguracyjnych/Game server parameters) co moze zdezorientowac, gdy skille w grze beda uderzac max 16 celow.
3. **Problem martwych obiektow:** Istnieje warunek `if (rVictim.IsDead()) return FALSE;`. Obiekty martwe natychmiast przestaja przyjmowac ciosy. Brak stanu overkill (hity w zewloki).
4. **Alokacja podczas lotu:** Operacje tworzenia tablic i ciagle podpinanie wezlow sfer uderzen per-frame pochlaniaja zasoby CPU i opieraja sie na ciaglym `DetectCollisionDynamicSphereVSDynamicSphere` co przy wielu jednostkach na ekranie dlawi wydajnosc jednowatkowa C++.

---

_Dokument ten sluzy jako glowne referencyjne zrodlo wiedzy dla struktur i systemow zarzadzania instancjami aktorow podczas obliczen walki i ich budowy modelu._
