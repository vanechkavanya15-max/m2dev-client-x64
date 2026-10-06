# DOKUMENTACJA ARCHITEKTURY I STRUKTURY KODU (STANDARD 2026)
## Przewodnik dla Agentow AI i Deweloperow

Niniejszy dokument opisuje organizacje kodu zrodlowego w repozytorium `src/`, przeplyw danych oraz standardy modernizacji kodu do C++20.

---

## 1. MAPA MODUŁOW I ODPOWIEDZIALNOSCI

```
src/
├── AudioLib/          -> [STUBBED] Zastapione lekka atrapa zero-cost (brak miniaudio, brak dzwiekow).
├── EterBase/          -> Narzedzia systemowe, zarzadzanie pamiecia, kompresja LZO, szyfrowanie TEA/Sodium.
├── EterLib/           -> Klasy sieciowe bazowe (NetStream, NetAddress, NetDevice), kolejki pakietow.
├── GameLib/           -> Matematyka i encje gry (ActorInstance, ItemData, ItemManager, Fly, Property).
├── EterPythonLib/     -> Integracja interpretera Pythona (PythonLauncher, PythonSlotWindow, UI helpers).
└── UserInterface/     -> Glowna logika klienta Metin2:
    ├── UserInterface.cpp                 -> Punkt wejscia WinMain / main, inicjalizacja modulow.
    ├── PythonApplication.cpp / .h        -> Glowna petla aplikacji, obsluga czasu, okna i aktualizacji.
    ├── Packet.h / BeaviumProtocol.h      -> Definicje struktur pakietow sieciowych (naglowki, rozmiary).
    ├── PythonNetworkStream.cpp / .h      -> Glowny strumien sieciowy klienta (zarzadca polaczenia TCP).
    ├── PythonNetworkStreamPhase*.cpp     -> Handlery pakietow w zaleznosci od fazy polaczenia:
    │   ├── PhaseHandshake.cpp            -> Negocjacja kluczy, faza poczatkowa.
    │   ├── PhaseLogin.cpp                -> Autoryzacja i logowanie na konto.
    │   ├── PhaseSelect.cpp               -> Wybor i tworzenie postaci.
    │   ├── PhaseLoading.cpp              -> Ladowanie swiata, koordynatow i mapy.
    │   ├── PhaseGame.cpp                 -> Glowna rozgrywka: czat, punkty, skille, gildia, handel.
    │   ├── PhaseGameActor.cpp            -> Pojawianie sie mobow/graczy, poruszanie sie encji.
    │   └── PhaseGameItem.cpp             -> Przedmioty w ekwipunku i na ziemi.
    ├── PythonPlayer.cpp / .h             -> Stan lokalnego gracza (HP, MP, EXP, koordynaty, cel ataku).
    ├── PythonCharacterManager.cpp / .h   -> Menedzer wszystkich postaci i potworow w zasiegu wzroku.
    ├── PythonItem.cpp / .h               -> Menedzer itemow lezacych na ziemi i w ekwipunku.
    └── BotApi.h                          -> Nowoczesny interfejs programistyczny C++20 dla bota.
```

---

## 2. PRZEPLYW DANYCH W ARCHITEKTURZE (DATA FLOW)

### A. Odbior pakietu z serwera (Incoming Packet):
1. **Gniazdo TCP:** Dane trafiaja do bufora w `CNetworkStream` (`EterLib`).
2. **Petla aktualizacji:** `CPythonApplication::Update()` -> `CPythonNetworkStream::Process()`.
3. **Dispatcher Fazy:** `CPythonNetworkStream::GamePhase()` czyta naglowek (header) i rozmiar z `Packet.h`.
4. **Handler logiczny:** Wywolanie dedykowanej metody `Recv*()`, np.:
   * `RecvCharacterAppendPacket()` -> tworzy nowa encje w `CPythonCharacterManager`.
   * `RecvItemSetPacket()` -> aktualizuje slot w `CPythonPlayer` / `CPythonItem`.
   * `RecvPointChange()` -> aktualizuje HP/MP gracza.
5. **Stan gry:** Dane zostaja zapisane w pamieci klienta. *(Uwaga: wycinamy zaleznosci probujace wywolywac okienka GUI Pythona)*.

### B. Akcja bota i wyslanie pakietu (Outgoing Action):
1. **Decyzja bota:** Logika bota lub agent AI wywoluje metode z `BotApi.h` (np. `AttackVid(targetVid)` lub `MoveTo(x, y)`).
2. **Serializacja pakietu:** Metoda `CPythonNetworkStream::Send*Packet()` wypelnia strukture z `Packet.h`.
3. **Kolejka gniazda:** Pakiet jest szyfrowany i dodawany do kolejki wychodzacej w `CNetworkStream`.
4. **Wysylka TCP:** Dane trafiaja do serwera gry.

---

## 3. ZASADY CZYSTOSCI I MODERNIZACJI KODU (C++20 - ROK 2026)

Dla zachowania maksymalnej czytelnosci dla agentow AI stosujemy nastepujace reguly:

### Regula 1: Zamiana archaicznych typow danych
* Zamiast `DWORD` -> uzywamy `uint32_t`.
* Zamiast `WORD`  -> uzywamy `uint16_t`.
* Zamiast `BYTE`  -> uzywamy `uint8_t`.
* Zamiast `BOOL`  -> uzywamy `bool` (oraz `true`/`false` zamiast `TRUE`/`FALSE`).
* Zamiast `LONGLONG` / `__int64` -> uzywamy `int64_t` / `uint64_t`.

### Regula 2: Likwidacja notacji wegierskiej w nowym i refaktoryzowanym kodzie
* Zamiast `pkInstMain` -> `mainInstance` lub `mainPlayer`.
* Zamiast `dwTargetVID` -> `targetVid`.
* Zamiast `c_szName` -> `name` (najlepiej `std::string_view` lub `const std::string&`).
* Zamiast `m_kMap_dwVID_pkInst` -> `m_instancesByVid`.

### Regula 3: Izolacja logiki sieciowej od GUI (Decoupling)
W starym kodzie wystepowaly wywolania typu:
```cpp
// ZLE (STARY STYL 2004 - wymusza GUI i wywoluje crashe bez okienek):
PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAlignment", Py_BuildValue("()"));
```
W nowoczesnym kodzie headless:
```cpp
// DOBRZE (STANDARD 2026):
// Aktualizujemy wylacznie stan w CPythonPlayer, a powiadomienie GUI wywolujemy
// tylko wtedy, gdy wskaznik na okno faktycznie istnieje:
if (m_apoPhaseWnd[PHASE_WINDOW_GAME]) {
    PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAlignment", Py_BuildValue("()"));
}
```

### Regula 4: Wykorzystanie nowoczesnych mechanizmow C++23
* Uzywac `std::format` i `EterBase::ModernLogger` do logowania i skladania lancuchow znakow zamiast niebezpiecznego `sprintf` / `TraceError`.
* Uzywac `std::string_view` do niemodyfikowalnych tekstow oraz `std::span` do buforow bajtow pakietow.
* Stosowac `std::to_underlying` dla silnych enumow pakietowych.

### Regula 5: Eliminacja wskaznikow wyjsciowych (std::expected<T, E>)
* Kazda funkcja odczytujaca pakiety lub weryfikujaca stan zwraca `std::expected` (np. `EterBase::PacketResult<void>`).
* Calkowity zakaz wzorca `bool Read(Data* out)`. Typ zwracany niesie albo pelna wartosc, albo jednoznaczny kod bledu.

### Regula 6: Likwidacja piramid if (Monadyczny std::optional)
* Zamiast 5 zagniezdzen `if (pActor) { if (pItem) ... }`, uzywamy potokow `.and_then()`, `.transform()`, `.value_or()`.

### Regula 7: Silne Typy Domenowe (StrongTypes.h)
* Zamiast archaicznego i podatnego na halucynacje AI `uint32_t`, stosujemy:
  * `EntityId` (dla VID aktora/moba)
  * `ItemVnum` (dla identyfikatora przedmiotu)
  * `ItemSlot` (dla indeksu slotu)
  * `SkillId` (dla identyfikatora skilla)
* Kompilator MSVC na poziomie typow uniemozliwia pomylenie VID celu z VNUM miecza.

### Regula 8: Wielowymiarowy operator dostepu operator[](x, y)
* W klasach siatek kolizji i map (`CollisionGrid`) uzywamy C++23 `grid[x, y]` zamiast archaicznego `grid[y * width + x]`.

---

## 4. STATUS BIBLIOTEKI DZWIEKOWEJ (AUDIOLIB)
* Modul `AudioLib` zostal w pelni zastapiony atrapa zero-cost (stub).
* `miniaudio.c` i `miniaudio.h` (4 MB) zostaly wykluczone.
* Wszystkie wywolania `SoundEngine::Instance().Play*` sa natychmiastowymi operacjami pustymi (no-op).
* Zero narzutu na watek dzwieku, pamiec RAM i procesor.
