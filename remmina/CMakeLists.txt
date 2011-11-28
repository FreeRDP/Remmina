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

set(REMMINA_SRCS
	src/remminaabout.c
	src/remminaabout.h
	src/remminaappletmenu.c
	src/remminaappletmenu.h
	src/remminaappletmenuitem.c
	src/remminaappletmenuitem.h
	src/remminaavahi.c
	src/remminaavahi.h
	src/remmina.c
	src/remminachainbutton.c
	src/remminachainbutton.h
	src/remminachatwindow.c
	src/remminachatwindow.h
	src/remminaconnectionwindow.c
	src/remminaconnectionwindow.h
	src/remminacrypt.c
	src/remminacrypt.h
	src/remminaexec.c
	src/remminaexec.h
	src/remminafile.c
	src/remminafileeditor.c
	src/remminafileeditor.h
	src/remminafile.h
	src/remminafilemanager.c
	src/remminafilemanager.h
	src/remminaftpclient.c
	src/remminaftpclient.h
	src/remminaicon.c
	src/remminaicon.h
	src/remminainitdialog.c
	src/remminainitdialog.h
	src/remminakeychooser.c
	src/remminakeychooser.h
	src/remminalog.c
	src/remminalog.h
	src/remminamain.c
	src/remminamain.h
	src/remminamarshals.c
	src/remminamarshals.h
	src/remminamarshals.list
	src/remminapluginmanager.c
	src/remminapluginmanager.h
	src/remminapref.c
	src/remminaprefdialog.c
	src/remminaprefdialog.h
	src/remminapref.h
	src/remminaprotocolwidget.c
	src/remminaprotocolwidget.h
	src/remminapublic.c
	src/remminapublic.h
	src/remminascaler.c
	src/remminascaler.h
	src/remminascrolledviewport.c
	src/remminascrolledviewport.h
	src/remminasftpclient.c
	src/remminasftpclient.h
	src/remminasftpplugin.c
	src/remminasftpplugin.h
	src/remminassh.c
	src/remminassh.h
	src/remminasshplugin.c
	src/remminasshplugin.h
	src/remminastringarray.c
	src/remminastringarray.h
	src/remminastringlist.c
	src/remminastringlist.h
	src/remminawidgetpool.c
	src/remminawidgetpool.h
	)

add_executable(remmina ${REMMINA_SRCS})

include_directories(${GTK3_INCLUDE_DIRS})
target_link_libraries(remmina ${GTK3_LIBRARIES})

#find_suggested_package(LIBSSH)
#if(LIBSSH_FOUND)
#	add_definitions(-DHAVE_LIBSSH)
#	include_directories(${LIBSSH_INCLUDE_DIRS})
#	target_link_libraries(remmina ${LIBSSH_LIBRARIES})
#endif()

if(GCRYPT_FOUND)
	include_directories(${GCRYPT_INCLUDE_DIRS})
	target_link_libraries(remmina ${GCRYPT_LIBRARIES})
endif()

if(AVAHI_FOUND)
	include_directories(${AVAHI_INCLUDE_DIRS})
	target_link_libraries(remmina ${AVAHI_LIBRARIES})
endif()

find_suggested_package(VTE)
if(VTE_FOUND)
	add_definitions(-DHAVE_LIBVTE)
	include_directories(${VTE_INCLUDE_DIRS})
	target_link_libraries(remmina ${VTE_LIBRARIES})
endif()

find_suggested_package(APPINDICATOR)
if(APPINDICATOR_FOUND)
	add_definitions(-DHAVE_LIBAPPINDICATOR)
	include_directories(${APPINDICATOR_INCLUDE_DIRS})
	target_link_libraries(remmina ${APPINDICATOR_LIBRARIES})
endif()

add_subdirectory(po)
add_subdirectory(icons)
add_subdirectory(desktop)

install(TARGETS remmina DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina.pc.in ${CMAKE_CURRENT_BINARY_DIR}/remmina.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

