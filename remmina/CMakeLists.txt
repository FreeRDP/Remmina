# src/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
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
# Foundation, Inc., 59 Temple Place, Suite 330, 
# Boston, MA 02111-1307, USA.
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


cmake_minimum_required(VERSION 2.8)

set(REMMINA_SRCS
	src/remmina_about.c
	src/remmina_about.h
	src/remmina_applet_menu.c
	src/remmina_applet_menu.h
	src/remmina_applet_menu_item.c
	src/remmina_applet_menu_item.h
	src/remmina_avahi.c
	src/remmina_avahi.h
	src/remmina.c
	src/remmina_chain_button.c
	src/remmina_chain_button.h
	src/remmina_chat_window.c
	src/remmina_chat_window.h
	src/remmina_connection_window.c
	src/remmina_connection_window.h
	src/remmina_crypt.c
	src/remmina_crypt.h
	src/remmina_exec.c
	src/remmina_exec.h
	src/remmina_file.c
	src/remmina_file_editor.c
	src/remmina_file_editor.h
	src/remmina_file.h
	src/remmina_file_manager.c
	src/remmina_file_manager.h
	src/remmina_ftp_client.c
	src/remmina_ftp_client.h
	src/remmina_icon.c
	src/remmina_icon.h
	src/remmina_init_dialog.c
	src/remmina_init_dialog.h
	src/remmina_key_chooser.c
	src/remmina_key_chooser.h
	src/remmina_log.c
	src/remmina_log.h
	src/remmina_main.c
	src/remmina_main.h
	src/remmina_marshals.c
	src/remmina_marshals.h
	src/remmina_marshals.list
	src/remmina_plugin_manager.c
	src/remmina_plugin_manager.h
	src/remmina_pref.c
	src/remmina_pref_dialog.c
	src/remmina_pref_dialog.h
	src/remmina_pref.h
	src/remmina_protocol_widget.c
	src/remmina_protocol_widget.h
	src/remmina_public.c
	src/remmina_public.h
	src/remmina_scaler.c
	src/remmina_scaler.h
	src/remmina_scrolled_viewport.c
	src/remmina_scrolled_viewport.h
	src/remmina_sftp_client.c
	src/remmina_sftp_client.h
	src/remmina_sftp_plugin.c
	src/remmina_sftp_plugin.h
	src/remmina_ssh.c
	src/remmina_ssh.h
	src/remmina_ssh_plugin.c
	src/remmina_ssh_plugin.h
	src/remmina_string_array.c
	src/remmina_string_array.h
	src/remmina_string_list.c
	src/remmina_string_list.h
	src/remmina_widget_pool.c
	src/remmina_widget_pool.h
	src/remmina_external_tools.c
	src/remmina_external_tools.h
	)

add_executable(remmina ${REMMINA_SRCS})

include_directories(${GTK_INCLUDE_DIRS})
target_link_libraries(remmina ${GTK_LIBRARIES})

find_package(X11)
include_directories(${X11_INCLUDE_DIR})
target_link_libraries(remmina ${X11_LIBRARIES})

find_suggested_package(LIBSSH)
if(LIBSSH_FOUND)
	add_definitions(-DHAVE_LIBSSH)
	include_directories(${LIBSSH_INCLUDE_DIRS})
	target_link_libraries(remmina ${LIBSSH_LIBRARIES})
endif()

if(PTHREAD_FOUND)
	include_directories(${PTHREAD_INCLUDE_DIRS})
	target_link_libraries(remmina ${PTHREAD_LIBRARIES})
endif()

if(GCRYPT_FOUND)
	include_directories(${GCRYPT_INCLUDE_DIRS})
	target_link_libraries(remmina ${GCRYPT_LIBRARIES})
endif()

if(AVAHI_FOUND)
	include_directories(${AVAHI_INCLUDE_DIRS})
	target_link_libraries(remmina ${AVAHI_LIBRARIES})
endif()

if(GTK3_FOUND)
	set(_VTE_VERSION_NUM 2.90)
else()
	set(_VTE_VERSION_NUM)
endif()

find_suggested_package(VTE _VTE_VERSION_NUM)
if(VTE_FOUND)
	add_definitions(-DHAVE_LIBVTE)
	include_directories(${VTE_INCLUDE_DIRS})
	target_link_libraries(remmina ${VTE_LIBRARIES})
endif()

if(GTK3_FOUND)
	find_suggested_package(APPINDICATOR)
	if(APPINDICATOR_FOUND)
		add_definitions(-DHAVE_LIBAPPINDICATOR)
		include_directories(${APPINDICATOR_INCLUDE_DIRS})
		target_link_libraries(remmina ${APPINDICATOR_LIBRARIES})
	endif()
endif()

if(WITH_TRANSLATIONS)
	add_subdirectory(po)
endif()
add_subdirectory(icons)
add_subdirectory(desktop)
add_subdirectory(external_tools)

install(TARGETS remmina DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina.pc.in ${CMAKE_CURRENT_BINARY_DIR}/remmina.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

