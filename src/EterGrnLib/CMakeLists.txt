file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(EterGrnLib STATIC ${FILE_SOURCES})

target_include_directories(EterGrnLib PUBLIC
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/extern/include
)

target_link_libraries(EterGrnLib
	EterModelLib
	mio
)

GroupSourcesByFolder(EterGrnLib)

