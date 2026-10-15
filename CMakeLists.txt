cmake_minimum_required(VERSION 3.19)

project(m2dev-client-src)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ASan support
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/buildtool"
)

include(Utils)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Configurations" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo,Release>:ProgramDatabase>")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

find_program(SCCACHE_PROGRAM sccache)
if(SCCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "sccache enabled: ${SCCACHE_PROGRAM}")
endif()

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/FS>")

if(MSVC)
	add_compile_definitions(UNICODE _UNICODE)
endif()

add_compile_options(
	$<$<CXX_COMPILER_ID:MSVC>:/wd4828>
	$<$<CXX_COMPILER_ID:MSVC>:/wd4996>
	$<$<CXX_COMPILER_ID:MSVC>:/wd5033>
)

# ASan flags
if(ENABLE_ASAN)
	if(MSVC)
		add_compile_options(/fsanitize=address)
		add_link_options(/fsanitize=address)
		add_definitions(-D_DISABLE_VECTOR_ANNOTATION)
		add_definitions(-D_DISABLE_STRING_ANNOTATION)
	else()
		add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
		add_link_options(-fsanitize=address)
	endif()
endif()

add_definitions(-DNOMINMAX)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
# Flaga WINDOWS_IGNORE_PACKING_MISMATCH jest niezbedna przy kompilacji pod MSVC na platformie x64.
# Naglowki Windows SDK (winnt.h) weryfikuja domyslne wyrownanie struktur (8 bajtow na x64) i zglaszaja
# blad krytyczny kompilatora error C2338 w przypadku wykrycia niestandardowego upakowania (packing mismatch).
# W architekturze klienta Metin2 moduly silnikowe, sieciowe i graficzne:
# - UserInterface (struktury pakietow GameType.h, Network/Handlers, Packet.h)
# - Client/Network (sterowniki protokolu, kodery pakietow sieciowych o scislym wyrownaniu 1-bajtowym)
# - GameLib oraz PRTerrainLib (definicje wierzcholkow terenu TerrainPatch.h, wierzcholki Direct3D 9)
# - EterImageLib (naglowki plikow graficznych TGA_HEADER w Image.h)
# - EterLib oraz DumpProto
# intensywnie korzystaja z dyrektyw #pragma pack(1) lub #pragma pack(push, 1) w celu zagwarantowania
# binarnej kompatybilnosci struktur sieciowych i formatow danych z protokolem serwera gry Metin2.
# Gdy naglowki Windows sa dolaczane w jednostkach translacji operujacych na spakowanych strukturach,
# flaga ta wylacza asercje C2338 w Windows SDK i zapobiega przerwaniu procesu kompilacji.
add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)

add_definitions(-DUSE_LOD)
add_definitions(-DDUNGEON_WORK)
add_definitions(-DBUILDING_GRANNY_STATIC)
add_definitions(-DPy_NO_ENABLE_SHARED)

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG>")
add_compile_definitions("$<$<CONFIG:RelWithDebInfo>:_DISTRIBUTE>")
add_compile_definitions("$<$<CONFIG:Release>:_DISTRIBUTE>")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/extern/include")
include_directories("${CMAKE_SOURCE_DIR}/vendor/libsodium/src/libsodium/include")
include_directories("${CMAKE_SOURCE_DIR}/vendor/freetype-2.13.3/include")
include_directories("${CMAKE_SOURCE_DIR}/vendor/zstd-1.5.7/lib")

# Add subdirectories for libraries and executables
add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(extern)

enable_testing()
add_subdirectory(tests)
add_subdirectory(tools/gr2_to_glb)
