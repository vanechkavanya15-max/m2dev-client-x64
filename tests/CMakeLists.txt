cmake_minimum_required(VERSION 3.19)

add_executable(test_c26_strangler_facade test_c26_strangler_facade.cpp)

target_include_directories(test_c26_strangler_facade PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
    ${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
    ${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
)

target_link_libraries(test_c26_strangler_facade PRIVATE
    Client2026
    EterLib
    EterBase
    libzstd_static
    sodium
    lzo2
    mio
    ws2_32
)

add_executable(test_c26_e2e_pandora_simulation test_c26_e2e_pandora_simulation.cpp)

target_include_directories(test_c26_e2e_pandora_simulation PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
    ${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
    ${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
)

target_link_libraries(test_c26_e2e_pandora_simulation PRIVATE
    Client2026
    EterLib
    EterBase
    libzstd_static
    sodium
    lzo2
    mio
    ws2_32
)

add_executable(test_c26_state_unification test_c26_state_unification.cpp)

target_include_directories(test_c26_state_unification PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
    ${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
    ${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
)

target_link_libraries(test_c26_state_unification PRIVATE
    Client2026
    EterLib
    EterBase
    libzstd_static
    sodium
    lzo2
    mio
    ws2_32
)

add_executable(test_c26_mem_pool test_c26_mem_pool.cpp)

target_include_directories(test_c26_mem_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
    ${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
    ${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
)

target_link_libraries(test_c26_mem_pool PRIVATE
    EterLib
    EterBase
    sodium
    mio
    ws2_32
)

add_executable(test_c26_radix_sort test_c26_radix_sort.cpp)
target_include_directories(test_c26_radix_sort PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_link_libraries(test_c26_radix_sort PRIVATE
    EterLib
    EterBase
)

add_executable(test_c26_render_target_mgr test_c26_render_target_mgr.cpp)
target_include_directories(test_c26_render_target_mgr PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_link_libraries(test_c26_render_target_mgr PRIVATE
    EterLib
    EterBase
)

add_executable(test_c26_skill_cooldown test_c26_skill_cooldown.cpp)
target_include_directories(test_c26_skill_cooldown PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_skill_cooldown PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_item_bonus test_c26_item_bonus.cpp)
target_include_directories(test_c26_item_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_item_bonus PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_skill_domain test_c26_skill_domain.cpp)
target_include_directories(test_c26_skill_domain PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_skill_domain PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_inventory_domain test_c26_inventory_domain.cpp)
target_include_directories(test_c26_inventory_domain PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_inventory_domain PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_actor_registry test_c26_actor_registry.cpp)
target_include_directories(test_c26_actor_registry PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_actor_registry PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_spatial_hash_grid test_c26_spatial_hash_grid.cpp)
target_include_directories(test_c26_spatial_hash_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_spatial_hash_grid PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_zero_copy_ring_buffer test_c26_zero_copy_ring_buffer.cpp)
target_include_directories(test_c26_zero_copy_ring_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_zero_copy_ring_buffer PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_quickslot_domain test_c26_quickslot_domain.cpp)
target_include_directories(test_c26_quickslot_domain PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_quickslot_domain PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_simd_culling test_c26_simd_culling.cpp)
target_include_directories(test_c26_simd_culling PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_simd_culling PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_world_context test_c26_world_context.cpp)
target_include_directories(test_c26_world_context PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_c26_world_context PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_domain_errors_events test_c26_domain_errors_events.cpp)
target_include_directories(test_c26_domain_errors_events PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_c26_domain_errors_events PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_instance_components 
    test_c26_instance_components.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/InstanceComponents/InstanceCombatComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/AffectFlagContainer.cpp
)
target_include_directories(test_c26_instance_components PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_SOURCE_DIR}/src/UserInterface
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_c26_instance_components PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_zero_overhead_state_cache 
    test_c26_zero_overhead_state_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/Client/Graphics/ZeroOverheadStateCache.cpp
)
target_include_directories(test_c26_zero_overhead_state_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)

add_executable(test_c26_dynamic_geometry_ring_buffer test_c26_dynamic_geometry_ring_buffer.cpp)
target_include_directories(test_c26_dynamic_geometry_ring_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_c26_dynamic_geometry_ring_buffer PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_instance_physics_engine test_c26_instance_physics_engine.cpp)
target_include_directories(test_c26_instance_physics_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(test_c26_instance_physics_engine PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_instance_combat_engine test_c26_instance_combat_engine.cpp)
target_include_directories(test_c26_instance_combat_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(test_c26_instance_combat_engine PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_type_sanitization test_c26_type_sanitization.cpp)
target_include_directories(test_c26_type_sanitization PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(test_c26_type_sanitization PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_msdf_text_batcher test_c26_msdf_text_batcher.cpp)
target_include_directories(test_c26_msdf_text_batcher PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(test_c26_msdf_text_batcher PRIVATE
    Client2026
    EterBase
)

add_executable(test_c26_hardware_mesh_instancing test_c26_hardware_mesh_instancing.cpp)
target_include_directories(test_c26_hardware_mesh_instancing PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_SOURCE_DIR}/src/GameLib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(test_c26_hardware_mesh_instancing PRIVATE
    Client2026
    GameLib
    EterBase
)

enable_testing()
add_test(NAME StranglerFacadeTest COMMAND test_c26_strangler_facade)
add_test(NAME PandoraSimulationTest COMMAND test_c26_e2e_pandora_simulation)
add_test(NAME StateUnificationTest COMMAND test_c26_state_unification)
add_test(NAME MemPoolTest COMMAND test_c26_mem_pool)
add_test(NAME RadixSort64Test COMMAND test_c26_radix_sort)
add_test(NAME RenderTargetMgrTest COMMAND test_c26_render_target_mgr)
add_test(NAME SkillCooldownTest COMMAND test_c26_skill_cooldown)
add_test(NAME SkillDomainTest COMMAND test_c26_skill_domain)
add_test(NAME ItemBonusTest COMMAND test_c26_item_bonus)
add_test(NAME InventoryDomainTest COMMAND test_c26_inventory_domain)
add_test(NAME QuickslotDomainTest COMMAND test_c26_quickslot_domain)
add_test(NAME ActorRegistryTest COMMAND test_c26_actor_registry)
add_test(NAME SpatialHashGridTest COMMAND test_c26_spatial_hash_grid)
add_test(NAME ZeroCopyRingBufferTest COMMAND test_c26_zero_copy_ring_buffer)
add_test(NAME SimdCullingTest COMMAND test_c26_simd_culling)
add_test(NAME WorldContextTest COMMAND test_c26_world_context)
add_test(NAME DomainErrorsEventsTest COMMAND test_c26_domain_errors_events)
add_test(NAME InstanceComponentsTest COMMAND test_c26_instance_components)
add_test(NAME ZeroOverheadStateCacheTest COMMAND test_c26_zero_overhead_state_cache)
add_test(NAME DynamicGeometryRingBufferTest COMMAND test_c26_dynamic_geometry_ring_buffer)
add_test(NAME InstancePhysicsEngineTest COMMAND test_c26_instance_physics_engine)
add_test(NAME InstanceCombatEngineTest COMMAND test_c26_instance_combat_engine)
add_test(NAME TypeSanitizationTest COMMAND test_c26_type_sanitization)
add_test(NAME MsdfTextBatcherTest COMMAND test_c26_msdf_text_batcher)
add_test(NAME HardwareMeshInstancingTest COMMAND test_c26_hardware_mesh_instancing)

add_executable(test_c26_protocol_pillar1 test_c26_protocol_pillar1.cpp)
target_include_directories(test_c26_protocol_pillar1 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_protocol_pillar1 PRIVATE
    Client2026
    EterBase
)

add_test(NAME ProtocolPillar1Test COMMAND test_c26_protocol_pillar1)

add_executable(test_c26_rhi_engine test_c26_rhi_engine.cpp)
target_include_directories(test_c26_rhi_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_c26_rhi_engine PRIVATE
    Client2026
    EterBase
)

add_test(NAME RHIEngineTest COMMAND test_c26_rhi_engine)

add_executable(test_c26_phase_game_routers
    test_c26_phase_game_routers.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/Network/Routers/NetCombatRouter.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/Network/Routers/NetItemRouter.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/Network/Routers/NetActorRouter.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/Network/Routers/PhaseGamePacketDispatcher.cpp
)
target_include_directories(test_c26_phase_game_routers PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/UserInterface
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(test_c26_phase_game_routers PRIVATE
    TEST_MODE_DISABLE_STDAFX
)
target_link_libraries(test_c26_phase_game_routers PRIVATE
    EterBase
)
add_test(NAME PhaseGameRoutersTest COMMAND test_c26_phase_game_routers)

add_executable(test_c26_pybridge_fastcall test_c26_pybridge_fastcall.cpp)
target_include_directories(test_c26_pybridge_fastcall PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_link_libraries(test_c26_pybridge_fastcall PRIVATE
    Client2026
    Python
    EterBase
    version
    ws2_32
)

add_test(NAME PyBridgeFastCallTest COMMAND test_c26_pybridge_fastcall)


add_executable(test_c26_player_controllers
    test_c26_player_controllers.cpp
    player_controller_mocks.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerMovementController.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerCombatController.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerTargetController.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerSkillExecutor.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerItemController.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/PlayerControllers/PlayerPKController.cpp
)
target_include_directories(test_c26_player_controllers PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/UserInterface
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_compile_definitions(test_c26_player_controllers PRIVATE
    TEST_MODE_DISABLE_STDAFX
)
target_link_libraries(test_c26_player_controllers PRIVATE
    Client2026
    EterBase
    EterLib
    GameLib
    PackLib
)
add_test(NAME PlayerControllersTest COMMAND test_c26_player_controllers)


add_executable(test_c26_terrain_subsystem
    test_c26_terrain_subsystem.cpp
)
target_include_directories(test_c26_terrain_subsystem PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/GameLib
    ${CMAKE_SOURCE_DIR}/src/GameLib/Terrain
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_link_libraries(test_c26_terrain_subsystem PRIVATE
    GameLib
    EterBase
    lzo2
    mio
    ws2_32
)
add_test(NAME TerrainSubsystemTest COMMAND test_c26_terrain_subsystem)

add_executable(test_c26_process_scanner
    test_c26_process_scanner.cpp
    ${CMAKE_SOURCE_DIR}/src/UserInterface/ProcessScanner.cpp
)
target_include_directories(test_c26_process_scanner PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/UserInterface
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)
target_compile_definitions(test_c26_process_scanner PRIVATE
    TEST_MODE_DISABLE_STDAFX
)
target_link_libraries(test_c26_process_scanner PRIVATE
    EterBase
    ws2_32
)
add_test(NAME ProcessScannerTest COMMAND test_c26_process_scanner)

