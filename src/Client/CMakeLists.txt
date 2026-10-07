file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(Client2026 STATIC ${FILE_SOURCES})

target_link_libraries(Client2026
	DirectX
	Python
	EterBase
)

target_include_directories(Client2026 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Graphics
	${CMAKE_CURRENT_SOURCE_DIR}/Network
	${CMAKE_CURRENT_SOURCE_DIR}/World
	${CMAKE_CURRENT_SOURCE_DIR}/Gameplay
	${CMAKE_CURRENT_SOURCE_DIR}/UI
	${CMAKE_CURRENT_SOURCE_DIR}/Platform
	${CMAKE_SOURCE_DIR}/src/UserInterface
	${CMAKE_SOURCE_DIR}/src/GameLib
	${CMAKE_SOURCE_DIR}/src/EterLib
	${CMAKE_SOURCE_DIR}/src/EterBase
	${CMAKE_SOURCE_DIR}/extern/include
)

GroupSourcesByFolder(Client2026)
