# remmina-plugin-rdp - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL. If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so. If you
# do not wish to do so, delete this exception statement from your
# version. If you delete this exception statement from all source
# files in the program, then also delete it here.


include(ExternalProject)

set(CMAKE_THREAD_PREFER_PTHREAD)
find_package(Threads)


set(REMMINA_PLUGIN_RDP_SRCS
	rdp_plugin.c
	rdp_plugin.h
	rdp_event.c
	rdp_event.h
	rdp_file.c
	rdp_file.h
	rdp_settings.c
	rdp_settings.h
	rdp_gdi.c
	rdp_gdi.h
	rdp_graphics.c
	rdp_graphics.h
	rdp_cliprdr.c
	rdp_cliprdr.h
	rdp_channels.c
	rdp_channels.h
	)

if (WITH_EMBEDDED_FREERDP)

	find_package(OpenSSL REQUIRED)
	find_package(Pulse REQUIRED)
	find_package(Cups REQUIRED)

	set(FREERDP_TOPDIR ${CMAKE_CURRENT_SOURCE_DIR}/external/FreeRDP )
	set(FREERDP_INCLUDE_DIRS ${FREERDP_TOPDIR}/install/include )
	set(FREERDP_LIBDIR ${FREERDP_TOPDIR}/install/${CMAKE_INSTALL_LIBDIR} )
	set(FREERDP_LIBRARIES
		${FREERDP_LIBDIR}/librdtk.a
		${FREERDP_LIBDIR}/libfreerdp.a
		${FREERDP_LIBDIR}/libwinpr.a
		${FREERDP_LIBDIR}/libwinpr-makecert-tool.a
		${FREERDP_LIBDIR}/libxfreerdp-client.a
		${FREERDP_LIBDIR}/libfreerdp-client.a
		${FREERDP_LIBDIR}/freerdp/libaudin-client.a
	#	${FREERDP_LIBDIR}/freerdp/libaudin-client-alsa.a
		${FREERDP_LIBDIR}/freerdp/libaudin-client-pulse.a
		${FREERDP_LIBDIR}/freerdp/libcliprdr-client.a
		${FREERDP_LIBDIR}/freerdp/libdisp-client.a
		${FREERDP_LIBDIR}/freerdp/libdrdynvc-client.a
		${FREERDP_LIBDIR}/freerdp/libdrive-client.a
		${FREERDP_LIBDIR}/freerdp/libecho-client.a
		${FREERDP_LIBDIR}/freerdp/libencomsp-client.a
		${FREERDP_LIBDIR}/freerdp/libparallel-client.a
		${FREERDP_LIBDIR}/freerdp/libprinter-client.a
		${FREERDP_LIBDIR}/freerdp/librail-client.a
		${FREERDP_LIBDIR}/freerdp/librdpdr-client.a
		${FREERDP_LIBDIR}/freerdp/librdpei-client.a
		${FREERDP_LIBDIR}/freerdp/librdpgfx-client.a
		${FREERDP_LIBDIR}/freerdp/librdpsnd-client.a
	#	${FREERDP_LIBDIR}/freerdp/librdpsnd-client-alsa.a
		${FREERDP_LIBDIR}/freerdp/librdpsnd-client-pulse.a
		${FREERDP_LIBDIR}/freerdp/libremdesk-client.a
		${FREERDP_LIBDIR}/freerdp/libserial-client.a
		${FREERDP_LIBDIR}/freerdp/libsmartcard-client.a
		${FREERDP_LIBDIR}/freerdp/libtsmf-client.a
	#	${FREERDP_LIBDIR}/freerdp/libtsmf-client-alsa-audio.a
	#	${FREERDP_LIBDIR}/freerdp/libtsmf-client-ffmpeg-decoder.a
		${FREERDP_LIBDIR}/freerdp/libtsmf-client-pulse-audio.a
	)

	ExternalProject_Add(
		FreeRDP
		PREFIX ${FREERDP_TOPDIR}/src
		SOURCE_DIR ${FREERDP_TOPDIR}/src
		BINARY_DIR ${FREERDP_TOPDIR}/build
		INSTALL_DIR ${FREERDP_TOPDIR}/install
		STAMP_DIR ${FREERDP_TOPDIR}/stamps
		TMP_DIR ${FREERDP_TOPDIR}/tmp
		CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=${FREERDP_TOPDIR}/install"
		UPDATE_COMMAND ""
		CMAKE_ARGS -DWITH_CUPS=on -DWITH_PULSE=on -DWITH_ALSA=off -DWITH_GSTREAMER_1_0=off -DWITH_MANPAGES=off -DWITH_WAYLAND=off -DBUILD_SHARED_LIBS=off -DWITH_FFMPEG=off
	)

	# Put a dependency on FreeRDP project via libfreerdp.a, so remmina
	# will stop building until FreeRDP is ready in "make -jN" multithread build
	add_library(libfreerdp STATIC IMPORTED)
	set_property(TARGET libfreerdp PROPERTY IMPORTED_LOCATION ${FREERDP_LIBDIR}/libfreerdp.a)
	add_dependencies(libfreerdp FreeRDP)


	# Add dynamic libraries needed for FreeRDP
	set(FREERDP_LIBRARIES
		-Wl,--start-group ${FREERDP_LIBRARIES} -Wl,--end-group
		${XKBFILE_LIBRARIES} ${OPENSSL_LIBRARIES} ${PULSE_LIBRARIES} ${CUPS_LIBRARIES})

endif()

add_definitions(-DFREERDP_REQUIRED_MAJOR=${FREERDP_REQUIRED_MAJOR})
add_definitions(-DFREERDP_REQUIRED_MINOR=${FREERDP_REQUIRED_MINOR})
add_definitions(-DFREERDP_REQUIRED_REVISION=${FREERDP_REQUIRED_REVISION})


add_library(remmina-plugin-rdp ${REMMINA_PLUGIN_RDP_SRCS})
set_target_properties(remmina-plugin-rdp PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-rdp PROPERTIES NO_SONAME 1)

if (WITH_EMBEDDED_FREERDP)
	add_dependencies(remmina-plugin-rdp libfreerdp)
endif()

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${FREERDP_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-rdp ${REMMINA_COMMON_LIBRARIES} ${FREERDP_LIBRARIES})

install(TARGETS remmina-plugin-rdp DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-rdp-ssh.png 16x16/emblems/remmina-rdp.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-rdp-ssh.png 22x22/emblems/remmina-rdp.png DESTINATION ${APPICON22_EMBLEMS_DIR})
