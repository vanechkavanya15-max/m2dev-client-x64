cmake_minimum_required(VERSION 3.19)

file(GLOB TEST_SOURCES "test_c26_*.cpp")

add_executable(ClientTests
    test_main.cpp
    ${TEST_SOURCES}
)

set_target_properties(ClientTests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    FOLDER "tests"
)

target_include_directories(ClientTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_includes
    ${CMAKE_SOURCE_DIR}/extern/include
    ${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
    ${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
)

target_link_libraries(ClientTests PRIVATE
    Client2026
    EterBase
    libzstd_static
    sodium
    lzo2
    mio
    ws2_32
)

enable_testing()
add_test(NAME ClientTests COMMAND ClientTests)
