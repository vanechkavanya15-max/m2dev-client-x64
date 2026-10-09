file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_executable(UserInterface WIN32 ${FILE_SOURCES} ${CMAKE_SOURCE_DIR}/src/UserInterface/UserInterface.rc)
set_target_properties(UserInterface PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'"
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

	lzo2
	libzstd_static
	mio
	
	DirectX
	Granny
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
