# OpenCMISS-Cmgui Application
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

FUNCTION(SVN_GET_REVISION DIR RESULT)
	# Determine the svn revision of the source in the given directory.
	FIND_PROGRAM(SVNVERSION_EXECUTABLE svnversion
		DOC "subversion svnversion command line application.")
	IF( NOT SVNVERSION_EXECUTABLE )
		FIND_PROGRAM( TORTOISESVN_EXECUTABLE SubWCRev
			DOC "Tortiose SVN Working Copy client" )
	ENDIF( NOT SVNVERSION_EXECUTABLE )
	IF( SVNVERSION_EXECUTABLE )
		EXECUTE_PROCESS(COMMAND ${SVNVERSION_EXECUTABLE} ${DIR}
			OUTPUT_VARIABLE VARIABLE
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		STRING( REPLACE ":" "-" VARIABLE ${VARIABLE} )
		SET( ${RESULT} ${VARIABLE} PARENT_SCOPE )
	ELSEIF( TORTOISESVN_EXECUTABLE )
		EXECUTE_PROCESS(COMMAND ${TORTOISESVN_EXECUTABLE} ${DIR}
			OUTPUT_VARIABLE ${VARIABLE}
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		SET( MAX_NUMBER 0 )
		SEPARATE_ARGUMENTS(RESPONSE_LIST WINDOWS_COMMAND "${${VARIABLE}}")
		#MESSAGE( STATUS "TORTOISE SVN RESPONSE: ${RESPONSE_LIST}" )
		FOREACH( RESPONSE_ITEM ${RESPONSE_LIST} )
			STRING( REGEX MATCH "^[0-9]+$" HAVE_NUMBER ${RESPONSE_ITEM} )
			STRING( REGEX MATCH "^[0-9]+:([0-9]+)$" HAVE_NUMBER_RANGE ${RESPONSE_ITEM} )
			IF( HAVE_NUMBER_RANGE )
				SET( HAVE_NUMBER ${CMAKE_MATCH_1} )
			ENDIF( HAVE_NUMBER_RANGE )
			IF( HAVE_NUMBER )
				IF( ${MAX_NUMBER} LESS ${HAVE_NUMBER} )
					SET( MAX_NUMBER ${HAVE_NUMBER} )
				ENDIF( ${MAX_NUMBER} LESS ${HAVE_NUMBER} )
			ENDIF( HAVE_NUMBER )
		ENDFOREACH( RESPONSE_ITEM ${RESPONSE_LIST} )
		SET( ${VARIABLE} ${MAX_NUMBER} )
	ENDIF( SVNVERSION_EXECUTABLE )
ENDFUNCTION(SVN_GET_REVISION)

FUNCTION( GET_CMGUI_SYSTEM_NAME RETURN_SYSTEM_NAME )
	IF( WIN32 )
		SET( SYSTEM_NAME "Windows" )
	ELSEIF( APPLE )
		EXECUTE_PROCESS(COMMAND sw_vers -productName RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE PRODUCT_NAME )
		STRING( STRIP ${PRODUCT_NAME} PRODUCT_NAME )
		EXECUTE_PROCESS(COMMAND sw_vers -productVersion RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE PRODUCT_VERSION )
		STRING( STRIP ${PRODUCT_VERSION} PRODUCT_VERSION )
		STRING( REPLACE " " "-" SYSTEM_NAME "${PRODUCT_NAME} ${PRODUCT_VERSION}" )
	ELSEIF( UNIX )
		SET( SYSTEM_NAME "Linux" )
		EXECUTE_PROCESS(COMMAND cat /etc/issue RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE COMMAND_OUTPUT ERROR_VARIABLE ERROR_OUTPUT)
	IF( COMMAND_RESULT EQUAL 0 )
		STRING( REGEX MATCH "^[^\\]*" COMMAND_OUTPUT ${COMMAND_OUTPUT} )
		STRING( STRIP ${COMMAND_OUTPUT} COMMAND_OUTPUT )
			message(STATUS "comm out: ${COMMAND_OUTPUT}" )
			STRING( REPLACE " " "-" SYSTEM_NAME ${COMMAND_OUTPUT} )
			STRING( REPLACE "\n" " " SYSTEM_NAME ${SYSTEM_NAME} )
			STRING( REPLACE "(" "'('" SYSTEM_NAME ${SYSTEM_NAME} )
			STRING( REPLACE ")" "')'" SYSTEM_NAME ${SYSTEM_NAME} )
			
		ENDIF()
	ELSE()
		MESSAGE( FATAL_ERROR "Don't yet know this system." )
	ENDIF()

	SET( ${RETURN_SYSTEM_NAME} ${SYSTEM_NAME} PARENT_SCOPE)
ENDFUNCTION()

