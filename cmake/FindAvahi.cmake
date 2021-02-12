# Try to find the Avahi libraries and headers
# Once done this will define:
#
#  AVAHI_FOUND          - system has the avahi libraries
#  AVAHI_INCLUDE_DIRS   - the avahi include directories
#  AVAHI_LIBRARIES      - The libraries needed to use avahi

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(_AVAHI avahi-client)
endif(PKG_CONFIG_FOUND)

find_library(AVAHI_COMMON_LIB NAMES avahi-common
    PATHS
    ${_AVAHI_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

find_library(AVAHI_CLIENT_LIB NAMES avahi-client
    PATHS
    ${_AVAHI_LIBRARY_DIRS}
    ${CLIENT_LIB_DIR}
)

find_library(AVAHI_UI_LIB NAMES avahi-ui-gtk3
    PATHS
    ${_AVAHI_LIBRARY_DIRS}
    ${UI_LIB_DIR}
)


if(AVAHI_COMMON_LIB AND AVAHI_CLIENT_LIB AND AVAHI_UI_LIB)
    set(AVAHI_LIBRARIES ${AVAHI_COMMON_LIB} ${AVAHI_CLIENT_LIB} ${AVAHI_UI_LIB})
    message(STATUS "Avahi-Libs found: ${AVAHI_LIBRARIES}")
endif()

find_path(COMMON_INCLUDE_DIR watch.h
    PATH_SUFFIXES avahi-common
    PATHS
    ${_AVAHI_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

find_path(CLIENT_INCLUDE_DIR client.h
    PATH_SUFFIXES avahi-client
    PATHS
    ${_AVAHI_INCLUDE_DIRS}
    ${CLIENT_INCLUDE_DIR}
)

find_path(UI_INCLUDE_DIR avahi-ui.h
    PATH_SUFFIXES avahi-ui
    PATHS
    ${_AVAHI_INCLUDE_DIRS}
    ${UI_INCLUDE_DIR}
)

if(COMMON_INCLUDE_DIR AND CLIENT_INCLUDE_DIR AND UI_INCLUDE_DIR)
    set(AVAHI_INCLUDE_DIRS ${COMMON_INCLUDE_DIR} ${CLIENT_INCLUDE_DIR} ${UI_INCLUDE_DIR})
    message(STATUS "Avahi-Include-Dirs found: ${AVAHI_INCLUDE_DIRS}")
endif()

if(AVAHI_LIBRARIES AND AVAHI_INCLUDE_DIRS)
    set(AVAHI_FOUND TRUE)
endif()
