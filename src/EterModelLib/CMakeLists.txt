file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(EterModelLib STATIC ${FILE_SOURCES})

target_include_directories(EterModelLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/include
)

GroupSourcesByFolder(EterModelLib)
