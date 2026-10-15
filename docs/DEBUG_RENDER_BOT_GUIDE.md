# Podrecznik Programisty: System Rysowania Diagnostycznego i ESP (DEBUG_RENDER_BOT_GUIDE)

Ten dokument stanowi oficjalny przewodnik dla tworcow skryptow automatyzacji, test harness, botow oraz narzedzi analitycznych w srodowisku klienta Metin2 C++23.

---

## 1. Wprowadzenie i Modul `debugRender`

Silnik klienta gry posiada wbudowany, wysokowydajny mostek C-API do sprzetowego renderowania geometrii 3D bezposrednio w fazie klatki Direct3D 9:
- **Zero wyciekow stanow**: Caly rendering debugera dziala pod ochrona straznika RAII `ScopedDebugRenderGuard`.
- **Obsluga X-Ray**: Opcjonalne ignorowanie bufora Z (`xray=True`), co pozwala widziec geometrie przez sciany, podziemia, drzewa i budynki.
- **Odpornosc na limity sprzetowe**: Automatyczne batchowanie wierzcholkow w C++ (do 4096 prymitywow per DrawCall).
- **Kompensacja osi Y**: Silnik automatycznie dokonuje translacji miedzy ukladem pikseli gry a ukladem renderera Direct3D 9 (`-pos.y`).

---

## 2. Pelna Specyfikacja API Pythona

```python
import debugRender
```

### 1. Zarzadzanie Stanem i Buforem:
- `debugRender.Enable(bEnable)`: Wlacza (`True`) lub wylacza (`False`) renderowanie geometrii diagnostycznej.
- `debugRender.IsEnabled()`: Zwraca `1` jesli modul jest aktywny, `0` jesli wylaczony.
- `debugRender.Clear()`: Czysci wszystkie zakolejkowane geometrie. Zaleca sie wywolywac na poczatku kazdego ticku `OnUpdate`.
- `debugRender.GetCounts()`: Zwraca krotke `(lineCount, boxCount, circleCount)` podsumowujaca geometrie w kolejce.

### 2. Rysowanie Prymitywow 3D w Przestrzeni Swiata:
- `debugRender.DrawLine(x1, y1, z1, x2, y2, z2, color, xray=False)`:
  Rysuje odcinek pomiedzy punktami A i B.
- `debugRender.DrawAABB(minX, minY, minZ, maxX, maxY, maxZ, color, xray=False)`:
  Rysuje prostopadloscian orientowany osiowo (AABB).
- `debugRender.DrawCircle(cx, cy, cz, radius, color, xray=False, segments=24)`:
  Rysuje okrag na ziemi wokol punktu `(cx, cy, cz)` z automatycznym liftem +2.5f zapobiegajacym z-fightingowi.
- `debugRender.DrawCross(x, y, z, size=50.0, color=0xFFFFFFFF, xray=False)`:
  Rysuje przestrzenny krzyzyk 3D (3 osie) w punkcie swiata (np. znacznik dropu, waypointu).

### 3. Sledzenie i Oznaczanie Aktorow (Potwory, Gracze, Metiny):
- `debugRender.DrawTracer(vid, color, xray=True)`:
  Rysuje linie od wysokosci klatki piersiowej gracza glownego do celu o podanym `vid`.
- `debugRender.DrawBox(vid, color, xray=False)`:
  Rysuje ramke AABB dopasowana do pozycji wskazanego aktora.
- `debugRender.DrawCircleActor(vid, radius, color, xray=False, segments=24)`:
  Rysuje okrag na ziemi dokladnie pod stopami wskazanego aktora (np. zasieg ataku).

### 4. Rzutowanie Pozycji 3D na Ekran 2D (Overlay / GUI ESP):
- `debugRender.WorldToScreen(x, y, z)`:
  Rzutuje dowolna pozycje 3D na wspolrzedne okna gry.
  Zwraca: `(screenX, screenY, isVisible)`.
  Jezeli `isVisible == 0`, punkt znajduje sie za plecami kamery gracza i NIE powinien byc rysowany w 2D.
- `debugRender.GetActorScreenPos(vid)`:
  Pobiera pozycje aktora o danym VID i rzutuje ja na ekran 2D: `(screenX, screenY, isVisible)`.

### 5. Szybkie Skanowanie Otoczenia w C++:
- `debugRender.GetActorsInRadius(radius, typeFilter=-1, includeDead=False)`:
  Przeszukuje wszystkich zaladowanych aktorow na scenie w zadanym promieniu od gracza.
  - `radius`: promien w jednostkach gry (np. 1500.0, 3000.0).
  - `typeFilter`: filtr typu aktora (`-1` = wszystkie, lub stala `TYPE_*`).
  - `includeDead`: czy uwzgledniac martwe istoty (`False` domyslnie pomija trupy).
  Zwraca krotke identyfikatorow: `(vid1, vid2, vid3, ...)`.

---

## 3. Stale Wbudowane

### Typy Aktorow:
```python
debugRender.TYPE_ENEMY      # 0 - Potwory / moby
debugRender.TYPE_NPC        # 1 - NPC handlarze, misje
debugRender.TYPE_STONE      # 2 - Kamienie Metin
debugRender.TYPE_WARP       # 3 - Teleporty / portale
debugRender.TYPE_DOOR       # 4 - Bramy / drzwi
debugRender.TYPE_BUILDING   # 5 - Budynki
debugRender.TYPE_PC         # 6 - Gracze
debugRender.TYPE_POLY       # 7 - Potwory z polymorphii
debugRender.TYPE_HORSE      # 8 - Konie / wierzchowce
```

### Gotowa Paleta Kolorow ARGB:
```python
debugRender.COLOR_RED       # 0xFFFF0000 - Czerwony (Wrogowie, HP krytyczne)
debugRender.COLOR_GREEN     # 0xFF00FF00 - Zielony (Sojusznicy, strefy bezpieczne)
debugRender.COLOR_BLUE      # 0xFF0000FF - Niebieski
debugRender.COLOR_YELLOW    # 0xFFFFFF00 - Zolty (Metiny, cele priorytetowe)
debugRender.COLOR_CYAN      # 0xFF00FFFF - Jasnobiekitny (Zasieg wlasny)
debugRender.COLOR_MAGENTA   # 0xFFFF00FF - Fioletowy (Wrodzy gracze PvP)
debugRender.COLOR_WHITE     # 0xFFFFFFFF - Bialy (Trasy, waypointy)
debugRender.COLOR_ORANGE    # 0xFFFFA500 - Pomaranczowy (Tracery X-Ray)
```

---

## 4. Kompletny Przyklad Modulu ESP do `game.py`

Ponizszy przyklad przedstawia pelna implementacje klasy pomocniczej `BotVisualizer`, ktora mozna podpiac w metodzie `GameWindow.OnUpdate()`:

```python
import debugRender
import player
import chr

class BotVisualizer(object):
    def __init__(self):
        self.enabled = True
        self.attackRadius = 1200.0
        self.scanRadius = 3500.0

    def Toggle(self):
        self.enabled = not self.enabled
        debugRender.Enable(self.enabled)
        if not self.enabled:
            debugRender.Clear()

    def OnUpdate(self):
        if not self.enabled:
            return

        # 1. Czyszczenie geometrii z poprzedniej klatki
        debugRender.Clear()

        # 2. Rysowanie zasiegu ataku wokol gracza (promien 1200 na ziemi)
        mainVid = player.GetMainCharacterIndex()
        if mainVid != 0:
            debugRender.DrawCircleActor(mainVid, self.attackRadius, debugRender.COLOR_CYAN, False, 32)

        # 3. Oznaczanie kamieni Metin w promieniu skanowania (Zolte ramki + Tracery X-Ray)
        metinVids = debugRender.GetActorsInRadius(self.scanRadius, debugRender.TYPE_STONE, False)
        for vid in metinVids:
            debugRender.DrawBox(vid, debugRender.COLOR_YELLOW, False)
            debugRender.DrawTracer(vid, debugRender.COLOR_YELLOW, True)

        # 4. Oznaczanie agresywnych mobow (Czerwone ramki)
        mobVids = debugRender.GetActorsInRadius(self.attackRadius, debugRender.TYPE_ENEMY, False)
        for vid in mobVids:
            debugRender.DrawBox(vid, debugRender.COLOR_RED, False)

        # 5. Oznaczanie innych graczy (Fioletowe boxy i pomaranczowe tracery)
        playerVids = debugRender.GetActorsInRadius(self.scanRadius, debugRender.TYPE_PC, False)
        for vid in playerVids:
            debugRender.DrawBox(vid, debugRender.COLOR_MAGENTA, False)
            debugRender.DrawTracer(vid, debugRender.COLOR_ORANGE, True)
```

Wpiecie w `game.py`:
```python
# W GameWindow.__init__:
self.visualizer = BotVisualizer()

# W GameWindow.OnUpdate:
if hasattr(self, "visualizer"):
    self.visualizer.OnUpdate()

# W obsludze klawisza (np. F10 lub klawisza tyldy):
if app.IsPressed(app.DIK_F10):
    self.visualizer.Toggle()
```
