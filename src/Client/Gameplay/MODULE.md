# Modul: Client::Gameplay
## Status: Czysta Domena C++23 (Warstwa Domenowa)

### 1. Przeznaczenie i Odpowiedzialnosc (Single Responsibility Principle)
Ten modul odpowiada wylacznie za reguly biznesowe i stan postaci gracza:
- Ekwipunek, siatka slotow, blokady i zapobieganie przepelnieniu (`InventoryDomain`).
- Paski szybkiego dostepu (Quickslot 1-4, F1-F4) (`QuickslotDomain`).
- Statystyki bazowe, punkty statusu, HP/SP/EXP (obsluga 64-bit) (`PlayerStatsDomain`).
- Poziomy umiejetnosci i progi many (`SkillDomain`).
- Czasy odnowienia z interfejsem chrono (`SkillCooldownTracker`).
- Kumulacja bonusow i socketow kamieni dusz (`ItemBonusCalculator`).

---

### 2. Zelazne Reguly Architektoniczne (Architectural Guards)
1. **Zero Pythona:** Bezwzgledny zakaz uzywania `PyObject*`, `Py_BuildValue`, naglowkow Pythona.
2. **Zero DirectX / GPU:** Zakaz struktur `D3D9`, `LPDIRECT3DDEVICE9`, `D3DXVECTOR3`.
3. **Czyste Typy Standardowe:** Uzywaj wylacznie `std::optional`, `std::span`, `std::array`, `StrongTypes` (`EntityVid`, `ItemVnum`, `SlotIndex`), `int64_t`.
4. **Bezpieczenstwo Watkowe:** Kazda klasa przechowujaca stan posiada `mutable std::shared_mutex` chroniacy rownolegly dostep z watku sieciowego i renderera.
5. **Obsluga Bledow:** Zwracaj `std::optional<T>` lub `EterBase::PacketResult<T>` zamiast kodow numerycznych -1 czy nullptr.

---

### 3. Eksportowane Klasy i Interfejsy
- `Client::Gameplay::InventoryDomain`
- `Client::Gameplay::QuickslotDomain`
- `Client::Gameplay::PlayerStatsDomain`
- `Client::Gameplay::SkillDomain`
- `Client::Gameplay::SkillCooldownTracker`
- `Client::Gameplay::ItemBonusCalculator`

---

### 4. Przypisane Testy Jednostkowe (In-Memory, Zero Mockow)
- `tests/test_c26_inventory_domain.cpp`
- `tests/test_c26_quickslot_domain.cpp`
- `tests/test_c26_player_stats_domain.cpp`
- `tests/test_c26_skill_domain.cpp`
- `tests/test_c26_skill_cooldown.cpp`
- `tests/test_c26_item_bonus.cpp`
