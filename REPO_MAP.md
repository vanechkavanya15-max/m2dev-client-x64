# REPO MAP: Metin2 x64 Client2026 (SourceCodeClient)

> **Zrodlo Prawdy Architektury (Single Source of Truth) dla Agentow AI i Inzynierow**  
> Wygenerowano deterministycznie przez `tools/generate_repo_map.py`.  
> Standard: **C++20 / C++23 x64 Clean Architecture**. Koszt tokenow AI: **0**.

| Metryka Projektu | Wartosc | Opis |
|---|---|---|
| **Pliki Naglowkowe (.h / .hpp)** | `1117` | Interfejsy, struktury i silne typowanie C++23 |
| **Pliki Zrodlowe (.cpp / .c)** | `1326` | Implementacje domen, renderera i mostkow GUI |
| **Liczba Zidentyfikowanych Klas** | `2815` | Domeny, fasady, komponenty ECS i menedzery |
| **Przestrzenie Nazw (Namespaces)** | `130` | Scisly podzial Clean / Onion Architecture |
| **Routery, Dyspozytory i Kodeki** | `188` | Zdecentralizowany routing pakietow i zdarzen |
| **Rejestr CTest (Zarejestrowane)** | `51` | Zestawy testowe CTest (100% PASS, 0 warningow) |
| **Pliki Testow Jednostkowych** | `264` | Kompleksowe testy ramowe doctest / in-memory |

## 1. Architektura i Przestrzenie Nazw (Clean / Onion Layers)

Przeplyw zaleznosci w silniku jest scisle jednokierunkowy:  
**UserInterface (GUI/Python)** ➔ **Client::Gameplay / Network (Domeny C++23)** ➔ **Client::Core / EterBase (Fundamenty)**.

| Przestrzen Nazw | Warstwa Silnika | Pliki | Odpowiedzialnosc Architektoniczna |
|---|---|---|---|
| `Client::Gameplay` | **Warstwa Domenowa** | 65 | Czysta logika gry: walka, ekwipunek, umiejetnosci, handel (Zero DirectX/HWND) |
| `Client::Network` | **Warstwa Sieciowa** | 59 | Kodeki binarne, enkoder komend, framery pakietow i kryptografia AES/TEA |
| `Client::Network::Handlers` | **Handlery Protokolu** | 60 | Wyspecjalizowane handlery deserializacji pakietow serwerowych |
| `Client::Actor` | **Zarzadzanie Encjami** | 14 | Rejestr aktorow, FSM stanu, indeksacja przestrzenna i cullery widocznosci |
| `Client::Core` | **Fundamenty Silnika** | 29 | Silne typowanie (StrongTypes), PacketResult<T>, obsluga bledow i zdarzen |
| `Client::Bridge` | **Mostki Runtime** | 19 | Fasady mostka C++ <-> Python runtime, telemetria zdarzen i ochrona GIL |
| `Client::Concurrency` | **Wspolbieznosc** | 7 | Glowny dyspozytor watku (MainThreadDispatcher), ring buffery i ladowacze |
| `Client::Graphics::RHI` | **Warstwa RHI** | 7 | Hardware Abstraction Layer D3D9/Vulkan, bufory wierzcholkow i deklaracje |
| `Client::World` | **Swiat Gry** | 8 | Kontekst swiata, strefy kolizji, dynamiczna pogoda i alpha blending |
| `UserInterface::Network::Routers` | **Routery Fazy Gry** | 12 | Centralny PhaseGamePacketDispatcher oraz dedykowane routery domenowe |
| `UserInterface::Contracts` | **Interfejsy Bazowe** | 5 | Abstrakcje IPacketRouter, IGameEvents i zlacza pub-sub |
| `UserInterface::Core` | **Rdzen Prezentacji** | 13 | Globalny EventBus zdarzen, fabryki kontrolek i obsluga okien |
| `UserInterface::Domain` | **Modele GUI** | 23 | Stanowe modele widoku: ekwipunek, sklep, gildia, minimapa |
| `EterLib::Render` | **Potok Renderowania** | 80 | Dyspozytory faz renderowania (PassDispatchers), sortowanie i culling SIMD |
| `EterLib` | **Biblioteka Graficzna** | 1 | CStateManager, RAII State Guards, menedzery tekstur, kamer i czcionek |
| `EterBase` | **Podstawa Systemowa** | 28 | Silne typy (EntityVid, ItemVnum), zegary, SHA/MD5, CRC32, bezpieczna pamiec |
| `GameLib` | **Zasoby Gry** | 27 | ItemManager, RaceManager, ActorInstance, instancjonowanie modeli i animacji |
| `PackLib` | **System VFS** | - | Wirtualny system plikow VFS, ladowanie archiwow, pamiec mapowana MIO |
| `EterModelLib` | **Modele i Animacje** | 6 | Kontrolery LOD, transformacje wezlow szkieletowych i mostek glTF 2.0 |

## 2. Architektura Routerow, Dyspozytorow i Handlerow

Wzorzec dekompozycji pakietow Metin2 2026: monolit `CPythonNetworkStreamPhaseGame` zastapiono dedykowanymi routerami implementujacymi `IPacketRouter`.

### Routery Pakietow Sieciowych (PhaseGame / Opcode Routers)
| Komponent | Odpowiedzialnosc |
|---|---|
| `PhaseGamePacketDispatcher` | Centralny dyspozytor fazy gry O(1) jump table dla naglowkow pakietow |
| `NetCombatRouter` | Routing pakietow walki, obrazen, efektow pvp i celow latajacych |
| `NetItemRouter` | Routing zarzadzania ekwipunkiem, slotami, dropem z ziemi i magazynem |
| `NetActorRouter` | Routing spawnu, despawnu, aktualizacji postaci, potworow i NPC |
| `NetShopRouter` | Routing transakcji sklepowych, tablic handlowych i zakupow u NPC |
| `NetExchangeRouter` | Routing bezpiecznego handlu miedzygrazaczowego (Player Exchange) |
| `NetQuestRouter` | Routing dialogow zadan, potwiedzen questow i skryptow NPC |
| `NetPartyRouter` | Routing zaproszen do grupy, stanu czlonkow party i bonusow |
| `NetGuildRouter` | Routing zarzadzania gildia, wojen gildyjnych i magazynu |
| `ModernPacketDispatcher` | Generyczny dyspozytor opkodow pakietow binarnych ze sprawdzaniem typow |
| `IPacketRouter` | Czysty interfejs bazowy z kontraktem CanHandleHeader() i GetRouterName() |

### Domenowe Handlery Polecen (Gameplay Command Handlers)
| Komponent | Odpowiedzialnosc |
|---|---|
| `CombatCommandHandler` | Obsluga polecen ataku, uzycia czaru, zmiany celu |
| `InventoryCommandHandler` | Obsluga przesuwania, uzywania, niszczenia i rozdzielania itemow |
| `ChatCommandHandler` | Obsluga wiadomosci prywatnych, normalnych, gildyjnych i krzyku |
| `MovementCommandHandler` | Walidacja i nadawanie wektorow ruchu oraz interpolacji gracza |
| `ExchangeCommandHandler` | Wymiana handlowa: dodawanie itemu, zlota, akceptacja i anulowanie |
| `GuildCommandHandler` | Zarzadzanie uprawnieniami, zaproszeniami i wojnami |
| `PartyCommandHandler` | Formowanie druzyny, zmiana lidera i dystrybucja expa |
| `QuestCommandHandler` | Obsluga wyborow dialogowych i postepu misji fabularnych |
| `SafeboxCommandHandler` | Bezpieczny dostep do magazynu gracza i depozytu |
| `ShopCommandHandler` | Kupno/sprzedaz przedmiotow, otwieranie wlasnego tobolka |

### Binarne Kodeki Pakietow (Client::Network Codecs)
| Komponent | Odpowiedzialnosc |
|---|---|
| `ActorPacketCodec` | Serializacja / deserializacja pakietow spawnu i ruchu aktorow |
| `CombatPacketCodec` | Serializacja trafien, atakow zwyklych, obrazen i statusow |
| `ExchangePacketCodec` | Bezpieczna serializacja slotow i monet w handlu miedzygrazaczowym |
| `GuildPacketCodec` | Pakiety informacyjne gildii, rang, czlonkow i wojen |
| `ItemPacketCodec` | Aktualizacja stanu przedmiotow, socketow, atrybutow i ilosci |
| `PartyPacketCodec` | Pakiety stanu druzyny, wskaznikow zycia i zaproszen |
| `ShopPacketCodec` | Wymiana danych towarow u handlarzy i w tobolkach graczy |
| `SkillPacketCodec` | Stany poziomow umiejetnosci i czasow odnowienia (cooldown) |
| `TargetPacketCodec` | Wybor celu, synchronizacja punktow HP celu i dystansu |
| `WorldPacketCodec` | Zmiana pory dnia, pogody, wspolrzednych mapy i teleportacji |

### Dyspozytory Faz Renderowania (EterLib Render Pass Dispatchers)
| Komponent | Odpowiedzialnosc |
|---|---|
| `OpaquePassDispatcher` | Faza 1: Renderowanie nieprzezroczystej geometrii terenu i modeli |
| `AlphaTestPassDispatcher` | Faza 2: Renderowanie roslinnosci, lisci i krawedzi z alpha testing |
| `DepthPrepassDispatcher` | Faza wsteczna bufora Z zapobiegajaca overdraw pikseli |
| `AlphaBlendPassDispatcher` | Faza 3: Renderowanie przezroczystych efektow, wody i czasteczek |
| `AdditivePassDispatcher` | Faza 4: Renderowanie magii czarow i addytywnych zrodel swiatla |
| `PostProcessPassDispatcher` | Faza 5: Efekty pelnoekranowe, tonemapping i korekcja barwna |
| `UIPassDispatcher` | Faza 6: Ortograficzny rendering interfejsu uzytkownika i czcionek MSDF |

### Dyspozytory Systemowe, IPC i Szyna Zdarzen
| Komponent | Odpowiedzialnosc |
|---|---|
| `MainThreadDispatcher` | Kolejkowanie zadan z watkow roboczych do glownej petli renderera |
| `QueuedEventDispatcher` | Asynchroniczne dysponowanie zdarzeniami domenowymi |
| `IPCCommandDispatcher` | Mostek komunikacji miedzyprocesowej (symulator / headless bot) |
| `PythonEventBridge` | Rozsylanie zdarzen domenowych C++ do handlerow okien w Pythonie |
| `EventBus` | Szyna publikujaca zdarzenia (Publish-Subscribe) z zerowym sprzezeniem |

## 3. Katalog Podsystemow i Klas Kluczowych

### Client::Gameplay
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `CombatDomain` | Obsluga kalkulacji walki, przeliczanie obrazen, zasieg broni i celowanie |
| `InventoryDomain` | Logika siatki ekwipunku, walidacja rozmiarow przedmiotow i zabezpieczenie slotow |
| `SkillDomain` | Menedzer umiejetnosci, obsluga czasu odnowienia (cooldown) i poziomow M/G/P |
| `ItemBonusEngine` | Przeliczanie bonusow, wbudowanych atrybutow i wzmocnien ekwipunku |
| `QuickslotDomain` | Mapowanie skrotow klawiszowych, paskow szybkiego dostepu i makr |
| `CharacterStatsCache` | Szybki cache atrybutow HP/SP/Staminy z telemetria zmian |

### Client::Network
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `DynamicPacketFramer` | Dynamiczne ramkowanie pakietow TCP z wykrywaniem granic |
| `AesCryptoProvider` | Sprzetowa akceleracja szyfrowania AES-128/CTR strumienia sieciowego |
| `ByteStreamReader / Writer` | Bezpieczny parser binarny zero-allocation dla struktur danych |
| `CombatCommandEncoder` | Enkodowanie komend ataku i czarow do ramek sieciowych |
| `MovementCommandEncoder` | Precyzyjne pakowanie wektorow przemieszczenia i synchronizacji |

### Client::Actor
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `ActorRegistry` | Rejestr aktorow w swiecie gry z ochrona std::shared_mutex |
| `ActorSpatialIndex` | Przestrzenna siatka hashujaca dla natychmiastowych zapytan o sasiadow |
| `ActorLifecycleManager` | Maszyna stanow cyklu zycia aktora (Spawn -> Active -> Dead -> Despawn) |
| `ActorVisibilityCuller` | Wycinanie frustum dla instancji aktorow z akceleracja SIMD |

### Client::Core
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `StrongTypes` | Silne typowanie identyfikatorow: EntityVid, ItemVnum, SlotIndex |
| `PacketResult<T>` | Typ wynikowy Result z bogatym kodem bledu zamiast -1 lub nullptr |
| `DomainErrors` | Hierarchia scislych kodow bledow domenowych (Combat, Item, Trade) |
| `ZeroCopyRingBuffer` | Wielowatkowy bufor pierscieniowy bez alokacji pamieci dynamicznej |

### UserInterface
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `PythonNetworkStream` | Asynchroniczne gniazdo TCP, handshaking i delegacja do routerow |
| `PythonPlayer` | Fasada stanu gracza, synchronizacja z serwerem i okna UI |
| `PythonCharacterManager` | Zarzadzanie wizualnymi instancjami postaci w swiecie 3D |
| `PhaseGameBridge` | Cienki mostek laczacy watki sieciowe z interfejsem Pythona |

### EterLib & Render
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `CStateManager` | Deterministyczna maszyna stanow DirectX 9 z zerowym narzutem |
| `ScopedD3DStateGuard` | RAII guardy przywracajace stan potoku graficznego po zakonczeniu bloku |
| `HardwareMeshInstancing` | Masowe instancjonowanie geometrii modeli i postaci w jednym draw-callu |
| `SimdCulling` | Wielowatkowe wycinanie bryly widzenia z uzyciem instrukcji AVX2/SSE4 |

### EterModelLib & PackLib
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `GltfPipeline` | Nowoczesny potok importu modeli 3D w formacie glTF 2.0 (.glb) |
| `VfsPackManager` | Wirtualny system plikow VFS montujacy archiwa gry w pamieci RAM |
| `LODController` | Plynne przelaczanie poziomow detali modeli na podstawie odleglosci kamery |

### GameLib
| Klasa / Komponent | Przeznaczenie i Rola Architektoniczna |
|---|---|
| `CItemManager` | Zarzadzanie prototypami przedmiotow, opisami i ikonami VNUM |
| `CRaceManager` | Rejestr ras, modeli szkieletowych i zrodel animacji postaci/potworow |
| `CArea / CMapManager` | Partycjonowanie swiata 3D, zarzadzanie kafelkami terenu i woda |

## 4. Rejestr Testow Jednostkowych (CTest Matrix: 100% PASS)

Wszystkie `51` zarejestrowane zestawy CTest przechodza w 100% bez warningow.
Weryfikacja: `ctest -C Release --output-on-failure`.

| # | Nazwa Testu CTest | Plik Testowy | Obszar Testowany |
|---|---|---|---|
| 1 | `StranglerFacadeTest` | `test_c26_strangler_facade.cpp` | Symulacja & E2E |
| 2 | `PandoraSimulationTest` | `test_c26_e2e_pandora_simulation.cpp` | Symulacja & E2E |
| 3 | `StateUnificationTest` | `test_c26_state_unification.cpp` | Podsystemy Silnika |
| 4 | `MemPoolTest` | `test_c26_mem_pool.cpp` | Podsystemy Silnika |
| 5 | `RadixSort64Test` | `test_c26_radix_sort.cpp` | Rdzen & Wspolbieznosc |
| 6 | `RenderTargetMgrTest` | `test_c26_render_target_mgr.cpp` | Grafika & D3D9 |
| 7 | `SkillCooldownTest` | `test_c26_skill_cooldown.cpp` | Gameplay & Domeny |
| 8 | `SkillDomainTest` | `test_c26_skill_domain.cpp` | Gameplay & Domeny |
| 9 | `ItemBonusTest` | `test_c26_item_bonus.cpp` | Gameplay & Domeny |
| 10 | `InventoryDomainTest` | `test_c26_inventory_domain.cpp` | Gameplay & Domeny |
| 11 | `QuickslotDomainTest` | `test_c26_quickslot_domain.cpp` | Gameplay & Domeny |
| 12 | `ActorRegistryTest` | `test_c26_actor_registry.cpp` | Gameplay & Domeny |
| 13 | `SpatialHashGridTest` | `test_c26_spatial_hash_grid.cpp` | Rdzen & Wspolbieznosc |
| 14 | `ZeroCopyRingBufferTest` | `test_c26_zero_copy_ring_buffer.cpp` | Rdzen & Wspolbieznosc |
| 15 | `SimdCullingTest` | `test_c26_simd_culling.cpp` | Grafika & D3D9 |
| 16 | `WorldContextTest` | `test_c26_world_context.cpp` | Podsystemy Silnika |
| 17 | `DomainErrorsEventsTest` | `test_c26_domain_errors_events.cpp` | Gameplay & Domeny |
| 18 | `InstanceComponentsTest` | `test_c26_instance_components.cpp` | Podsystemy Silnika |
| 19 | `ZeroOverheadStateCacheTest` | `test_c26_zero_overhead_state_cache.cpp` | Grafika & D3D9 |
| 20 | `DynamicGeometryRingBufferTest` | `test_c26_dynamic_geometry_ring_buffer.cpp` | Rdzen & Wspolbieznosc |
| 21 | `InstancePhysicsEngineTest` | `test_c26_instance_physics_engine.cpp` | Podsystemy Silnika |
| 22 | `InstanceCombatEngineTest` | `test_c26_instance_combat_engine.cpp` | Podsystemy Silnika |
| 23 | `TypeSanitizationTest` | `test_c26_type_sanitization.cpp` | Rdzen & Wspolbieznosc |
| 24 | `MsdfTextBatcherTest` | `test_c26_msdf_text_batcher.cpp` | Podsystemy Silnika |
| 25 | `HardwareMeshInstancingTest` | `test_c26_hardware_mesh_instancing.cpp` | Grafika & D3D9 |
| 26 | `ProtocolPillar1Test` | `test_c26_protocol_pillar1.cpp` | Siec & Protokol |
| 27 | `ModernPacketDispatcherTest` | `test_c26_modern_packet_dispatcher.cpp` | Siec & Protokol |
| 28 | `RHIEngineTest` | `test_c26_rhi_engine.cpp` | Grafika & D3D9 |
| 29 | `PhaseGameRoutersTest` | `test_c26_phase_game_routers.cpp` | Siec & Protokol |
| 30 | `PhaseAuthRoutersTest` | `test_c33_phase_auth_routers.cpp` | Siec & Protokol |
| 31 | `PyBridgeFastCallTest` | `test_c26_pybridge_fastcall.cpp` | Podsystemy Silnika |
| 32 | `PlayerControllersTest` | `test_c26_player_controllers.cpp` | Podsystemy Silnika |
| 33 | `TerrainSubsystemTest` | `test_c26_terrain_subsystem.cpp` | Podsystemy Silnika |
| 34 | `ProcessScannerTest` | `test_c26_process_scanner.cpp` | Podsystemy Silnika |
| 35 | `MultiServerProtocolTest` | `test_c26_multi_server_protocol.cpp` | Siec & Protokol |
| 36 | `TestHarnessEngineTest` | `test_c26_test_harness_engine.cpp` | Symulacja & E2E |
| 37 | `test_c26_gltf_math` | `test_c26_gltf_math.cpp` | Grafika & D3D9 |
| 38 | `test_c26_gltf_model` | `test_c26_gltf_model.cpp` | Grafika & D3D9 |
| 39 | `test_c27_gltf_pipeline` | `test_c27_gltf_pipeline.cpp` | Grafika & D3D9 |
| 40 | `test_c28_real_simulation` | `test_c28_real_simulation.cpp` | Symulacja & E2E |
| 41 | `test_c29_gltf_d3d_bridge` | `test_c29_gltf_d3d_bridge.cpp` | Grafika & D3D9 |
| 42 | `D3D9StateGuardsTest` | `test_c30_d3d9_state_guards.cpp` | Grafika & D3D9 |
| 43 | `BeaviumServerProfileTest` | `test_c31_beavium_server_profile.cpp` | Podsystemy Silnika |
| 44 | `AgentMCPBridgeTest` | `test_c32_agent_mcp_bridge.cpp` | Podsystemy Silnika |
| 45 | `BotFFIBridgeTest` | `test_c33_bot_ffi_bridge.cpp` | Podsystemy Silnika |
| 46 | `DebugRendererTest` | `test_c31_debug_renderer.cpp` | Grafika & D3D9 |
| 47 | `UniversalProtocolDriversTest` | `test_c34_universal_protocol_drivers.cpp` | Siec & Protokol |
| 48 | `CryptoProvidersTest` | `test_c35_crypto_providers.cpp` | Podsystemy Silnika |
| 49 | `AITelemetryTest` | `test_ai_telemetry.cpp` | Podsystemy Silnika |
| 50 | `McpHarnessSelfTest` | `Python3::Interpreter` | Podsystemy Silnika |
| 51 | `HeadlessClientLiveTest` | `Python3::Interpreter` | Podsystemy Silnika |

### Podsumowanie Pokrycia Testowego per Domena (256 Plikow Testowych)
| Domena / Kategoria | Testy CTest | Pliki Testowe | Zakres Weryfikacji |
|---|---|---|---|
| **Podsystemy Silnika** | `18` CTests | `136` plikow | Testy integracyjne wyspecjalizowanych modulow klienta |
| **Grafika & D3D9** | `11` CTests | `37` plikow | RAII guardy stanow D3D9, potok glTF 2.0 i buforowanie geometrii |
| **Gameplay & Domeny** | `7` CTests | `43` plikow | Testy czystej logiki biznesowej, reguly gry i ekwipunek |
| **Siec & Protokol** | `6` CTests | `36` plikow | Kodeki binarne, walidatory naglowkow i dispatchery fazy gry |
| **Rdzen & Wspolbieznosc** | `5` CTests | `6` plikow | Zero-copy buffery, indeksacja przestrzenna i alokatory |
| **Symulacja & E2E** | `4` CTests | `6` plikow | Zintegrowany harness E2E, scenariusze gry i fasady Stranglera |

---
*Koniec dokumentu REPO_MAP.md. Plik spelnia scisly limit rozmiaru do 25 KB dla modeli LLM.*
