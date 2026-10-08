# Modul: Client::World
## Status: Czysta Domena Swiata i Przestrzeni C++23

### 1. Przeznaczenie i Odpowiedzialnosc
Modul zarzadza tozsamoscia, cyklem zycia oraz polozeniem przestrzennym encji w swiecie gry:
- Rejestr encji `EntityVid -> ActorRecord` ze statusem zywy/martwy (`ActorRegistry`).
- Dwuwymiarowy indeks komorkowy O(1) dla zapytan zasiegu radaru i najblizszego wroga (`SpatialHashGrid`).
- Zero powiazan ze strukturami siatek 3D czy modelami Granny.

---

### 2. Zelazne Reguly Architektoniczne
1. **Zero Grafiki:** Brak struktur `CInstanceBase`, brak macierzy widoku kamery, brak DirectX.
2. **Indeks Przestrzenny O(1):** Wszystkie zapytania o sasiedztwo realizowane przez `QueryRadius` lub `QueryNearest` z komorkami 256x256.
3. **Strong Types:** Wszystkie identyfikatory encji uzywaja silnego typu `EterBase::EntityVid`.
4. **Thread-Safety:** Mutexy `std::shared_mutex` zabezpieczaja wspolbiezne aktualizacje pozycji z pakietow sieciowych.

---

### 3. Eksportowane Klasy i Interfejsy
- `Client::World::ActorRegistry` (`ActorRecord`, `SetMainActorVid`, `IsAlive`, `IsDead`)
- `Client::World::SpatialHashGrid` (`Insert`, `Update`, `Remove`, `QueryRadius`, `QueryNearest`)

---

### 4. Przypisane Testy Jednostkowe
- `tests/test_c26_actor_registry.cpp`
- `tests/test_c26_spatial_hash_grid.cpp`
