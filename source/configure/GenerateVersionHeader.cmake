# Generate version header at compile time.

INCLUDE( MacroDefinitions )

# Work out intermediate directory based on Makefile generator and location of version executable
# From location of version executable
STRING( REGEX REPLACE "/[^/]+$" "" VERSION_EXE_PATH ${VERSION_EXE_LOCATION} )
STRING( REGEX MATCH "[^/]+$" INTDIR ${VERSION_EXE_PATH} )
STRING( TOLOWER ${INTDIR} LOWERCASE_CURRENT_BUILD_TYPE )
Subversion_GET_REVISION( ${FILE_IN_CMGUI_SVN_REPOSITORY} SVN_REVISION )
EXECUTE_PROCESS(COMMAND ${VERSION_EXE_LOCATION}
    OUTPUT_VARIABLE DATE_TIME_STRING OUTPUT_STRIP_TRAILING_WHITESPACE)

SET( VERSION_HEADER_STRING 
"

#if !defined GENERATED_VERSION_H
#define GENERATED_VERSION_H

#define CMISS_NAME_STRING \"CMISS(cmgui)\"
#define CMISS_VERSION_STRING \"${CMGUI_MAJOR_VERSION}.${CMGUI_MINOR_VERSION}.${CMGUI_PATCH_VERSION}\"
#define CMISS_DATE_STRING \"${DATE_TIME_STRING}\"
#define CMISS_COPYRIGHT_STRING \"Copyright 1996-2010, Auckland UniServices Ltd.\"
#define CMISS_SVN_REVISION_STRING \"${SVN_REVISION}\"
#define CMISS_BUILD_STRING \"${CMAKE_SYSTEM_PROCESSOR} ${OPERATING_SYSTEM} ${CMGUI_USER_INTERFACE} ${LOWERCASE_CURRENT_BUILD_TYPE}\"

#endif

" )

FILE( WRITE ${CONFIGURED_VERSION_HEADER} ${VERSION_HEADER_STRING} )
MESSAGE( STATUS "Build information: ${CMAKE_SYSTEM_PROCESSOR} ${OPERATING_SYSTEM} ${CMGUI_USER_INTERFACE} ${LOWERCASE_CURRENT_BUILD_TYPE} ${SVN_REVISION}" )

