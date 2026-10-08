# Modul: UserInterface::Network (Mostki GUI i Dyspozytor)
## Status: Warstwa Fasady Sieciowej (Strangler Bridge)

### 1. Przeznaczenie i Odpowiedzialnosc
Modul laczy strumien sieciowy gry (`CPythonNetworkStream`) z wyspecjalizowanymi kodekami domenowymi C++23 oraz interfejsem graficznym Pythona:
- 12 Mostkow GUI (`PhaseGame*Bridge`), ktore odbieraja zdeserializowane pakiety i przekazuja je do odpowiednich okien Pythona.
- Redukcja monolitu `PythonNetworkStreamPhaseGame.cpp` do cienkiego dyspozytora tabelarycznego.

---

### 2. Zelazne Reguly Architektoniczne
1. **Wzorzec Strangler Facade:** Mostek odbiera surowy bufor, wywoluje kodek z `Client::Network::*PacketCodec`, sprawdza `Result<T>` i tylko przy sukcesie wywoluje metody Pythona lub menedzerow gry.
2. **Dekoracja Friend Class:** Kazdy mostek deklarowany jest jako `friend class PhaseGame*Bridge;` w `PythonNetworkStream.h`.
3. **Czystosc Wywolan:** Zadna logika matematyczna ani deserializacja pakietow nie moze byc zduplikowana wewnatrz mostka – calosc delegowana do kodeka C++23.

---

### 3. Eksportowane Mostki GUI
- `PhaseGameGuildBridge`
- `PhaseGamePartyBridge`
- `PhaseGameQuestBridge`
- `PhaseGameShopBridge`
- `PhaseGameExchangeBridge`
- `PhaseGameSkillsBridge`
- `PhaseGameTargetBridge`
- `PhaseGameCombatBridge`
- `PhaseGameWorldBridge`
- `PhaseGameRefineBridge`
- `PhaseGameChatBridge`
- `PhaseGameSyncBridge`
