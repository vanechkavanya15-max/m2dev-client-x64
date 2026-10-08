# ARCHITECTURE.md - Standard Architektury Silnika Klienta Gry 2026

Ten dokument jest glownym zrodlem prawdy (Single Source of Truth) dla wszystkich inzynierow i Agentow AI (Antigravity, Google Jules, roje Swarm) pracujacych w repozytorium klienta.

---

## 1. Warstwy Architektoniczne Silnika (Clean / Onion Architecture)

Przeplyw zaleznosci jest scisle jednokierunkowy:

```
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA PREZENTACJI & GUI (src/UserInterface/)                         │
│ • Okna Pythona, widoki, rendering DirectX 9 / D3D9                     │
│ • Cienkie fasady: PythonPlayer, PythonCharacterManager, PythonNetworkStream │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ (zalezy wylacznie od domen)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA DOMEN STANOWYCH C++23 (src/Client/Gameplay, World, Network)     │
│ • Czysta logika biznesowa, reguly gry, ekwipunek, statystyki, kodeki   │
│ • ZERO PYTHONA, ZERO DIRECTX, ZERO HWND                                │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ (zalezy wylacznie od rdzenia)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ WARSTWA RDZENIA & FUNDAMENTOW (src/EterBase/, src/Client/Core/)        │
│ • Silne typy (EntityVid, ItemVnum), PacketResult<T>, Math SIMD, Mutexy │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Pelny Przeplyw Pakietu Sieciowego

```
[ Serwer Gry (TCP) ]
         │
         ▼
[ CPythonNetworkStream (Gniazdo Asynchroniczne) ]
         │
         ▼
[ PacketDispatcher / PhaseGameBridge (Mostek GUI) ]
         │ (Przekazuje std::span<const uint8_t>)
         ▼
[ Client::Network::*PacketCodec (Czysta Deserializacja C++23) ]
         │ (Zwraca EterBase::PacketResult<TPacketData>)
         ▼
[ Client::Gameplay / Client::World (Aktualizacja Stanu Domenowego) ]
         │ (Bezpieczna mutacja chroniona std::shared_mutex)
         ▼
[ UserInterface::Core::EventBus (Rozgloszenie Zdarzenia) ]
         │ (Powiadomienie subskrybentow)
         ▼
[ Python UI / CPythonPlayer (Odswiezenie Elementow Okna Gry) ]
```

---

## 3. Standardy Inzynierskie i Zasady C++23 dla Agentow AI

1. **Czyste Nazewnictwo (Clean Naming):**
   - Zakaz notacji wegierskiej (`m_kVct_pkInstAlive` -> `m_aliveInstances`, `dwVID` -> `vid`).
2. **Silne Typowanie (Strong Types):**
   - Zakaz uzywania surowego `DWORD` dla identyfikatorow:
     - Identyfikator encji w swiecie -> `EntityVid`
     - Numer prototypu przedmiotu -> `ItemVnum`
     - Indeks slotu w ekwipunku -> `SlotIndex`
     - Identyfikator czaru -> `SkillIndex`
3. **Semantyka Wyniku (`Result<T, Error>`):**
   - Metody domenowe zwracaja `EterBase::PacketResult<T>` lub `std::optional<T>` z precyzyjnym enumem bledu zamiast -1 czy nullptr.
4. **Wstrzykiwanie Zaleznosci (Dependency Injection):**
   - Klasy przyjmuja referencje do potrzebnych domen przez konstruktor, umozliwiajac testy jednostkowe w pamieci RAM (in-memory) bez uruchamiania okna gry czy karty graficznej.
5. **Kompilacja i Testy:**
   - Kompilacja ze wsparciem cache: `sccache` zintegrowany w `CMakeLists.txt`.
   - Polecenie budowania: `cmake --build build --config Release --target Client2026 UserInterface --parallel`.
   - Weryfikacja: `ctest -C Release --output-on-failure` (100% PASS wymagane przed zatwierdzeniem kodu).
