# Modular InstanceBase Architecture:
# - InstanceBase.cpp (Core entity lifecycle, state loop)
# - InstanceBaseAppearance.cpp (Armor, hair, weapons, parts, refine effects, sashes/wings)
# - InstanceBaseMount.cpp (Mount/horse boarding, gallop, race sync)
# - InstanceBaseTransform.cpp, InstanceBaseBattle.cpp, InstanceBaseMotion.cpp, etc.
#
# Modular CPythonPlayer Architecture (Domain Sharding):
# - PythonPlayer.cpp (Core lifecycle, singleton, clear, init, main actor)
# - PythonPlayer_Combat.cpp (Attack target, combat calculations, attack state delegation)
# - PythonPlayer_Inventory.cpp (Inventory slots, belt inventory, item moving, drag & drop)
# - PythonPlayer_Stats.cpp (Status points, levels, Yang, Exp, stamina, pk mode)
# - PythonPlayer_Skills.cpp (Skill slots, cooltimes, levels, efficient percentages)
file(GLOB_RECURSE FILE_SOURCES CONFIGURE_DEPENDS "*.h" "*.c" "*.cpp")

set(PYTHON_PLAYER_DOMAIN_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/PythonPlayer_Combat.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PythonPlayer_Inventory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PythonPlayer_Stats.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PythonPlayer_Skills.cpp
)
list(APPEND FILE_SOURCES ${PYTHON_PLAYER_DOMAIN_SOURCES})
set(DEBUG_RENDER_MODULE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/PythonDebugRenderModule.h
	${CMAKE_CURRENT_SOURCE_DIR}/PythonDebugRenderModule.cpp
)
list(APPEND FILE_SOURCES ${DEBUG_RENDER_MODULE_SOURCES})
list(REMOVE_DUPLICATES FILE_SOURCES)

add_executable(UserInterface WIN32 ${FILE_SOURCES} ${CMAKE_SOURCE_DIR}/src/UserInterface/UserInterface.rc)
set_target_properties(UserInterface PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false' /LTCG"
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	OUTPUT_NAME_DEBUG "Metin2_Debug"
	OUTPUT_NAME_RELEASE "Metin2_Release"
	OUTPUT_NAME_RELWITHDEBINFO "Metin2_RelWithDebInfo"
)

target_link_libraries(UserInterface 
	AudioLib
	Discord
	EffectLib 
	EterBase 
	EterGrnLib 
	EterModelLib
	EterImageLib 
	EterLib
	EterLocale
	EterPythonLib
	GameLib
	PRTerrainLib
	PythonModules
	ScriptLib
	SpeedTreeLib
	SphereLib
	PackLib
	Client2026
	imgui

	lzo2
	libzstd_static
	mio
	
	DirectX
	SpeedTree
	Python
	WebView
	
	ws2_32 
	strmiids
	amstrmid
	dmoguids
	ddraw
	version
	Dbghelp
)

target_include_directories(UserInterface PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/Client
	${CMAKE_SOURCE_DIR}/extern/imgui
	${CMAKE_SOURCE_DIR}/extern/imgui/backends
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/Core
	${CMAKE_CURRENT_SOURCE_DIR}/Services
	${CMAKE_CURRENT_SOURCE_DIR}/Actors
	${CMAKE_CURRENT_SOURCE_DIR}/ECS
	${CMAKE_CURRENT_SOURCE_DIR}/Network
	${CMAKE_CURRENT_SOURCE_DIR}/Network/Handlers
	${CMAKE_CURRENT_SOURCE_DIR}/Network/Senders
	${CMAKE_CURRENT_SOURCE_DIR}/Network/Dispatchers
	${CMAKE_CURRENT_SOURCE_DIR}/Network/Routers
	${CMAKE_CURRENT_SOURCE_DIR}/Network/Actors
	${CMAKE_CURRENT_SOURCE_DIR}/InstanceControllers
	${CMAKE_CURRENT_SOURCE_DIR}/InstanceComponents
	${CMAKE_CURRENT_SOURCE_DIR}/ECS/SIMD
	${CMAKE_CURRENT_SOURCE_DIR}/TextTail
	${CMAKE_CURRENT_SOURCE_DIR}/GroundDrop
	${CMAKE_CURRENT_SOURCE_DIR}/PlayerControllers
	${CMAKE_CURRENT_SOURCE_DIR}/TestHarness
)

GroupSourcesByFolder(UserInterface)

add_custom_command(TARGET UserInterface POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:UserInterface>
		${CMAKE_SOURCE_DIR}/m2dev-client-main/Metin2.exe
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:UserInterface>
		${CMAKE_SOURCE_DIR}/m2dev-client-main/$<TARGET_FILE_NAME:UserInterface>
	COMMENT "Deploying binary to m2dev-client-main runtime directory..."
)
