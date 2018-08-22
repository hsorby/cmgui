
# OpenCMISS-Cmgui Application
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


cmake_minimum_required(VERSION 3.5.0)

project(Cmgui VERSION 4.0.0 LANGUAGES CXX)

# Check bare minimum configuration
set(CMGUI_CMAKE_MODULE_PATH "${CMGUI_CMAKE_MODULE_PATH}" CACHE STRING "Location of OpenCMISS CMake modules.")
if (NOT EXISTS "${CMGUI_CMAKE_MODULE_PATH}/OpenCMISS/OCMiscFunctions.cmake")
    if (NOT "${CMGUI_CMAKE_MODULE_PATH}")
        set(_SET_CMGUI_MODULE_PATH_STRING " Try setting the configuration variable CMGUI_CMAKE_MODULE_PATH.")
    endif ()
    message(FATAL_ERROR "'${CMGUI_CMAKE_MODULE_PATH}/OpenCMISS/OCMiscFunctions.cmake' does not exists.  The OpenCMISS CMake modules may not be installed/available.${_SET_CMGUI_MODULE_PATH_STRING}")
endif ()

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMGUI_CMAKE_MODULE_PATH}/FindModuleWrappers")
list(APPEND CMAKE_MODULE_PATH "${CMGUI_CMAKE_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH "${CMGUI_CMAKE_MODULE_PATH}/OpenCMISS")

include(FunctionDefinitions)

set(CMGUI_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Where do you want your Cmgui libraries installed?")
set(CMAKE_INSTALL_PREFIX ${CMGUI_INSTALL_PREFIX} CACHE INTERNAL "Internalise CMAKE_INSTALL_PREFIX, manipulate via CMGUI_INSTALL_PREFIX" FORCE)
if (NOT CMAKE_CONFIGURATION_TYPES)
    set(CMGUI_BUILD_TYPE "Release" CACHE STRING "What configuration do you want to build: Debug, Release, MinSizeRel, RelWithDebugInfo")
    set(CMAKE_BUILD_TYPE ${CMGUI_BUILD_TYPE} CACHE INTERNAL "Internalise CMAKE_BUILD_TYPE, manipulate via CMGUI_BUILD_TYPE" FORCE)
endif ()

option(CMGUI_USE_PERL_INTERPRETER "Do you want to use the perl interpreter?" TRUE)
option(CMGUI_OFFICIAL_RELEASE "Set to TRUE if creating an official release." FALSE)

set(FILES_FOR_CLEANING)

if (CMGUI_USE_PERL_INTERPRETER)
    set(CMISS_PERL_INTERPRETER_FIND_FLAG REQUIRED)
    set(DEFINITIONS_FOR_PERL_INTERPRETER USE_PERL_INTERPRETER)
else ()
    set(CMISS_PERL_INTERPRETER_FIND_FLAG QUIET)
endif ()

# Find our friendly Zinc CMake config file.
# We can only use the static version of the library for the Cmgui application
set(ZINC_USE_STATIC TRUE)
#set(REQUIRE_INTERNAL_ZINC_INCLUDE_DIR TRUE)
find_package(Zinc REQUIRED)
find_package(cmiss_perl_interpreter ${CMISS_PERL_INTERPRETER_FIND_FLAG})
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL UiTools REQUIRED)
find_package(Git QUIET)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

set(QRC_RESOURCES src/command/qt/ui_files.qrc)

if (WIN32)
    set(WIN32_SYSTEM TRUE )
endif ()

if (MSVC)
    set(EXTRA_COMPILER_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
endif ()

include(cmgui_sources_list.cmake)
include(cmgui_headers_list.cmake)

if (APPLE)
    set(OSX_ICON src/icons/cmiss_torso.icns)
    set_source_files_properties(${OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif ()

set(CMGUI_REVISION "unknown-rev")
if (GIT_FOUND)
    git_get_revision(CMGUI_REVISION_LONG)
    string(SUBSTRING ${CMGUI_REVISION_LONG} 0 6 CMGUI_REVISION)
endif()

get_cmgui_system_name(CMGUI_SYSTEM_NAME)
set(CMGUI_BUILD_INFORMATION "\"${CMGUI_SYSTEM_NAME}\"")
set(CMGUI_VERSION_STAGING_HDR "${PROJECT_BINARY_DIR}/CMakeFiles/cmgui_version.staged.h")

# We want the version header to be configured at compile time and not just configure time.
set(CMGUI_VERSION_HDR ${CMAKE_CURRENT_BINARY_DIR}/src/configure/cmgui_version.h)

if (CMAKE_CONFIGURATION_TYPES)
    set(CONFIGURE_BUILD_TYPE $<CONFIG>)
else ()
    set(CONFIGURE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif ()

# Configure the CMake script that generates the version header file.
set(CONFIGURE_VERSION_SCRIPT ${PROJECT_BINARY_DIR}/CMakeFiles/ConfigureVersion.cmake)
configure_file(${PROJECT_SOURCE_DIR}/cmake/ConfigureVersion.in.cmake ${CONFIGURE_VERSION_SCRIPT} @ONLY)

add_custom_command(OUTPUT ${CMGUI_VERSION_STAGING_HDR}
    COMMAND "${CMAKE_COMMAND}" -DACTIVE_BUILD_TYPE=${CONFIGURE_BUILD_TYPE} -P ${CONFIGURE_VERSION_SCRIPT}
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMGUI_VERSION_STAGING_HDR} ${CMGUI_VERSION_HDR}
    COMMAND "${CMAKE_COMMAND}" -E remove ${CMGUI_VERSION_STAGING_HDR}
)
add_custom_target(generate_version_file
    DEPENDS ${CMGUI_VERSION_STAGING_HDR}
)
set_source_files_properties(${CMGUI_VERSION_HDR} PROPERTIES GENERATED TRUE SKIP_AUTOMOC ON)
list(APPEND FILES_FOR_CLEANING ${CMGUI_VERSION_HDR})

set(PROJECT_INCLUDE_DIR ${PROJECT_BINARY_DIR}/src)
set(CMGUI_CONFIGURE_HDR ${PROJECT_INCLUDE_DIR}/configure/cmgui_configure.h)
configure_file(${PROJECT_SOURCE_DIR}/src/configure/cmgui_configure.h.cmake ${CMGUI_CONFIGURE_HDR})

set(DEFINITIONS_FOR_CMGUI FE_value)

set(CMGUI_TARGET cmgui)
add_executable(${CMGUI_TARGET} WIN32 MACOSX_BUNDLE ${CMGUI_SRCS} ${OSX_ICON} ${QRC_RESOURCES})
add_dependencies(${CMGUI_TARGET} generate_version_file)

set_target_properties(${CMGUI_TARGET} PROPERTIES DEBUG_POSTFIX "d")
target_link_libraries(${CMGUI_TARGET} PRIVATE cmgui_model graphics_qt command_qt zinc-static ${CMISS_PERL_INTERPRETER_LIBRARIES} Qt5::Widgets Qt5::UiTools)
target_include_directories(${CMGUI_TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/src" ${PROJECT_INCLUDE_DIR} ${CMISS_PERL_INTERPRETER_INCLUDE_DIRS})
target_compile_definitions(${CMGUI_TARGET} PRIVATE ${DEFINITIONS_FOR_CMGUI} ${DEFINITIONS_FOR_PERL_INTERPRETER})

# I will want to remove these next three lines at some point.
include(source/cmake/app_srcs.cmake)
include(source/cmake/app_hdrs.cmake)
add_custom_target(cmgui_old SOURCES ${APP_SRCS} ${APP_HDRS})

add_custom_target(cmgui_headers SOURCES ${CMGUI_HDRS} ${CMGUI_CONFIGURE_HDR} ${CMGUI_VERSION_HDR})

# Additional clean files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${FILES_FOR_CLEANING}")

# On Apple platforms we need to do two extra tasks 1. Create a symbolic link for the
# application bundle to cmgui for buildbot testing and 2. Remove old Cmgui application
# bundles
if (APPLE)
    set(CMGUI_BUNDLE_NAME Cmgui)
    file(GLOB OLD_CMGUIS "${CMAKE_CURRENT_BINARY_DIR}/Cmgui*.app")
    foreach(OLD_CMGUI ${OLD_CMGUIS})
        MESSAGE(STATUS "Removing old Cmgui: ${OLD_CMGUI}")
        EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E remove_directory ${OLD_CMGUI})
    endforeach()
    add_custom_command(TARGET ${CMGUI_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove cmgui
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMGUI_BUNDLE_NAME}.app/Contents/MacOS/${CMGUI_BUNDLE_NAME} cmgui
        COMMENT "Creating symlink to cmgui for Buildbot testing."
    )
endif ()

set(CPACK_PACKAGE_NAME "Cmgui")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION "${CMGUI_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${CMGUI_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${CMGUI_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${CMGUI_PATCH_VERSION}")
if(WIN32)
    MESSAGE(STATUS "Please implement NSIS installer for me.")
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_SET_DESTDIR ON)
    list(APPEND CPACK_GENERATOR "PackageMaker")
    set(CPACK_SYSTEM_NAME "OSX")
    set(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacInstallerWelcome.txt)
    set_target_properties(${CMGUI_TARGET} PROPERTIES
        OUTPUT_NAME ${CMGUI_BUNDLE_NAME}
        MACOSX_BUNDLE_INFO_STRING "Cmgui 3D finite element visualisation application."
        MACOSX_BUNDLE_ICON_FILE "cmiss_torso.icns"
        MACOSX_BUNDLE_GUI_IDENTIFIER "Cmgui"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CMGUI_VERSION}${CMGUI_DEVELOPER_VERSION}"
        MACOSX_BUNDLE_BUNDLE_NAME "${CMGUI_BUNDLE_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMGUI_VERSION}${CMGUI_DEVELOPER_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_COPYRIGHT "The University of Auckland, 2018"
    )
    set(CMGUI_BUNDLE_LOCATION "/Applications")
    set(CMAKE_INSTALL_PREFIX "/Applications/${CMGUI_BUNDLE_NAME}.app/Contents")
    set(CMGUI_BUNDLE_DESTINATION BUNDLE DESTINATION ${CMGUI_BUNDLE_LOCATION})
elseif(UNIX)
    set(CPACK_SET_DESTDIR ON)
    string(FIND "${CMGUI_SYSTEM_NAME}" "Ubuntu" INDEX)
    if (INDEX EQUAL 0)
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Hugh Sorby") #required
    list(APPEND CPACK_GENERATOR "DEB")
    endif ()
endif()

install(TARGETS ${CMGUI_TARGET}
    RUNTIME DESTINATION bin
    ${CMGUI_BUNDLE_DESTINATION}
)

set(CMGUI_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
if(WIN32)
	string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CMGUI_ARCHITECTURE)
endif()

set(CPACK_PACKAGE_VENDOR "opencmiss")
set(CPACK_PACKAGE_RELOCATABLE FALSE)

set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.rst)

set(CPACK_PACKAGE_VERSION_MAJOR ${CMGUI_MAJOR_VERSION})
set(CPACK_PACKAGE_FILE_NAME "cmgui-${CMGUI_VERSION}${CMGUI_DEVELOPER_VERSION}-${CMGUI_ARCHITECTURE}-${CMGUI_SYSTEM_NAME}")
set(CPACK_OUTPUT_FILE_PREFIX "package")

include(CPack)

