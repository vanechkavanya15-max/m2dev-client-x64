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
add_test(NAME ActorRegistryTest COMMAND test_c26_actor_registry)
add_test(NAME SpatialHashGridTest COMMAND test_c26_spatial_hash_grid)
add_test(NAME ZeroCopyRingBufferTest COMMAND test_c26_zero_copy_ring_buffer)


