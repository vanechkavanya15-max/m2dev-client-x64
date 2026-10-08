file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(GameLib STATIC ${FILE_SOURCES})

target_link_libraries(GameLib
	lzo2
	mio
)

target_include_directories(GameLib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/Terrain
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/Client
	${CMAKE_SOURCE_DIR}/src/Client/Graphics
	${CMAKE_SOURCE_DIR}/extern/include
)

GroupSourcesByFolder(GameLib)
