file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(Client2026 STATIC ${CLIENT_SOURCES})

target_include_directories(Client2026 PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Client
    ${CMAKE_SOURCE_DIR}/extern/include
)

target_compile_features(Client2026 PUBLIC cxx_std_20)
