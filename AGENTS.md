# INSTRUKCJE DLA AGENTOW AI (JULES SWARM)

## 1. ZASADY GLOWNE I PRIORYTETY
1. **GRAFIKI NIE RUSZAMY NA RAZIE:** Zgodnie z wytycznymi, podsystemy graficzne (DirectX, renderery, modele) pozostaja nienaruszone.
2. **AUDIOLIB WYTNIETY:** `AudioLib` zostal zastapiony lekka, bezkosztowa atrapa (stub). Zadne moduly dzwieku (miniaudio, Miles) nie sa kompilowane ani inicjalizowane.
3. **GLOWNY CEL NA ROK 2026:** Porzadek, modularyzacja, refaktoryzacja do standardu C++20 oraz stworzenie **maksymalnej czytelnosci dla agentow AI** w kodzie sieci, logiki i stanu gry.

---

## 2. KOMPENDIUM WIEDZY: KATALOG `docs/`
W katalogu `docs/` znajduje sie kompletna dekonstrukcja architektury klienta:
* `docs/CLIENT_SYSTEM_MAP.md` - centralna mapa systemu, cykl zycia i petla glowna.
* `docs/01_architecture/` - gleboka analiza warstwowa i integracja z serwerem.
* `docs/02_subsystems/` - dekonstrukcja bibliotek pomocniczych.
* `docs/03_network_protocol/`:
  * `01_network_stream_core_handshake.md` - handshake, time-sync, ping-pong.
  * `02_phases_login_select_loading.md` - przebieg faz logowania i wyboru postaci.
  * `03_phase_game_actors_items.md` - obsluga ruchu, encji i przedmiotow.
  * `04_packets_cg_client_to_server.md` - KOMPLETNY rejestr pakietow wychodzacych.
  * `05_packets_gc_server_to_client.md` - KOMPLETNY rejestr pakietow przychodzacych.
* `docs/04_python_bindings/` - powiazania C++ do modulow Pythona (`player`, `chrmgr`, `item`).

ZANIM zaczniesz zmieniac dany pakiet lub modul, przeczytaj odpowiedni plik w `docs/`!

---

## 3. KLUCZOWA PRAWDA ARCHITEKTONICZNA: STAN GRY ZYJE W C++
Python w kliencie Metin2 jest **wylacznie warstwa widoku/interfejsu (View)**. Caly krytyczny stan gry zyje w czystym kodzie C++:
* **`CPythonPlayer`** -> przechowuje tablice ekwipunku, aktualne HP/MP/EXP, sloty umiejetnosci i cele ataku.
* **`CPythonCharacterManager`** -> trzyma instancje wszystkich potworow, graczy i NPC w zasiegu widzenia (`CInstanceBase`).
* **`CPythonItem`** -> zarzadza przedmiotami lezacymi na ziemi.

Oznacza to, ze bot i agenci AI operuja bezposrednio na strukturach C++, bez koniecznosci polegania na skryptach graficznych UI.

---

## 4. STANDARDY MODERNIZACJI NA ROK 2026 (STANDARD C++23 & AI READABILITY)

Projekt dziala w pelnym standardzie **C++23** (MSVC 2022 x64, CMake). Wszyscy agenci Jules Swarm musza przestrzegac nastepujacych zasad:

### A. Eliminacja wskaznikow wyjsciowych (std::expected<T, E>)
Nigdy nie piszemy funkcji w starym stylu: `bool Parse(Data* out)`.
Wszystkie nowe metody i handlery zwracaja scisle `std::expected` lub `EterBase::PacketResult<T>`:
```cpp
// PRAWIDLOWO (Standard 2026 C++23):
EterBase::PacketResult<void> ProcessDamagePacket(std::span<const uint8_t> buffer);
```
Dla modelu LLM/AI typ zwracany niesie albo pelne dane, albo precyzyjny powod bledu (`PacketError::BufferUnderflow`, `InvalidHeader`). Zero domyslow i zero ukrytych efektow ubocznych!

### B. Likwidacja piramid "if" (Monadyczny std::optional)
Zamiast 5 zagniezdzen `if (pActor) { if (pItem) { if (pSkill) ... } }`, uzywamy czystych potokow:
```cpp
auto weaponVnum = EntityManager::Find(vid)
    .and_then(&Actor::GetEquipment)
    .transform(&Equipment::GetWeaponVnum)
    .value_or(ItemVnum(0));
```
Oszczedza to tokeny kontekstu i wyklucza bledy brzegowe przy analizie kodu przez agentow AI.

### C. Silne Typy Domenowe (StrongTypes.h)
Zakaz uzywania generycznego `uint32_t` na wszystko. Uzywaj typow z `src/EterBase/StrongTypes.h`:
* `EntityId` -> identyfikator aktora, moba, gracza (VID).
* `ItemVnum` -> numer typu przedmiotu (VNUM).
* `ItemSlot` -> indeks slotu w ekwipunku/depozycie.
* `SkillId` -> numer umiejetnosci.
Dzieki temu kompilator fizycznie uniemozliwia pomylenie VID celu z VNUM miecza.

### D. Bezpieczne Formatowanie Pamieci (std::format i ModernLogger)
Calkowity zakaz uzywania niebezpiecznych funkcji `sprintf`, `vsprintf` i buforow `char[1024]`.
Stosujemy `std::format` oraz `EterBase::ModernLogger::Info(...)`, `ModernLogger::Error(...)`.

### E. Wielowymiarowy operator dostepu operator[](x, y)
Dla siatek kolizji i map (`CollisionGrid`, `MapAttr`) uzywamy C++23: `grid[x, y]` zamiast starego `grid[y * width + x]`.

---

## 5. ZASADY PRACY W REPOZYTORIUM
* Ignorujemy `m2dev-client-main/`, `build/`, `bin/`.
* Kazdy agent tworzy lub modyfikuje swoj dedykowany modul bez kolizji z innymi agentami (zasada Zero-Conflict).
* Przed implementacja pakietu ZAWSZE sprawdz specyfikacje w `docs/03_network_protocol/04_packets_cg_client_to_server.md` oraz `05_packets_gc_server_to_client.md`.
