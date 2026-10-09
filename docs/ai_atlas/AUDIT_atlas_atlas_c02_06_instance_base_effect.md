---
task_id: "atlas_c02_06_instance_base_effect"
cluster: "ACT"
module_name: "CInstanceBase - Wizualne Efekty Instancji i Aury"
target_files:
- src/UserInterface/InstanceBaseEffect.cpp
- src/UserInterface/InstanceControllers/InstanceEffectControllerImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_06_instance_base_effect.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul jest odpowiedzialny za zarzadzanie wizualnymi efektami przypisanymi do instancji (postaci/obiektow) w grze. Obejmuje to doczepianie efektow czasteczkowych (bony, aury, swiecenia broni, statusy jak trucizna czy podpalenie) do konkretnych kosci w modelu postaci, a takze aktualizacje tych efektow w zaleznosci od stanu gry.
Architektura w kliencie 2026 r. ewoluowala i zrezygnowala z monolitycznego pliku `InstanceEffectControllerImpl.cpp` na rzecz zestawu odizolowanych pod-kontrolerow realizujacych konkretne efekty (SRP), takich jak `InstanceEffect_ArmorShine.cpp`, `InstanceEffect_Aura.cpp`, `InstanceEffect_BuffVisual.cpp`, itp., dziedziczacych po interfejsie `IInstanceEffectController`.

Przeplyw danych:
1. Obiekty lub systemy zewnetrzne wolaja metody interfejsu (np. `AttachBoneEffect`, `SetSwordAura`).
2. Implementacje pod-kontrolerow (np. `InstanceEffect_Aura`) zarzadzaja przypisywaniem efektu do tzw. handle'a.
3. Kontroler nastepnie wysyla zdarzenia z uzyciem systemu `EventBus` (np. `BoneEffectAttachedEvent`, `AuraStateChangedEvent`).
4. Rejestrowanie efektow i ich renderowanie oddzielone jest od logiki stanu za pomoca eventow.

Cykl zycia:
- Alokacja kontrolerow nastepuje zazwyczaj za pomoca funkcji fabryk (np. `CreateInstanceEffectController()`, `CreateArmorShineController()`) zwracajacych `std::unique_ptr<IInstanceEffectController>`.
- Zarzadzanie zasobami (handle dla efektow) odbywa sie lokalnie przy uzyciu map `std::unordered_map`. Zwalnianie (resetowanie i dealokacja) wymuszane jest poprzez `ClearAllEffects()` oraz destruktor.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):**
  - Wywolania z warstwy postaci (np. `CInstanceBase`).
  - System obslugi zdarzen walki (CombatEvents) - szczegolnie `InstanceEffect_SkillHit` dziala reaktywnie nasluchujac `Core::CombatEvents::ActorDamaged`.
  - Serwerowe pakiety aktualizujace stany instancji i buffy.
- **Zaleznosci wyjsciowe (Outbound):**
  - `Core::EventBus` - do rozglaszania zdarzen jak `BoneEffectAttachedEvent`, `ArmorShineSetEvent`, `WeaponGlowUpdateEvent`.
  - Narzedzia EterBase (logowanie `EterBase::ModernLogger`, typy silne `EterBase::SkillId`, bledy `EterBase::PacketResult`).
- **Drzewo dyrektyw `#include`:**
  - Standardowe biblioteki (m.in. `<memory>`, `<unordered_map>`, `<mutex>`, `<cstdint>`, `<string_view>`, `<vector>`, `<expected>`, `<format>`).
  - `IInstanceEffectController.h` z glownego folderu modulow InstanceControllers.
  - Ostrzezenie o cyklicznosci: Wykorzystanie EventBusa znaczaco ogranicza powstawanie cyklicznych zaleznosci.
- **Model pamieciowy:**
  - Podstawowe zarzadzanie polimorficznymi obiektami nastepuje z uzyciem `std::unique_ptr`.
  - Wskazniki surowe stosuje sie minimalnie (w `CreateSkillHitEffectController` zwracany z eksportu C, ale obslugiwany po stronie wywolujacego).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `IInstanceEffectController` (Rozmiar: ~8 byte ptr do vtable): Interfejs glowny zarzadzania efektami, thread-owner to glownie watek logiki gry.
- `EffectAttachData` (Rozmiar: 16+ bajtow w zaleznosci od string_view): Struktura przetrzymujaca ID efektu, nazwe kosci, skale i flage zapetlenia.
- Zdarzenia `EventBus`: `BoneEffectAttachedEvent`, `AuraStateChangedEvent`, `ArmorShineSetEvent`, `BuffVisualStateChangedEvent`, `SkillHitEffectCreatedEvent`, itp. sluza wylacznie jako nosniki danych DTO.

**Tabela Metod Publicznych (`IInstanceEffectController`):**
- `EterBase::PacketResult<uint32_t> AttachBoneEffect(const EffectAttachData& data)`: Podczepia efekt. Zwraca unikalny handle (ID przypisania).
- `EterBase::PacketResult<void> DetachEffect(uint32_t handle)`: Usuwa aktywne przypisanie. Oczekuje wczesniejszego AttachBoneEffect.
- `void SetSwordAura(bool active, uint32_t auraType)`: Kontroluje aure broni.
- `void SetBuffVisual(uint32_t buffId, bool active)`: Aktywuje / dezaktywuje wizualne efekty buffow.
- `void SetStatusEffect(uint8_t statusFlag, bool active)`: Aktualizuje efekty nakladane na cel (trucizna, zamrozenie).
- `void SetItemShine(uint8_t partIndex, uint8_t refineLevel)`: Aktualizuje wizualne swiecenie ulepszonego sprzetu (wymaga refineLevel >= 7).
- `void ClearAllEffects()`: Usuwa wszystkie efekty podczepione pod dany kontroler.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- `EffectAttachData`:
  - `uint32_t effectId` (offset: 0)
  - `std::string_view boneName` (offset: 8)
  - `float scale` (offset: 24)
  - `bool isLooping` (offset: 28)
  -(Wartosci moga sie nieznacznie roznic w zaleznosci od paddingu kompilatora i wielkosci std::string_view)

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Kod jest oddzielony od sieci, nie uzywa wprost struktur sieciowych. Modul dostosowuje sie do obslugi rozpakowanych danych dostarczanych przez pakiety GC (np. pakiety wlasciwosci itemu lub stany buffow).
- **Metody Pythona (`PyMethodDef`):** Moduly unikaja bezposrednich zaleznosci od Pythona, przestrzegajac zasady Zero-Conflict C++23. Ewentualne mosty realizuje warstwa PythonNetworkStream lub inne dedykowane systemy Python->C++.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Funkcje EventBus wywolywane z kontrolerow dzialaja synchronicznie lub na glownym watku logiki. Implementacja `InstanceEffect_Bone` wykorzystuje `std::mutex` dla blokowania dostepu do `m_activeEffects`.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  - Wywolywanie zdarzen czyszczacych efekty z petli iterujacej po strukturze (problem invalidacji iteratorow). Implementacja np. `InstanceEffect_Aura` uzywa wektora pomocniczego, by tego uniknac.
  - Wywolywanie `AttachBoneEffect` bez nazwy kosci zwraca blad w walidacji parametru wejsciowego (`EffectAttachData::boneName.empty()`).
- **Zarzadzanie zasobami (RAII):** Efekty sa podpiete pod obiekty kontrolerow, dlatego niszczenie kontrolera (`~IInstanceEffectController()`) wymusza wewnetrzne `ClearAllEffects()`, co zabezpiecza przed wiszacymi w pamieci uchwytami.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Stworz nowy plik `InstanceEffect_[NowaFunkcja].cpp` implementujacy interfejs `IInstanceEffectController`.
  2. Dodaj lokalne wewnetrzne zdarzenia dziedziczace z `Core::IEvent`.
  3. Zaimplementuj metody (np. `AttachBoneEffect`), a nieobslugiwane zignoruj. Zawsze wywoluj `EventBus::Publish` z nowymi stanami, by zachowac separacje logiki od UI/grafiki.
  4. Dodaj fabryke zwracajaca nowy system `std::unique_ptr`.
- **Jak debugowac i logowac:** Wykorzystuj wylacznie `EterBase::ModernLogger::Info/Debug/Warn/Error`. Nasluchuj zdarzen wysylanych do szyny `EventBus`, zeby zbadac reakcje subskrybentow.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W zwiazku z separacja przez zdarzenia (Events), uzywaj `doctest`. Subskrybuj szydelkowo na EventBus (`Core::EventBus::GetInstance().Subscribe`), wymuszaj operacje kontrolera i sprawdzaj czy event na szynie odnotowal wlasciwy stan i handle, zachowujac srodowisko odizolowane od bibliotek GUI/DirectX.
