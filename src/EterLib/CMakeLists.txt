file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")
list(FILTER FILE_SOURCES EXCLUDE REGEX "NullRHI/.*")

add_library(EterLib STATIC ${FILE_SOURCES})

target_link_libraries(EterLib
	sodium
	mio
	freetype
)

GroupSourcesByFolder(EterLib)
