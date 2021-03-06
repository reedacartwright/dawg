SET(DAWG_LIB_SOURCES
	ma.cpp
	matic.cpp
	output.cpp
	mutt.cpp
	parse.cpp
	models.cpp
	${RANDOM_GEN_FILES_X}
	"${CMAKE_CURRENT_BINARY_DIR}/../include/dawg/details/config.h"
)

SET(PACKAGE_STRING "${NEW_PACKAGE_NAME} ${NEW_PACKAGE_VERSION}")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../include/dawg/details/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/../include/dawg/details/config.h)

if(WIN32 AND NOT UNIX)
# shared libray is not configured for windows, so don't build it.
else()
ADD_LIBRARY(libdawg2 SHARED ${DAWG_LIB_SOURCES})
SET_TARGET_PROPERTIES(libdawg2 PROPERTIES
	OUTPUT_NAME "dawg2"
	SOVERSION 1
)
INSTALL(TARGETS libdawg2
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	${PROJ_INSTALL_EXTRA_ARGS})
endif()

ADD_LIBRARY(libdawg2-static STATIC ${DAWG_LIB_SOURCES})
SET_TARGET_PROPERTIES(libdawg2-static PROPERTIES
	OUTPUT_NAME "dawg2"
	PREFIX "lib"
)
INSTALL(TARGETS libdawg2-static
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	${PROJ_INSTALL_EXTRA_ARGS})
