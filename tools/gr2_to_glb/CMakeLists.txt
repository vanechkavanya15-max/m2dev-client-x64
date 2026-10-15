project(gr2_to_glb)

set(SOURCES
    main.cpp
    GrannyExtractor.cpp
    GrannyExtractor.h
    GlbWriter.cpp
    GlbWriter.h
)

add_executable(gr2_to_glb ${SOURCES})

target_include_directories(gr2_to_glb PRIVATE
    ${CMAKE_SOURCE_DIR}/extern/include
)

target_link_libraries(gr2_to_glb PRIVATE
    ${CMAKE_SOURCE_DIR}/extern/library/Granny/granny2_static.lib
)
