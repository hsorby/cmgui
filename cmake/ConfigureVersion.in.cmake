# OpenCMISS-Cmgui Application
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if (CMGUI_OFFICIAL_RELEASE)
    string(TIMESTAMP CMGUI_DATETIME_STRING "+%d/%m/%Y %H:%M:%S" UTC)
else ()
    set(CMGUI_DATETIME_STRING "00/00/00 00:00:00")
endif ()

set(CMGUI_VERSION @PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@)
string(TOLOWER ${ACTIVE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)

set(CMAKE_MODULE_PATH @CMAKE_MODULE_PATH@)
find_package(Git QUIET)

set(CMGUI_REVISION_LONG "unknown-revision")
if (GIT_FOUND)
    git_get_revision(CMGUI_REVISION_LONG WORKING_DIRECTORY "@PROJECT_SOURCE_DIR@")
    string(SUBSTRING ${CMGUI_REVISION_LONG} 0 6 CMGUI_REVISION)
endif()

set(CMGUI_BUILD_INFORMATION @CMGUI_BUILD_INFORMATION@)

configure_file("@PROJECT_SOURCE_DIR@/src/configure/cmgui_version.h.cmake"
    "@CMGUI_VERSION_STAGING_HDR@" @ONLY)
