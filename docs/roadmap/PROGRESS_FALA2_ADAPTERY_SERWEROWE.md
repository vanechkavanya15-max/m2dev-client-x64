# FALA 2: ADAPTERY SERWEROWE I IMITATOR KLIENTA (75 AGENTOW)
## Rejestr Zadan, Stan Realizacji i Kontrakty Architektoniczne

**Cel Fali 2:** Podpiecie czystych komend domenowych z Fali 1 pod konkretne formaty sieciowe, wymienna kryptografie, profile serwerow JSON oraz deterministyczna emulacje klienta oficialnego bez zadnego memcpy.  
**Standard:** C++23, MSVC 2022 x64, CMake.  
**Zasada:** Kazde zadanie tworzy:
1. Plik naglowkowy `.h` w `src/Client/`
2. Plik implementacji `.cpp` w `src/Client/`
3. Kompletny plik testowy `test_c26_*.cpp` w `tests/`
**Zelazna Zasada:** ZERO MODYFIKACJI RENDERERA (DirectX 9, GrpDevice, shaders, Granny, SpeedTree, PRTerrain nienaruszone).

---

## ZESPOL E: SILNIK SCHEMATOW I AUTO-SERIALIZACJI (20 AGENTOW)
*Obszar:* Dynamiczne kodowanie i dekodowanie pakietow z eliminacja memcpy i rzutowania struktur.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **E-01** | [x] | `DynamicPacketFramer` (framing 1B Ymir vs 2B/4B m2dev) | `Client/Network/DynamicPacketFramer.h` | `Client/Network/DynamicPacketFramer.cpp` | `test_c26_dynamic_packet_framer.cpp` |
| **E-02** | [x] | `ModernPacketDispatcher` (rejestracja i dispatch handlerow GC) | `Client/Network/ModernPacketDispatcher.h` | `Client/Network/ModernPacketDispatcher.cpp` | `test_c26_modern_packet_dispatcher.cpp` |
| **E-03** | [x] | `PacketRingBuffer` (bezpieczny bufor kolowy odbioru i wysylki) | `Client/Network/PacketRingBuffer.h` | `Client/Network/PacketRingBuffer.cpp` | `test_c26_packet_ring_buffer.cpp` |
| **E-04** | [x] | `ActorPacketCodec` (kodek serializacji encji postaci) | `Client/Network/ActorPacketCodec.h` | `Client/Network/ActorPacketCodec.cpp` | `test_c26_actor_packet_codec.cpp` |
| **E-05** | [x] | `CombatPacketCodec` (kodek pakietow ataku i obrazen) | `Client/Network/CombatPacketCodec.h` | `Client/Network/CombatPacketCodec.cpp` | `test_c26_combat_packet_codec.cpp` |
| **E-06** | [x] | `ItemPacketCodec` (kodek ekwipunku i itemow na ziemi) | `Client/Network/ItemPacketCodec.h` | `Client/Network/ItemPacketCodec.cpp` | `test_c26_item_packet_codec.cpp` |
| **E-07** | [x] | `CombatCommandEncoder` (automatyczna serializacja AttackCommand) | `Client/Network/CombatCommandEncoder.h` | `Client/Network/CombatCommandEncoder.cpp` | `test_c26_combat_command_encoder.cpp` |
| **E-08** | [x] | `MovementCommandEncoder` (serializacja MoveCommand do bajtow CG) | `Client/Network/MovementCommandEncoder.h` | `Client/Network/MovementCommandEncoder.cpp` | `test_c26_movement_command_encoder.cpp` |
| **E-09** | [x] | `InventoryCommandEncoder` (serializacja UseItem, DropItem) | `Client/Network/InventoryCommandEncoder.h` | `Client/Network/InventoryCommandEncoder.cpp` | `test_c26_inventory_command_encoder.cpp` |
| **E-10** | [x] | `ExchangeCommandEncoder` (serializacja komend wymiany) | `Client/Network/ExchangeCommandEncoder.h` | `Client/Network/ExchangeCommandEncoder.cpp` | `test_c26_exchange_command_encoder.cpp` |
| **E-11** | [x] | `ShopCommandEncoder` (serializacja komend sklepu NPC) | `Client/Network/ShopCommandEncoder.h` | `Client/Network/ShopCommandEncoder.cpp` | `test_c26_shop_command_encoder.cpp` |
| **E-12** | [x] | `QuestCommandEncoder` (serializacja odpowiedzi questowych) | `Client/Network/QuestCommandEncoder.h` | `Client/Network/QuestCommandEncoder.cpp` | `test_c26_quest_command_encoder.cpp` |
| **E-13** | [x] | `SocialCommandEncoder` (serializacja chat, whisper, party) | `Client/Network/SocialCommandEncoder.h` | `Client/Network/SocialCommandEncoder.cpp` | `test_c26_social_command_encoder.cpp` |
| **E-14** | [x] | `PendingSpawnRegistry` (kolejka opoznionego tworzenia bytow) | `Client/Network/PendingSpawnRegistry.h` | `Client/Network/PendingSpawnRegistry.cpp` | `test_c26_pending_spawn_registry.cpp` |
| **E-15** | [x] | `NetworkStreamPort` (port sieciowy laczacy nowy silnik ze starym CNetworkStream) | `Client/Bridge/NetworkStreamPort.h` | `Client/Bridge/NetworkStreamPort.cpp` | `test_c26_network_stream_port.cpp` |
| **E-16** | [ ] | `PacketSchemaEngine` (dynamiczny parser schematow JSON w runtime) | `Client/Network/PacketSchemaEngine.h` | `Client/Network/PacketSchemaEngine.cpp` | `test_c26_packet_schema_engine.cpp` |
| **E-17** | [ ] | `ZeroCopyBufferView` (zerokosztowe widoki bajtowe bez alokacji pamieci) | `Client/Network/ZeroCopyBufferView.h` | `Client/Network/ZeroCopyBufferView.cpp` | `test_c26_zero_copy_buffer.cpp` |
| **E-18** | [ ] | `PacketHeaderValidator` (walidacja poprawnosci naglowka i dlugosci payloadu) | `Client/Network/PacketHeaderValidator.h` | `Client/Network/PacketHeaderValidator.cpp` | `test_c26_header_validator.cpp` |
| **E-19** | [ ] | `DynamicPayloadDeserializer` (deserializator zmiennych struktur GC) | `Client/Network/DynamicPayloadDeserializer.h` | `Client/Network/DynamicPayloadDeserializer.cpp` | `test_c26_dynamic_deserializer.cpp` |
| **E-20** | [ ] | `OutgoingPacketQueue` (kolejka priorytetowa wysylki pakietow CG) | `Client/Network/OutgoingPacketQueue.h` | `Client/Network/OutgoingPacketQueue.cpp` | `test_c26_outgoing_queue.cpp` |

---

## ZESPOL F: WYMIENNA KRYPTOGRAFIA I HANDSHAKE (20 AGENTOW)
*Obszar:* TEA, AES, Libsodium ChaCha20/X25519, maszyna stanow powitania HandshakeFSM.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **F-01** | [x] | `ICryptoProvider` (czysty interfejs szyfrowania strumienia) | `Client/Network/ICryptoProvider.h` | `Client/Network/ICryptoProvider.cpp` | `test_c26_crypto_interface.cpp` |
| **F-02** | [x] | `TeaCryptoProvider` (tradycyjna kryptografia Metin2 16B TEA/XTEA) | `Client/Network/TeaCryptoProvider.h` | `Client/Network/TeaCryptoProvider.cpp` | `test_c26_tea_crypto_provider.cpp` |
| **F-03** | [x] | `AesCryptoProvider` (szyfrowanie AES-128/256 dla PandoraMT2) | `Client/Network/AesCryptoProvider.h` | `Client/Network/AesCryptoProvider.cpp` | `test_c26_aes_crypto_provider.cpp` |
| **F-04** | [x] | `PluggableCryptoProvider` (dynamiczny przelacznik szyfrowania w locie) | `Client/Network/PluggableCryptoProvider.h` | `Client/Network/PluggableCryptoProvider.cpp` | `test_c26_pluggable_crypto_provider.cpp` |
| **F-05** | [x] | `HandshakeFSM` (automat fazy powitania: 13B Ymir vs 32B token) | `Client/Network/HandshakeFSM.h` | `Client/Network/HandshakeFSM.cpp` | `test_c26_handshake_fsm.cpp` |
| **F-06** | [ ] | `SodiumCryptoProvider` (nowoczesny handshake X25519 / ChaCha20) | `Client/Network/SodiumCryptoProvider.h` | `Client/Network/SodiumCryptoProvider.cpp` | `test_c26_sodium_crypto_provider.cpp` |
| **F-07** | [ ] | `KeyAgreementProtocol` (wymiana kluczy Diffie-Hellman) | `Client/Network/KeyAgreementProtocol.h` | `Client/Network/KeyAgreementProtocol.cpp` | `test_c26_key_agreement.cpp` |
| **F-08** | [ ] | `Crc32Hasher` (szybkie liczenie sum kontrolnych naglowkow) | `Client/Network/Crc32Hasher.h` | `Client/Network/Crc32Hasher.cpp` | `test_c26_crc32_hasher.cpp` |
| **F-09** | [ ] | `PacketNonceManager` (zarzadzanie wektorami inicjalizacyjnymi IV) | `Client/Network/PacketNonceManager.h` | `Client/Network/PacketNonceManager.cpp` | `test_c26_nonce_manager.cpp` |
| **F-10** | [ ] | `HandshakeChallengeResolver` (obsluga GC_KEY_CHALLENGE / COMPLETE) | `Client/Network/HandshakeChallengeResolver.h` | `Client/Network/HandshakeChallengeResolver.cpp` | `test_c26_challenge_resolver.cpp` |
| **F-11** | [ ] | `AuthLoginPayloadBuilder` (bezpieczne tworzenie pakietu CG_LOGIN) | `Client/Network/AuthLoginPayloadBuilder.h` | `Client/Network/AuthLoginPayloadBuilder.cpp` | `test_c26_auth_payload_builder.cpp` |
| **F-12** | [ ] | `AuthResultEvaluator` (ewaluacja odpowiedzi auth_success/failure) | `Client/Network/AuthResultEvaluator.h` | `Client/Network/AuthResultEvaluator.cpp` | `test_c26_auth_evaluator.cpp` |
| **F-13** | [ ] | `EmpireSelectionEncoder` (kodowanie pakietu wyboru krolestwa) | `Client/Network/EmpireSelectionEncoder.h` | `Client/Network/EmpireSelectionEncoder.cpp` | `test_c26_empire_encoder.cpp` |
| **F-14** | [ ] | `CharacterSelectionEncoder` (kodowanie pakietu wyboru postaci) | `Client/Network/CharacterSelectionEncoder.h` | `Client/Network/CharacterSelectionEncoder.cpp` | `test_c26_char_select_encoder.cpp` |
| **F-15** | [ ] | `CharacterCreationEncoder` (kodowanie tworzenia nowej postaci) | `Client/Network/CharacterCreationEncoder.h` | `Client/Network/CharacterCreationEncoder.cpp` | `test_c26_char_create_encoder.cpp` |
| **F-16** | [ ] | `CharacterDeletionEncoder` (kodowanie usuwania postaci z kodem usuniecia) | `Client/Network/CharacterDeletionEncoder.h` | `Client/Network/CharacterDeletionEncoder.cpp` | `test_c26_char_delete_encoder.cpp` |
| **F-17** | [ ] | `EnterGamePhaseTransition` (pakiet wejscia do gry CG_ENTERGAME) | `Client/Network/EnterGamePhaseTransition.h` | `Client/Network/EnterGamePhaseTransition.cpp` | `test_c26_enter_game.cpp` |
| **F-18** | [ ] | `NetworkTimeoutWatchdog` (wykrywanie zerwania socketu i lagow) | `Client/Network/NetworkTimeoutWatchdog.h` | `Client/Network/NetworkTimeoutWatchdog.cpp` | `test_c26_timeout_watchdog.cpp` |
| **F-19** | [ ] | `NetworkReconnectManager` (automatyczne ponawianie polaczenia po warp) | `Client/Network/NetworkReconnectManager.h` | `Client/Network/NetworkReconnectManager.cpp` | `test_c26_reconnect_manager.cpp` |
| **F-20** | [ ] | `TrafficMetricsCollector` (statystyki bajtow wejscia/wyjscia i ping) | `Client/Network/TrafficMetricsCollector.h` | `Client/Network/TrafficMetricsCollector.cpp` | `test_c26_traffic_metrics.cpp` |

---

## ZESPOL G: EMULACJA ZACHOWANIA OFICJALNEGO KLIENTA (20 AGENTOW)
*Obszar:* Limity czasowe, liczenie CRC ataku, interwaly podnoszenia dropu i zachowanie anty-cheat.

| ID | Status | Zadanie / Modul | Plik Naglowkowy (.h) | Plik Implementacji (.cpp) | Plik Testowy w tests/ |
|---|---|---|---|---|---|
| **G-01** | [x] | `MovementRateLimiter` (wysylanie ruchu scisle co 250ms) | `Client/Gameplay/MovementRateLimiter.h` | `Client/Gameplay/MovementRateLimiter.cpp` | `test_c26_movement_rate_limiter.cpp` |
| **G-02** | [x] | `CombatSequenceGuard` (liczenie CRC ataku i numeru dwSeq) | `Client/Gameplay/CombatSequenceGuard.h` | `Client/Gameplay/CombatSequenceGuard.cpp` | `test_c26_combat_sequence_guard.cpp` |
| **G-03** | [x] | `InventoryPickupLimiter` (odstepy podnoszenia dropu 100ms) | `Client/Gameplay/InventoryPickupLimiter.h` | `Client/Gameplay/InventoryPickupLimiter.cpp` | `test_c26_inventory_pickup_limiter.cpp` |
| **G-04** | [x] | `PhaseStateMachine` (maszyna faz: OffLine, Handshake, Login, Select, Loading, Game) | `Client/Network/PhaseStateMachine.h` | `Client/Network/PhaseStateMachine.cpp` | `test_c26_phase_state_machine.cpp` |
| **G-05** | [ ] | `PingPongKeepAlive` (odpowiadanie na PING z realistyczna delta) | `Client/Network/PingPongKeepAlive.h` | `Client/Network/PingPongKeepAlive.cpp` | `test_c26_ping_pong.cpp` |
| **G-06** | [ ] | `ClientTimeSynchronizer` (synchronizacja zegara klienta z server_time) | `Client/Network/ClientTimeSynchronizer.h` | `Client/Network/ClientTimeSynchronizer.cpp` | `test_c26_time_sync.cpp` |
| **G-07** | [ ] | `AttackSpeedEmulator` (dopasowanie interwalu uderzen do animacji broni) | `Client/Gameplay/AttackSpeedEmulator.h` | `Client/Gameplay/AttackSpeedEmulator.cpp` | `test_c26_attack_speed.cpp` |
| **G-08** | [ ] | `CastTimeEmulator` (emulacja czasu ladowania czaru przed wysylka) | `Client/Gameplay/CastTimeEmulator.h` | `Client/Gameplay/CastTimeEmulator.cpp` | `test_c26_cast_time.cpp` |
| **G-09** | [ ] | `PotionConsumptionCooldown` (limity uzywania mikstur czerwonych/niebieskich) | `Client/Gameplay/PotionConsumptionCooldown.h` | `Client/Gameplay/PotionConsumptionCooldown.cpp` | `test_c26_potion_cooldown.cpp` |
| **G-10** | [ ] | `ChatRateLimiter` (ochrona przed wyciszeniem za spam na czacie) | `Client/Gameplay/ChatRateLimiter.h` | `Client/Gameplay/ChatRateLimiter.cpp` | `test_c26_chat_rate_limiter.cpp` |
| **G-11** | [ ] | `WhisperRateLimiter` (limity czestotliwosci wiadomosci prywatnych) | `Client/Gameplay/WhisperRateLimiter.h` | `Client/Gameplay/WhisperRateLimiter.cpp` | `test_c26_whisper_rate_limiter.cpp` |
| **G-12** | [ ] | `ShopInteractionCooldown` (ochrona przed spamem zakupu u NPC) | `Client/Gameplay/ShopInteractionCooldown.h` | `Client/Gameplay/ShopInteractionCooldown.cpp` | `test_c26_shop_cooldown.cpp` |
| **G-13** | [ ] | `ExchangeInteractionCooldown` (ochrona przed spamem akcji handlu) | `Client/Gameplay/ExchangeInteractionCooldown.h` | `Client/Gameplay/ExchangeInteractionCooldown.cpp` | `test_c26_exchange_cooldown.cpp` |
| **G-14** | [ ] | `GuildActionCooldown` (limity uzywania umiejetnosci gildyjnych) | `Client/Gameplay/GuildActionCooldown.h` | `Client/Gameplay/GuildActionCooldown.cpp` | `test_c26_guild_cooldown.cpp` |
| **G-15** | [ ] | `DungeonTimerTracker` (lokalny tracker uplywu czasu pietra w lochu) | `Client/Gameplay/DungeonTimerTracker.h` | `Client/Gameplay/DungeonTimerTracker.cpp` | `test_c26_dungeon_timer.cpp` |
| **G-16** | [ ] | `FishingActionEmulator` (emulacja ludzkich odstepow wyciagania wedki) | `Client/Gameplay/FishingActionEmulator.h` | `Client/Gameplay/FishingActionEmulator.cpp` | `test_c26_fishing_emulator.cpp` |
| **G-17** | [ ] | `MiningActionEmulator` (emulacja czasu uderzania kilofem w zyle) | `Client/Gameplay/MiningActionEmulator.h` | `Client/Gameplay/MiningActionEmulator.cpp` | `test_c26_mining_emulator.cpp` |
| **G-18** | [ ] | `ItemRefineDelayEmulator` (emulacja opoznienia kowala przy ulepszaniu) | `Client/Gameplay/ItemRefineDelayEmulator.h` | `Client/Gameplay/ItemRefineDelayEmulator.cpp` | `test_c26_refine_delay.cpp` |
| **G-19** | [ ] | `HumanizedMicroPause` (losowe mikropauzy 10-30ms zapobiegajace detekcji botow) | `Client/Gameplay/HumanizedMicroPause.h` | `Client/Gameplay/HumanizedMicroPause.cpp` | `test_c26_micro_pause.cpp` |
| **G-20** | [ ] | `ClientSequenceIntegrityGuard` (weryfikacja czy sekwencja pakietow jest spojna) | `Client/Network/ClientSequenceIntegrityGuard.h` | `Client/Network/ClientSequenceIntegrityGuard.cpp` | `test_c26_sequence_guard.cpp` |

---

## ZESPOL H: PROFILE SERWEROW I TESTY E2E (15 AGENTOW)
*Obszar:* Konfiguracje JSON, dynamiczne mapowanie serwerow i kompleksowe testy E2E.

| ID | Status | Zadanie / Modul | Plik Zrodlowy / Konfiguracja | Plik Testowy w tests/ |
|---|---|---|---|---|
| **H-01** | [x] | `ServerProfile` (model danych profilu serwera w C++23) | `Client/Network/ServerProfile.h/.cpp` | `test_c26_server_profile.cpp` |
| **H-02** | [x] | `ServerProfileManager` (ladowanie i zarzadzanie profilami z katalogu servers/) | `Client/Network/ServerProfileManager.h/.cpp` | `test_c26_server_profile_manager.cpp` |
| **H-03** | [x] | Profil serwera: `servers/pandora.json` (ARM64/PC PandoraMT2) | `servers/pandora.json` | `test_c26_server_profile.cpp` |
| **H-04** | [x] | Profil serwera: `servers/classic_ymir.json` (oficjalny Ymir 2004) | `servers/classic_ymir.json` | `test_c26_server_profile.cpp` |
| **H-05** | [x] | Profil serwera: `servers/m2dev.json` (nowoczesny m2dev x64) | `servers/m2dev.json` | `test_c26_server_profile.cpp` |
| **H-06** | [ ] | Profil serwera: `servers/alune.json` (protokol serwera Alune) | `servers/alune.json` | `test_c26_server_profile.cpp` |
| **H-07** | [ ] | Profil serwera: `servers/glador.json` (protokol serwera Glador) | `servers/glador.json` | `test_c26_server_profile.cpp` |
| **H-08** | [ ] | Test E2E Auth Handshake: Symulacja logowania i generowania kluczy | `tests/e2e/test_e2e_auth_handshake.cpp` | Test integracyjny Auth |
| **H-09** | [ ] | Test E2E Character Select: Pakiety fazy wyboru i ladowania swiata | `tests/e2e/test_e2e_character_select.cpp` | Test integracyjny Select |
| **H-10** | [ ] | Test E2E Movement: Serializacja MoveCommand -> surowe bajty -> deszyfracja | `tests/e2e/test_e2e_movement_pipeline.cpp` | Test integracyjny Move |
| **H-11** | [ ] | Test E2E Combat: Serializacja AttackCommand z liczeniem CRC i dwSeq | `tests/e2e/test_e2e_combat_pipeline.cpp` | Test integracyjny Combat |
| **H-12** | [ ] | Test E2E Inventory & Equip: Uzycie przedmiotu, walidacja i odpowiedz | `tests/e2e/test_e2e_inventory_pipeline.cpp` | Test integracyjny Inventory |
| **H-13** | [ ] | Test E2E Chat & Whisper: Nadawanie na czacie globalnym i szept | `tests/e2e/test_e2e_chat_pipeline.cpp` | Test integracyjny Chat |
| **H-14** | [ ] | Test E2E Multi-Server Switch: Przelaczenie profilu w locie z Ymir na Pandora | `tests/e2e/test_e2e_profile_switch.cpp` | Test integracyjny Switch |
| **H-15** | [ ] | Kompletny Raport Walidacji E2E (100% zdeszyfrowanych pakietow) | `docs/E2E_VALIDATION_REPORT.md` | Caly suite testowy |

---

## PODSUMOWANIE FALI 2:
- Laczna liczba zadan: **75 zadan atomowych**.
- Status: **24/75 ukonczone w Client2026**; pozostale 51 zadan gotowe do natychmiastowego zlecenia robotnikom w roju Jules Swarm!
