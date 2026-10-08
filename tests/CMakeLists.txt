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

enable_testing()
add_test(NAME StranglerFacadeTest COMMAND test_c26_strangler_facade)
add_test(NAME PandoraSimulationTest COMMAND test_c26_e2e_pandora_simulation)
add_test(NAME StateUnificationTest COMMAND test_c26_state_unification)
add_test(NAME MemPoolTest COMMAND test_c26_mem_pool)

