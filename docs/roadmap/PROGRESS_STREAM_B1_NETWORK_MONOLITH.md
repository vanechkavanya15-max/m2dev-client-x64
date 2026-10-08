# WORKBOOK POSTEPU: STRUMIEN B1 - ROZBICIE MONOLITU SIECIOWEGO (NETWORK MONOLITH)
## Odpowiedzialny: Agent Lead B1 (Network Architecture Specialist)

**Cel Strumienia:** Rozbicie 4 234-liniowego monolitu `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` na hermetyczne, niezalezne klasy pakietowe w standardzie C++23 w katalogu `src/Client/Network/Handlers/`.  
**Zasada:** 1 pakiet = 1 para plikow (.h / .cpp) = 1 test jednostkowy. Budzet modulu: 100-300 linii.  
**Zelazna Zasada:** ZERO wywolan Pythona i GUI! Kazdy handler aktualizuje wylacznie `WorldContext` i emituje event do `EventBus`.

---

## 1. DOKLADNA MAPA MONOLITU DO USUNIECIA / ZASTAPIENIA

Stary plik: `src/UserInterface/PythonNetworkStreamPhaseGame.cpp` (4 234 linie)  
Powiazany naglowek: `src/UserInterface/PythonNetworkStream.h`  
Nowy katalog docelowy: `src/Client/Network/Handlers/`

### Standardowy Wzorzec Handlera C++23 dla Robotnikow Jules:
```cpp
// Przyklad wzorcowy: src/Client/Network/Handlers/ActorSpawnHandler.h
#pragma once
#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "Client/Core/WorldContext.h"

namespace Client::Network::Handlers {

class ActorSpawnHandler {
public:
    [[nodiscard]] static EterBase::PacketResult<void> Process(
        std::span<const uint8_t> payload,
        Client::Core::WorldContext& worldCtx
    );
};

} // namespace Client::Network::Handlers
```

---

## 2. REJESTR ZADAN ATOMOWYCH (SWARM TASK LIST)

| ID | Status | Nazwa Zadania / Pakiet | Plik Handlera C++23 | Plik Testu Jednostkowego | Przypisany Agent |
|---|---|---|---|---|---|
| **B1-01** | [x] | `GC_CHARACTER_ADD` / `SPAWN` | `ActorSpawnHandler.h/.cpp` | `test_c26_actor_spawn_handler.cpp` | Jules Worker #01 |
| **B1-02** | [x] | `GC_CHARACTER_UPDATE` | `ActorUpdateHandler.h/.cpp` | `test_c26_actor_update_handler.cpp` | Jules Worker #02 |
| **B1-03** | [x] | `GC_CHARACTER_DEL` | `ActorDeleteHandler.h/.cpp` | `test_c26_actor_delete_handler.cpp` | Jules Worker #03 |
| **B1-04** | [x] | `GC_CHARACTER_MOVE` | `ActorMoveHandler.h/.cpp` | `test_c26_actor_move_handler.cpp` | Jules Worker #04 |
| **B1-05** | [x] | `GC_CHARACTER_ADDITIONAL_INFO` | `ActorAdditionalInfoHandler.h/.cpp` | `test_c26_actor_additional_info_handler.cpp` | Jules Worker #05 |
| **B1-06** | [x] | `GC_POINT_CHANGE` / `POINTS` | `PlayerPointsHandler.h/.cpp` | `test_c26_player_points_handler.cpp` | Jules Worker #06 |
| **B1-07** | [x] | `GC_ITEM_SET` | `ItemSetHandler.h/.cpp` | `test_c26_item_set_handler.cpp` | Jules Worker #07 |
| **B1-08** | [x] | `GC_ITEM_DEL` | `ItemDelHandler.h/.cpp` | `test_c26_item_del_handler.cpp` | Jules Worker #08 |
| **B1-09** | [x] | `GC_ITEM_GROUND_ADD` | `ItemGroundAddHandler.h/.cpp` | `test_c26_item_ground_add_handler.cpp` | Jules Worker #09 |
| **B1-10** | [x] | `GC_ITEM_GROUND_DEL` | `ItemGroundDelHandler.h/.cpp` | `test_c26_item_ground_del_handler.cpp` | Jules Worker #10 |
| **B1-11** | [x] | `GC_CHAT` / `WHISPER` | `ChatHandler.h/.cpp` | `test_c26_chat_handler.cpp` | Jules Worker #11 |
| **B1-12** | [x] | `GC_EXCHANGE` | `ExchangeHandler.h/.cpp` | `test_c26_exchange_handler.cpp` | Jules Worker #12 |
| **B1-13** | [x] | `GC_SHOP` | `ShopHandler.h/.cpp` | `test_c26_shop_handler.cpp` | Jules Worker #13 |
| **B1-14** | [x] | `GC_SAFEBOX` | `SafeboxHandler.h/.cpp` | `test_c26_safebox_handler.cpp` | Jules Worker #14 |
| **B1-15** | [x] | `GC_QUEST` | `QuestHandler.h/.cpp` | `test_c26_quest_handler.cpp` | Jules Worker #15 |
| **B1-16** | [x] | `GC_DEAD` | `DeadHandler.h/.cpp` | `test_c26_dead_handler.cpp` | Jules Worker #16 |
| **B1-17** | [x] | `GC_STUN` | `StunHandler.h/.cpp` | `test_c26_stun_handler.cpp` | Jules Worker #17 |
| **B1-18** | [x] | `GC_TARGET` | `TargetHandler.h/.cpp` | `test_c26_target_handler.cpp` | Jules Worker #18 |
| **B1-19** | [x] | `GC_PARTY` | `PartyHandler.h/.cpp` | `test_c26_party_handler.cpp` | Jules Worker #19 |
| **B1-20** | [x] | `GC_GUILD` | `GuildHandler.h/.cpp` | `test_c26_guild_handler.cpp` | Jules Worker #20 |
| **B1-21** | [ ] | Podpiecie `ModernPacketDispatcher` pod `PythonNetworkStream::GamePhase()` | `Network/ModernPacketDispatcher.cpp` | `test_c26_modern_packet_dispatcher.cpp` | Jules Worker #21 |
| **B1-22** | [ ] | Calkowite odciecie starego `PythonNetworkStreamPhaseGame.cpp` z kompilacji | `UserInterface/CMakeLists.txt` | Kompilacja calego UserInterface | Jules Worker #22 |

---

## 3. KRYTERIA AKCEPTACJI DLA AGENTA B1 (DEFINITION OF DONE)
1. Zaden handler nie uzywa typu `DWORD`, `BYTE` ani `BOOL` (obowiazek `uint32_t`, `uint8_t`, `bool`, `StrongTypes`).
2. Zaden handler nie wola `PyCallClassMemberFunc`.
3. 100% testow w `tests/test_c26_*` dla tych handlerow zwraca wynik pozytywny (`PASS`).
