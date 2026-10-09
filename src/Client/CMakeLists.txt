file(GLOB_RECURSE FILE_SOURCES CONFIGURE_DEPENDS "*.h" "*.c" "*.cpp")

add_library(Client2026 STATIC ${FILE_SOURCES})

target_link_libraries(Client2026
	DirectX
	Python
	EterBase
	libzstd_static
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
	${CMAKE_SOURCE_DIR}/extern/include/python
	${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib
	${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include
)

target_compile_features(Client2026 PUBLIC cxx_std_23)

GroupSourcesByFolder(Client2026)
