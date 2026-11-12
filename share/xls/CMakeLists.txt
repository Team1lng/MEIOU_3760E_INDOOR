
add_definitions(
	-D__LITTLE_ENDIAN__
	-D_GNU_SOURCE
)

include_directories(
	.
	${CMAKE_CURRENT_SOURCE_DIR}
)
    
file(GLOB_RECURSE SOURCES ./*.c )
add_library(xls OBJECT ${SOURCES} )