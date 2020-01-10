# src/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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


cmake_minimum_required(VERSION 3.0.0)

list(APPEND REMMINA_SRCS
	"remmina_about.c"
	"remmina_about.h"
	"remmina_applet_menu.c"
	"remmina_applet_menu.h"
	"remmina_applet_menu_item.c"
	"remmina_applet_menu_item.h"
	"remmina_avahi.c"
	"remmina_avahi.h"
	"remmina.c"
	"remmina.h"
	"remmina_chat_window.c"
	"remmina_chat_window.h"
	"remmina_crypt.c"
	"remmina_crypt.h"
	"remmina_exec.c"
	"remmina_exec.h"
	"remmina_file.c"
	"remmina_file_editor.c"
	"remmina_file_editor.h"
	"remmina_file.h"
	"remmina_file_manager.c"
	"remmina_file_manager.h"
	"remmina_ftp_client.c"
	"remmina_ftp_client.h"
	"remmina_icon.c"
	"remmina_icon.h"
	"remmina_key_chooser.c"
	"remmina_key_chooser.h"
	"remmina_log.c"
	"remmina_log.h"
	"remmina_main.c"
	"remmina_main.h"
	"remmina_marshals.c"
	"remmina_marshals.h"
	"remmina_marshals.list"
	"remmina_masterthread_exec.c"
	"remmina_masterthread_exec.h"
	"remmina_message_panel.c"
	"remmina_message_panel.h"
	"remmina_plugin_manager.c"
	"remmina_plugin_manager.h"
	"remmina_ext_exec.c"
	"remmina_ext_exec.h"
	"remmina_pref.c"
	"remmina_pref_dialog.c"
	"remmina_pref_dialog.h"
	"remmina_pref.h"
	"remmina_protocol_widget.c"
	"remmina_protocol_widget.h"
	"remmina_public.c"
	"remmina_public.h"
	"remmina_scrolled_viewport.c"
	"remmina_scrolled_viewport.h"
	"remmina_sftp_client.c"
	"remmina_sftp_client.h"
	"remmina_sftp_plugin.c"
	"remmina_sftp_plugin.h"
	"remmina_sodium.c"
	"remmina_sodium.h"
	"remmina_ssh.c"
	"remmina_ssh.h"
	"remmina_ssh_plugin.c"
	"remmina_ssh_plugin.h"
	"remmina_string_array.c"
	"remmina_string_array.h"
	"remmina_string_list.c"
	"remmina_string_list.h"
	"remmina_unlock.c"
	"remmina_unlock.h"
	"remmina_utils.c"
	"remmina_utils.h"
	"remmina_widget_pool.c"
	"remmina_widget_pool.h"
	"remmina_external_tools.c"
	"remmina_external_tools.h"
	"remmina_sysinfo.h"
	"remmina_sysinfo.c"
	"rcw.c"
	"rcw.h"
	"rmnews.c"
	"rmnews.h"
	"remmina_mpchange.c"
	"remmina_mpchange.h"
	"remmina_scheduler.c"
	"remmina_scheduler.h"
	"remmina_stats.c"
	"remmina_stats.h"
	"remmina_stats_sender.c"
	"remmina_stats_sender.h"
	)

add_executable(remmina ${REMMINA_SRCS})
include_directories(${GTK_INCLUDE_DIRS} ${gio_INCLUDE_DIRS} ${gio-unix_INCLUDE_DIRS})
target_link_libraries(remmina ${GTK_LIBRARIES})

if(WITH_MANPAGES)
	install(FILES remmina.1 DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1)
endif()

find_package(X11)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(remmina ${X11_LIBRARIES})

target_link_libraries(remmina ${CMAKE_THREAD_LIBS_INIT})

find_package(Intl)
if(INTL_FOUND AND "${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD|OpenBSD|NetBSD")
	message(STATUS "${CMAKE_SYSTEM_NAME} detected, building with Intl")
	include_directories(${Intl_INCLUDE_DIRS})
	target_link_libraries(remmina ${Intl_LIBRARIES})
endif()

find_suggested_package(LIBSSH)
if(LIBSSH_FOUND)
	add_definitions(-DHAVE_LIBSSH)
	include_directories(${LIBSSH_INCLUDE_DIRS})
	target_link_libraries(remmina ${LIBSSH_LIBRARIES})
endif()

if(GCRYPT_FOUND)
	include_directories(${GCRYPT_INCLUDE_DIRS})
	target_link_libraries(remmina ${GCRYPT_LIBRARIES})
endif()

if(AVAHI_FOUND)
	include_directories(${AVAHI_INCLUDE_DIRS})
	target_link_libraries(remmina ${AVAHI_LIBRARIES})
endif()

if(OPENSSL_FOUND)
	include_directories(${OPENSSL_INCLUDE_DIRS})
	target_link_libraries(remmina ${OPENSSL_LIBRARIES})
endif()

option(WITH_VTE "Build with support for VTE" ON)
if(GTK3_FOUND AND WITH_VTE)
	set(_VTE_VERSION_NUMS 2.91 2.90)
	foreach(__VTE_VERSION ${_VTE_VERSION_NUMS})
		set(_VTE_VERSION_NUM ${__VTE_VERSION})
		find_suggested_package(VTE)
		if(VTE_FOUND)
			break()
		endif()
		message(STATUS "VTE ${__VTE_VERSION} not found")
	endforeach()
elseif(WITH_VTE)
	set(_VTE_VERSION_NUM)
	find_package(VTE)
endif()

if(VTE_FOUND)
	add_definitions(-DHAVE_LIBVTE)
	include_directories(${VTE_INCLUDE_DIRS})
	target_link_libraries(remmina ${VTE_LIBRARIES})
endif()

if(sodium_FOUND)
    message(STATUS "Sodium found")
    message(STATUS "Sodium include dirs ${sodium_INCLUDE_DIR}")
    include_directories(${sodium_INCLUDE_DIR})
    target_link_libraries(remmina sodium)
endif()

if(GTK3_FOUND)
	find_suggested_package(APPINDICATOR)
	if(APPINDICATOR_FOUND)
		add_definitions(-DHAVE_LIBAPPINDICATOR)
		include_directories(${APPINDICATOR_INCLUDE_DIRS})
		target_link_libraries(remmina ${APPINDICATOR_LIBRARIES})
	endif()
	find_required_package(JSONGLIB)
	if(JSONGLIB_FOUND)
		include_directories(${JSONGLIB_INCLUDE_DIRS})
		target_link_libraries(remmina ${JSONGLIB_LIBRARIES})
	else()
		message(FATAL_ERROR "json-glib library not found")
	endif()
	find_required_package(LIBSOUP24)
	if(LIBSOUP24_FOUND)
		include_directories(${LIBSOUP24_INCLUDE_DIRS})
		target_link_libraries(remmina ${LIBSOUP24_LIBRARIES})
	else()
		message(FATAL_ERROR "libsoup 2.4 library not found")
	endif()
endif()

add_subdirectory(external_tools)

install(TARGETS remmina DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/remmina/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/remmina
	FILES_MATCHING PATTERN "*.h")

