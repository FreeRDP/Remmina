# See LICENSE and COPYING files for copyright and license details.

include_directories(${CMAKE_SOURCE_DIR}/remmina-plugins)

set(APPICON16_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/emblems")
set(APPICON22_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/emblems")

set(REMMINA_COMMON_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
set(REMMINA_COMMON_LIBRARIES ${GTK_LIBRARIES})

find_suggested_package(LIBSSH)
if(LIBSSH_FOUND)
	add_definitions(-DHAVE_LIBSSH)
	include_directories(${SSH_INCLUDE_DIRS})
	target_link_libraries(remmina ${SSH_LIBRARIES})
endif()

find_required_package(LIBVNCSERVER)

find_required_package(XKBFILE)

if(LIBSSH_FOUND AND XKBFILE_FOUND)
	add_subdirectory(nx)
endif()

add_subdirectory(xdmcp)

find_suggested_package(FREERDP)
if(FREERDP_FOUND)
	add_subdirectory(rdp)
endif()

find_suggested_package(TELEPATHY)
if(TELEPATHY_FOUND)
	add_subdirectory(telepathy)
endif()

find_suggested_package(ZLIB)

if(ZLIB_FOUND)
	add_subdirectory(vnc)
endif()

if(WITH_EXAMPLES)
	add_subdirectory(tool_hello_world)
endif()
