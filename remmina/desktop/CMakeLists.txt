# desktop/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2018 Antenore Gatta, Giovanni Panozzo
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

set(ICON16_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/apps")
set(ICON22_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/apps")
set(ICON24_DIR "${REMMINA_DATADIR}/icons/hicolor/24x24/apps")
set(ICON32_DIR "${REMMINA_DATADIR}/icons/hicolor/32x32/apps")
set(ICON48_DIR "${REMMINA_DATADIR}/icons/hicolor/48x48/apps")
set(ICON64_DIR "${REMMINA_DATADIR}/icons/hicolor/64x64/apps")
set(ICON72_DIR "${REMMINA_DATADIR}/icons/hicolor/72x72/apps")
set(ICON96_DIR "${REMMINA_DATADIR}/icons/hicolor/96x96/apps")
set(ICON128_DIR "${REMMINA_DATADIR}/icons/hicolor/128x128/apps")
set(ICON256_DIR "${REMMINA_DATADIR}/icons/hicolor/256x256/apps")
set(ICONSVG_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/apps")

set(ICON16_DATA 16x16/apps/remmina.png)
set(ICON22_DATA 22x22/apps/remmina.png)
set(ICON24_DATA 24x24/apps/remmina.png)
set(ICON16_DATA_PANEL 16x16/apps/remmina-panel.png)
set(ICON22_DATA_PANEL 22x22/apps/remmina-panel.png)
set(ICON24_DATA_PANEL 24x24/apps/remmina-panel.png)
set(ICON16_DATA_PANEL_LIGHT 16x16/apps/remmina-panel-inverted.png)
set(ICON22_DATA_PANEL_LIGHT 22x22/apps/remmina-panel-inverted.png)
set(ICON24_DATA_PANEL_LIGHT 24x24/apps/remmina-panel-inverted.png)
set(ICON32_DATA 32x32/apps/remmina.png)
set(ICON48_DATA 48x48/apps/remmina.png)
set(ICON64_DATA 64x64/apps/remmina.png)
set(ICON72_DATA 72x72/apps/remmina.png)
set(ICON96_DATA 96x96/apps/remmina.png)
set(ICON128_DATA 128x128/apps/remmina.png)
set(ICON256_DATA 256x256/apps/remmina.png)
set(ICONSVG_DATA scalable/apps/remmina.svg)

install(FILES ${ICON16_DATA} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA} DESTINATION ${ICON24_DIR})
install(FILES ${ICON16_DATA_PANEL} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA_PANEL} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA_PANEL} DESTINATION ${ICON24_DIR})
install(FILES ${ICON16_DATA_PANEL_LIGHT} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA_PANEL_LIGHT} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA_PANEL_LIGHT} DESTINATION ${ICON24_DIR})
install(FILES ${ICON32_DATA} DESTINATION ${ICON32_DIR})
install(FILES ${ICON48_DATA} DESTINATION ${ICON48_DIR})
install(FILES ${ICON64_DATA} DESTINATION ${ICON64_DIR})
install(FILES ${ICON72_DATA} DESTINATION ${ICON72_DIR})
install(FILES ${ICON96_DATA} DESTINATION ${ICON96_DIR})
install(FILES ${ICON128_DATA} DESTINATION ${ICON128_DIR})
install(FILES ${ICON256_DATA} DESTINATION ${ICON256_DIR})
install(FILES ${ICONSVG_DATA} DESTINATION ${ICONSVG_DIR})

set(REMMINA_BINARY_PATH ${CMAKE_INSTALL_FULL_BINDIR}/remmina)
set(REMMINA_ICON "remmina")

find_program(XDGICON xdg-icon-resource)
find_program(GTKICON gtk-update-icon-cache)
find_program(XDGMENU xdg-desktop-menu)

if(XDGICON)
    message(STATUS "${XDGICON} found")
    install(CODE "
    MESSAGE(\"Updating icon cache.\")
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON16_DIR}/remmina.png --size 16 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON22_DIR}/remmina.png --size 22 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON24_DIR}/remmina.png --size 24 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON32_DIR}/remmina.png --size 32 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON48_DIR}/remmina.png --size 48 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON64_DIR}/remmina.png --size 64 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON72_DIR}/remmina.png --size 72 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON96_DIR}/remmina.png --size 96 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON128_DIR}/remmina.png --size 128 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --novendor ${ICON256_DIR}/remmina.png --size 256 remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON16_DIR}/remmina.png --size 16 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON22_DIR}/remmina.png --size 22 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON24_DIR}/remmina.png --size 24 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON32_DIR}/remmina.png --size 32 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON48_DIR}/remmina.png --size 48 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON64_DIR}/remmina.png --size 64 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON72_DIR}/remmina.png --size 72 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON96_DIR}/remmina.png --size 96 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON128_DIR}/remmina.png --size 128 application-x-remmina)
    execute_process(COMMAND ${XDGICON} install --mode system --context mimetypes ${ICON256_DIR}/remmina.png --size 256 application-x-remmina)
    ")
else()
    message(WARNING "Cannot update icon cache: xdg-icon-resource not found")
endif()

if(GTKICON)
    message(STATUS "${GTKICON} found")
    install(CODE "
    MESSAGE(\"Updating theme icon cache.\")
    execute_process(COMMAND ${GTKICON} -f ${REMMINA_DATADIR}/icons/hicolor)
    ")
else()
    message(WARNING "Cannot update icon cache: gtk-update-icon-cache not found")
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/remmina.desktop @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina-file.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/remmina-file.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina.desktop
    RENAME ${UNIQUE_APPNAME}.desktop
    DESTINATION "${REMMINA_DATADIR}/applications")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina-file.desktop
    DESTINATION "${REMMINA_DATADIR}/applications")
install(FILES appdata.xml
    RENAME ${UNIQUE_APPNAME}.appdata.xml
    DESTINATION "${REMMINA_DATADIR}/metainfo")
# XML mime types
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
set( XDG_MIME_INSTALL_DIR "${REMMINA_DATADIR}/mime/packages" )
find_package( SharedMimeInfo )
if( SHARED_MIME_INFO_FOUND )
    install( FILES remmina-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif()

