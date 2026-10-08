# FALA 1: FUNDAMENT HEXAGONALNY I CZYSTE DOMENY (75 AGENTOW)
## Rejestr Zadan, Stan Realizacji i Kontrakty Architektoniczne

**Cel Fali 1:** Calkowite odciecie logiki gry od singletonow i tworzenie czystych, funkcyjnych domen oraz deklaratywnych komend.  
**Standard:** C++23, MSVC 2022 x64, CMake.  
**Zasada:** Kazde zadanie tworzy:
1. Plik naglowkowy `.h` w `src/Client/`
2. Plik implementacji `.cpp` w `src/Client/`
3. Kompletny plik testowy `test_c26_*.cpp` w `tests/`
**Zelazna Zasada:** ZERO MODYFIKACJI RENDERERA (DirectX 9, GrpDevice, shaders, Granny, SpeedTree, PRTerrain nienaruszone).

---

## ZESPOL A: CORE & SESSION (15 AGENTOW)
*Obszar:* Baza sesji, unifikacja stanu swiata, wirtualne porty wejscia/wyjscia i deklaratywne komendy.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **A-01** | [x] | `WorldContext` (stan lokalnego gracza, HP, SP, Gold, rotacja) | `Client/Core/WorldContext.h` | `Client/Core/WorldContext.cpp` | `test_c26_world_context.cpp` |
| **A-02** | [x] | `WorldContext` (kontenery bytów, mapa encji VID w zasięgu) | `Client/Core/WorldContext.h` | `Client/Core/WorldContext_Entities.cpp` | `test_c26_world_entities.cpp` |
| **A-03** | [x] | `GameSession` (agregacja kontekstu i petla Tick) | `Client/Core/GameSession.h` | `Client/Core/GameSession.cpp` | `test_c26_game_session.cpp` |
| **A-04** | [x] | `INetworkPort` (czysty wirtualny interfejs I/O sieci) | `Client/Core/INetworkPort.h` | `Client/Core/INetworkPort.cpp` | `test_c26_network_port.cpp` |
| **A-05** | [x] | `MockNetworkPort` (dwukierunkowy port testowy w RAM) | `Client/Core/MockNetworkPort.h` | `Client/Core/MockNetworkPort.cpp` | `test_c26_mock_port.cpp` |
| **A-06** | [x] | `DomainCommands: AttackCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Combat.cpp` | `test_c26_cmd_attack.cpp` |
| **A-07** | [x] | `DomainCommands: MoveCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Move.cpp` | `test_c26_cmd_move.cpp` |
| **A-08** | [x] | `DomainCommands: UseSkillCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Skill.cpp` | `test_c26_cmd_skill.cpp` |
| **A-09** | [x] | `DomainCommands: UseItemCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Item.cpp` | `test_c26_cmd_item.cpp` |
| **A-10** | [x] | `DomainCommands: PickupCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Pickup.cpp` | `test_c26_cmd_pickup.cpp` |
| **A-11** | [x] | `DomainCommands: InteractNpcCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Npc.cpp` | `test_c26_cmd_npc.cpp` |
| **A-12** | [x] | `DomainCommands: ChatCommand` | `Client/Core/DomainCommands.h` | `Client/Core/DomainCommands_Chat.cpp` | `test_c26_cmd_chat.cpp` |
| **A-13** | [x] | `FrameTimer` (taktowanie logiki sztywnym krokiem czasowym) | `Client/Core/FrameTimer.h` | `Client/Core/FrameTimer.cpp` | `test_c26_frame_timer.cpp` |
| **A-14** | [x] | `MathSIMD` (wektory przestrzenne i optymalizacje SSE/AVX) | `Client/Core/MathSIMD.h` | `Client/Core/MathSIMD.cpp` | `test_c26_math_simd.cpp` |
| **A-15** | [x] | `StrongTypes` (silne identyfikatory EntityVid, ItemVnum, SkillId) | `Client/Core/StrongTypes.h` | `Client/Core/StrongTypes.cpp` | `test_c26_strong_types.cpp` |

---

## ZESPOL B: DOMENY WALKI I BYTOW (20 AGENTOW)
*Obszar:* Funkcyjne API walki, walidacja trajektorii i celow, zarzadzanie skillami i cyklem zycia bytow.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **B-01** | [x] | `CombatDomain::ProcessAttack` (funkcyjne API ataku) | `Client/Gameplay/CombatDomain.h` | `Client/Gameplay/CombatDomain.cpp` | `test_c26_combat_domain.cpp` |
| **B-02** | [x] | `CombatRangeCalculator` (zasiegi broni: miecz, luk, sztylet) | `Client/Gameplay/CombatRangeCalculator.h` | `Client/Gameplay/CombatRangeCalculator.cpp` | `test_c26_combat_range_calculator.cpp` |
| **B-03** | [x] | `CombatTargetValidator` (weryfikacja czy cel zyje i jest wrogi) | `Client/Gameplay/CombatTargetValidator.h` | `Client/Gameplay/CombatTargetValidator.cpp` | `test_c26_combat_target_validator.cpp` |
| **B-04** | [x] | `CombatDamagePredictor` (kalkulacja krytyka, przeszywki, bloku) | `Client/Gameplay/CombatDamagePredictor.h` | `Client/Gameplay/CombatDamagePredictor.cpp` | `test_c26_combat_damage_predictor.cpp` |
| **B-05** | [x] | `CombatCommandHandler` (dyspozytor komend ataku) | `Client/Gameplay/CombatCommandHandler.h` | `Client/Gameplay/CombatCommandHandler.cpp` | `test_c26_combat_command_handler.cpp` |
| **B-06** | [x] | `SkillDomain` (zarzadzanie poziomami i odblokowaniem skilli) | `Client/Gameplay/SkillDomain.h` | `Client/Gameplay/SkillDomain.cpp` | `test_c26_skill_domain.cpp` |
| **B-07** | [x] | `SkillCooltimeManager` (rejestr czasow odnowienia skilli) | `Client/Gameplay/SkillCooltimeManager.h` | `Client/Gameplay/SkillCooltimeManager.cpp` | `test_c26_skill_cooltime_handler.cpp` |
| **B-08** | [x] | `SkillCostCalculator` (obliczanie kosztow SP i HP skilla) | `Client/Gameplay/SkillCostCalculator.h` | `Client/Gameplay/SkillCostCalculator.cpp` | `test_c26_skill_cost_calc.cpp` |
| **B-09** | [x] | `ActorLifecycleDomain` (spawnowanie i tworzenie bytu) | `Client/Gameplay/ActorLifecycleDomain.h` | `Client/Gameplay/ActorLifecycleDomain.cpp` | `test_c26_actor_spawn_handler.cpp` |
| **B-10** | [x] | `ActorDespawnHandler` (bezpieczne usuwanie bytu ze swiata) | `Client/Gameplay/ActorDespawnHandler.h` | `Client/Gameplay/ActorDespawnHandler.cpp` | `test_c26_actor_delete_handler.cpp` |
| **B-11** | [x] | `ActorDeadReckoning` (przewidywanie pozycji bez lagow) | `Client/Gameplay/ActorDeadReckoning.h` | `Client/Gameplay/ActorDeadReckoning.cpp` | `test_c26_dead_reckoning.cpp` |
| **B-12** | [x] | `ActorMotionMachine` (stany animacji: Idle, Run, Attack, Hurt) | `Client/World/ActorMotionMachine.h` | `Client/World/ActorMotionMachine.cpp` | `test_c26_actor_motion_machine.cpp` |
| **B-13** | [x] | `ActorAlphaBlender` (plynne zanikanie i pojawianie sie bytow) | `Client/World/ActorAlphaBlender.h` | `Client/World/ActorAlphaBlender.cpp` | `test_c26_actor_alpha_blender.cpp` |
| **B-14** | [x] | `MountDomain` (mechanika wierzchowcow i konia) | `Client/Gameplay/MountDomain.h` | `Client/Gameplay/MountDomain.cpp` | `test_c26_mount_domain.cpp` |
| **B-15** | [x] | `PetDomain` (mechanika zwierzakow domowych i buffow) | `Client/Gameplay/PetDomain.h` | `Client/Gameplay/PetDomain.cpp` | `test_c26_pet_domain.cpp` |
| **B-16** | [x] | `StunStateGuard` (zarzadzanie ogluszeniem i paralizem) | `Client/Gameplay/StunStateGuard.h` | `Client/Gameplay/StunStateGuard.cpp` | `test_c26_stun_handler.cpp` |
| **B-17** | [x] | `DeadStateGuard` (zarzadzanie stanem zgonu i odrodzeniem) | `Client/Gameplay/DeadStateGuard.h` | `Client/Gameplay/DeadStateGuard.cpp` | `test_c26_dead_handler.cpp` |
| **B-18** | [x] | `CombatComboChain` (obsluga 4-uderzeniowych sekwencji combo) | `Client/Gameplay/CombatComboChain.h` | `Client/Gameplay/CombatComboChain.cpp` | `test_c26_combo_chain.cpp` |
| **B-19** | [x] | `TargetSelectionEngine` (wybor celu: TAB, klik, najblizszy) | `Client/Gameplay/TargetSelectionEngine.h` | `Client/Gameplay/TargetSelectionEngine.cpp` | `test_c26_target_handler.cpp` |
| **B-20** | [x] | `PvpModeManager` (tryby Peaceful, Hostile, Guild, Revenge) | `Client/Gameplay/PvpModeManager.h` | `Client/Gameplay/PvpModeManager.cpp` | `test_c26_pvp_mode.cpp` |

---

## ZESPOL C: DOMENY EKWIPUNKU I HANDLU (20 AGENTOW)
*Obszar:* Deterministyczna siatka slotow, uzywanie i dzielenie itemow, bezpieczna wymiana gracz-gracz, zakupy NPC.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **C-01** | [x] | `InventoryDomain` (zarzadca ekwipunku 90 slotow) | `Client/Gameplay/InventoryDomain.h` | `Client/Gameplay/InventoryDomain.cpp` | `test_c26_inventory_domain.cpp` |
| **C-02** | [x] | `InventoryGridManager` (rozmiary itemow 1x1, 1x2, 1x3) | `Client/Gameplay/InventoryGridManager.h` | `Client/Gameplay/InventoryGridManager.cpp` | `test_c26_inventory_grid_manager.cpp` |
| **C-03** | [x] | `InventoryItemValidator` (weryfikacja wymagan: level, plec, klasa) | `Client/Gameplay/InventoryItemValidator.h` | `Client/Gameplay/InventoryItemValidator.cpp` | `test_c26_inventory_item_validator.cpp` |
| **C-04** | [x] | `InventoryPickupLimiter` (odstepy czasowe podnoszenia 100ms) | `Client/Gameplay/InventoryPickupLimiter.h` | `Client/Gameplay/InventoryPickupLimiter.cpp` | `test_c26_inventory_pickup_limiter.cpp` |
| **C-05** | [x] | `InventoryCommandHandler` (obsluga Use, Equip, Unequip, Swap) | `Client/Gameplay/InventoryCommandHandler.h` | `Client/Gameplay/InventoryCommandHandler.cpp` | `test_c26_inventory_command_handler.cpp` |
| **C-06** | [x] | `InventoryDropPhysicsBridge` (pozycja dropu na ziemie z katem) | `Client/Gameplay/InventoryDropPhysicsBridge.h` | `Client/Gameplay/InventoryDropPhysicsBridge.cpp` | `test_c26_inventory_drop_physics_bridge.cpp` |
| **C-07** | [x] | `TradeDomain` (automat bezpiecznej wymiany z innym graczem) | `Client/Gameplay/TradeDomain.h` | `Client/Gameplay/TradeDomain.cpp` | `test_c26_trade_domain.cpp` |
| **C-08** | [x] | `ExchangeCommandHandler` (komendy Accept, Cancel, Lock, AddItem) | `Client/Gameplay/ExchangeCommandHandler.h` | `Client/Gameplay/ExchangeCommandHandler.cpp` | `test_c26_exchange_command_handler.cpp` |
| **C-09** | [x] | `ExchangeStateGuard` (stany wymiany: Open, Locked, Confirmed) | `Client/Gameplay/ExchangeStateGuard.h` | `Client/Gameplay/ExchangeStateGuard.cpp` | `test_c26_exchange_state_guard.cpp` |
| **C-10** | [x] | `ShopCommandHandler` (komendy zakupu i sprzedazy u NPC) | `Client/Gameplay/ShopCommandHandler.h` | `Client/Gameplay/ShopCommandHandler.cpp` | `test_c26_shop_command_handler.cpp` |
| **C-11** | [x] | `ShopPriceValidator` (sprawdzanie cen: Gold, Cheque, Gaya) | `Client/Gameplay/ShopPriceValidator.h` | `Client/Gameplay/ShopPriceValidator.cpp` | `test_c26_shop_price_validator.cpp` |
| **C-12** | [x] | `CurrencyType` (enum i konwersje walut gry) | `Client/Gameplay/CurrencyType.h` | `Client/Gameplay/CurrencyType.cpp` | `test_c26_currency_type.cpp` |
| **C-13** | [x] | `SafeboxCommandHandler` (obsluga depozytu i hasla do magazynu) | `Client/Gameplay/SafeboxCommandHandler.h` | `Client/Gameplay/SafeboxCommandHandler.cpp` | `test_c26_safebox_command_handler.cpp` |
| **C-14** | [x] | `ItemSocketsManager` (kamienie dusz i ulepszanie slotow) | `Client/Gameplay/ItemSocketsManager.h` | `Client/Gameplay/ItemSocketsManager.cpp` | `test_c26_item_sockets.cpp` |
| **C-15** | [x] | `ItemAttributesManager` (bonusy 1-5 oraz 6-7 bon) | `Client/Gameplay/ItemAttributesManager.h` | `Client/Gameplay/ItemAttributesManager.cpp` | `test_c26_item_attrs.cpp` |
| **C-16** | [x] | `QuickslotManager` (pasek 1-4 oraz F1-F4) | `Client/Gameplay/QuickslotManager.h` | `Client/Gameplay/QuickslotManager.cpp` | `test_c26_quickslot.cpp` |
| **C-17** | [x] | `SpecialInventoryManager` (magazyn na ksiegi, ulepy, kd) | `Client/Gameplay/SpecialInventoryManager.h` | `Client/Gameplay/SpecialInventoryManager.cpp` | `test_c26_special_inv.cpp` |
| **C-18** | [x] | `DragonSoulInventory` (siatka alchemii smoczych kamieni) | `Client/Gameplay/DragonSoulInventory.h` | `Client/Gameplay/DragonSoulInventory.cpp` | `test_c26_dragon_soul.cpp` |
| **C-19** | [x] | `BeltInventoryManager` (pas na mikstury) | `Client/Gameplay/BeltInventoryManager.h` | `Client/Gameplay/BeltInventoryManager.cpp` | `test_c26_belt_inv.cpp` |
| **C-20** | [x] | `CostumeInventoryManager` (sloty kostiumow, fryzur, szarf) | `Client/Gameplay/CostumeInventoryManager.h` | `Client/Gameplay/CostumeInventoryManager.cpp` | `test_c26_costume_inv.cpp` |

---

## ZESPOL D: DOMENY SWIATA I PRZESTRZENI (20 AGENTOW)
*Obszar:* Przestrzenne siatki indeksujace O(1), weryfikacja ruchu i kolizji, szyna zdarzen EventBus.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **D-01** | [x] | `SpatialHashGrid` (haszowanie przestrzenne bytów w O(1)) | `Client/World/SpatialHashGrid.h` | `Client/World/SpatialHashGrid.cpp` | `test_c26_spatial_hash_grid.cpp` |
| **D-02** | [x] | `MovementCommandHandler` (walidacja i wysylka komend ruchu) | `Client/Gameplay/MovementCommandHandler.h` | `Client/Gameplay/MovementCommandHandler.cpp` | `test_c26_movement_command_handler.cpp` |
| **D-03** | [x] | `MovementInterpolationEngine` (hermite spline i dead reckoning) | `Client/Gameplay/MovementInterpolationEngine.h` | `Client/Gameplay/MovementInterpolationEngine.cpp` | `test_c26_movement_interpolation_engine.cpp` |
| **D-04** | [x] | `MovementRateLimiter` (ograniczenie czestotliwosci ruchu np. 250ms) | `Client/Gameplay/MovementRateLimiter.h` | `Client/Gameplay/MovementRateLimiter.cpp` | `test_c26_movement_rate_limiter.cpp` |
| **D-05** | [x] | `MovementStepValidator` (ochrona przed teleportacja i speedhackiem) | `Client/Gameplay/MovementStepValidator.h` | `Client/Gameplay/MovementStepValidator.cpp` | `test_c26_movement_step_validator.cpp` |
| **D-06** | [x] | `MovementTerrainCollider` (detekcja kolizji z wysokoscia terenu) | `Client/Gameplay/MovementTerrainCollider.h` | `Client/Gameplay/MovementTerrainCollider.cpp` | `test_c26_movement_terrain_collider.cpp` |
| **D-07** | [x] | `TerrainHeightSampler` (probkowanie wysokosci Z mapy) | `Client/World/TerrainHeightSampler.h` | `Client/World/TerrainHeightSampler.cpp` | `test_c26_terrain_height_sampler.cpp` |
| **D-08** | [x] | `CollisionDetector` (kolizje ray-box i sfery) | `Client/World/CollisionDetector.h` | `Client/World/CollisionDetector.cpp` | `test_c26_collision_detector.cpp` |
| **D-09** | [x] | `ECSComponents` (komponenty: Transform, Velocity, BoundingBox) | `Client/World/ECSComponents.h` | `Client/World/ECSComponents.cpp` | `test_c26_ecs_components.cpp` |
| **D-10** | [x] | `EventBusPort` (szyna pub/sub dla bota i opcjonalnego interfejsu) | `Client/Core/EventBusPort.h` | `Client/Core/EventBusPort.cpp` | `test_c26_event_bus.cpp` |
| **D-11** | [x] | `MiniMapRadar` (przeliczanie pozycji encji na wspolrzedne radaru) | `Client/UI/MiniMapRadar.h` | `Client/UI/MiniMapRadar.cpp` | `test_c26_minimap_radar.cpp` |
| **D-12** | [x] | `TextTailEngine` (obliczanie pozycji 3D->2D napisow nad glowami) | `Client/UI/TextTailEngine.h` | `Client/UI/TextTailEngine.cpp` | `test_c26_texttail_engine.cpp` |
| **D-13** | [x] | `WindowHierarchy` (drzewo hierarchii okien interfejsu) | `Client/UI/WindowHierarchy.h` | `Client/UI/WindowHierarchy.cpp` | `test_c26_window_hierarchy.cpp` |
| **D-14** | [x] | `SocialDomain` (system znajomych, blokowania i szeptu) | `Client/Gameplay/SocialDomain.h` | `Client/Gameplay/SocialDomain.cpp` | `test_c26_social_domain.cpp` |
| **D-15** | [x] | `PartyCommandHandler` (zapraszanie, wyrzucanie, awans lidera) | `Client/Gameplay/PartyCommandHandler.h` | `Client/Gameplay/PartyCommandHandler.cpp` | `test_c26_party_command_handler.cpp` |
| **D-16** | [x] | `GuildCommandHandler` (zarzadzanie gildia, rangami i wojnami) | `Client/Gameplay/GuildCommandHandler.h` | `Client/Gameplay/GuildCommandHandler.cpp` | `test_c26_guild_command_handler.cpp` |
| **D-17** | [x] | `QuestCommandHandler` (wybory w dialogach NPC, wprowadzanie tekstu) | `Client/Gameplay/QuestCommandHandler.h` | `Client/Gameplay/QuestCommandHandler.cpp` | `test_c26_quest_command_handler.cpp` |
| **D-18** | [x] | `ChatFilterEngine` (filtrowanie spamu, wulgaryzmow i linkow) | `Client/Gameplay/ChatFilterEngine.h` | `Client/Gameplay/ChatFilterEngine.cpp` | `test_c26_chat_filter_engine.cpp` |
| **D-19** | [x] | `WhisperCommandHandler` (prywatne wiadomosci szeptu) | `Client/Gameplay/WhisperCommandHandler.h` | `Client/Gameplay/WhisperCommandHandler.cpp` | `test_c26_whisper_command_handler.cpp` |
| **D-20** | [x] | `PyBridgeFastCall` (szybki port danych C++ -> Python z walidacja) | `Client/UI/PyBridgeFastCall.h` | `Client/UI/PyBridgeFastCall.cpp` | `test_c26_pybridge_fastcall.cpp` |

---

## PODSUMOWANIE FALI 1:
- Laczna liczba zadan: **75 zadan atomowych**.
- Status: **75/75 zaimplementowanych i zintegrowanych w kodzie Client2026** wraz z testami jednostkowymi w `tests/`!
