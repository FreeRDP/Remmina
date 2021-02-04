# desktop/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

set(REMMINA_ICON ${REMMINA_APP_ID})

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
set(ICON512_DIR "${REMMINA_DATADIR}/icons/hicolor/512x512/apps")
set(ICONSVG_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/apps")
set(ICONSYM_DIR "${REMMINA_DATADIR}/icons/hicolor/symbolic/apps")
set(ICONAPPS_DIR "${REMMINA_DATADIR}/icons/hicolor/apps")
set(ICONPANSVG_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/panel")

set(ICON16_DATA 16x16/apps/${REMMINA_ICON}.png)
set(ICON22_DATA 22x22/apps/${REMMINA_ICON}.png)
set(ICON24_DATA 24x24/apps/${REMMINA_ICON}.png)
set(ICON32_DATA 32x32/apps/${REMMINA_ICON}.png)
set(ICON48_DATA 48x48/apps/${REMMINA_ICON}.png)
set(ICON64_DATA 64x64/apps/${REMMINA_ICON}.png)
set(ICON72_DATA 72x72/apps/${REMMINA_ICON}.png)
set(ICON96_DATA 96x96/apps/${REMMINA_ICON}.png)
set(ICON128_DATA 128x128/apps/${REMMINA_ICON}.png)
set(ICON256_DATA 256x256/apps/${REMMINA_ICON}.png)
set(ICON512_DATA 512x512/apps/${REMMINA_ICON}.png)
set(ICON16_DATA_PANEL 16x16/apps/remmina-panel.png)
set(ICON22_DATA_PANEL 22x22/apps/remmina-panel.png)
set(ICON24_DATA_PANEL 24x24/apps/remmina-panel.png)
set(ICON32_DATA_PANEL 32x32/apps/remmina-panel.png)
set(ICON48_DATA_PANEL 48x48/apps/remmina-panel.png)
set(ICON64_DATA_PANEL 64x64/apps/remmina-panel.png)
set(ICON72_DATA_PANEL 72x72/apps/remmina-panel.png)
set(ICON96_DATA_PANEL 96x96/apps/remmina-panel.png)
set(ICON128_DATA_PANEL 128x128/apps/remmina-panel.png)
set(ICON256_DATA_PANEL 256x256/apps/remmina-panel.png)
set(ICON16_DATA_PANEL_LIGHT 16x16/apps/remmina-panel-inverted.png)
set(ICON22_DATA_PANEL_LIGHT 22x22/apps/remmina-panel-inverted.png)
set(ICON24_DATA_PANEL_LIGHT 24x24/apps/remmina-panel-inverted.png)
set(ICON32_DATA_PANEL_LIGHT 32x32/apps/remmina-panel-inverted.png)
set(ICON48_DATA_PANEL_LIGHT 48x48/apps/remmina-panel-inverted.png)
set(ICON64_DATA_PANEL_LIGHT 64x64/apps/remmina-panel-inverted.png)
set(ICON72_DATA_PANEL_LIGHT 72x72/apps/remmina-panel-inverted.png)
set(ICON96_DATA_PANEL_LIGHT 96x96/apps/remmina-panel-inverted.png)
set(ICON128_DATA_PANEL_LIGHT 128x128/apps/remmina-panel-inverted.png)
set(ICON256_DATA_PANEL_LIGHT 256x256/apps/remmina-panel-inverted.png)
set(ICONSVG_PAN_DATA scalable/apps/remmina-panel.svg)
set(ICONSVG_PANINV_DATA scalable/apps/remmina-panel-inverted.svg)
set(ICONSVG_DATA scalable/apps/${REMMINA_ICON}.svg)
set(ICONSVG_RM_DATA scalable/apps/remmina-symbolic.svg)
set(ICONSVG_ORG_DATA scalable/apps/${REMMINA_ICON}-symbolic.svg)
set(ICONSYM_ORG_DATA scalable/apps/${REMMINA_ICON}-symbolic.svg)

install(FILES ${ICON16_DATA} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA} DESTINATION ${ICON24_DIR})
install(FILES ${ICON32_DATA} DESTINATION ${ICON32_DIR})
install(FILES ${ICON48_DATA} DESTINATION ${ICON48_DIR})
install(FILES ${ICON64_DATA} DESTINATION ${ICON64_DIR})
install(FILES ${ICON72_DATA} DESTINATION ${ICON72_DIR})
install(FILES ${ICON96_DATA} DESTINATION ${ICON96_DIR})
install(FILES ${ICON128_DATA} DESTINATION ${ICON128_DIR})
install(FILES ${ICON256_DATA} DESTINATION ${ICON256_DIR})
install(FILES ${ICON512_DATA} DESTINATION ${ICON512_DIR})
install(FILES ${ICONSVG_DATA} DESTINATION ${ICONSVG_DIR})
install(FILES ${ICONSYM_DATA} DESTINATION ${ICONSYM_DIR})
install(FILES ${ICONSVG_PAN_DATA} DESTINATION ${ICONSVG_DIR})
install(FILES ${ICONSVG_PAN_DATA} DESTINATION ${ICONPANSVG_DIR})
install(FILES ${ICONSVG_PANINV_DATA} DESTINATION ${ICONSVG_DIR})
install(FILES ${ICONSVG_PANINV_DATA} DESTINATION ${ICONPANSVG_DIR})
install(FILES ${ICONSVG_RM_DATA} DESTINATION ${ICONAPPS_DIR})
install(FILES ${ICONSVG_ORG_DATA} DESTINATION ${ICONAPPS_DIR})
install(FILES ${ICON16_DATA_PANEL} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA_PANEL} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA_PANEL} DESTINATION ${ICON24_DIR})
install(FILES ${ICON32_DATA_PANEL} DESTINATION ${ICON32_DIR})
install(FILES ${ICON48_DATA_PANEL} DESTINATION ${ICON48_DIR})
install(FILES ${ICON64_DATA_PANEL} DESTINATION ${ICON64_DIR})
install(FILES ${ICON72_DATA_PANEL} DESTINATION ${ICON72_DIR})
install(FILES ${ICON96_DATA_PANEL} DESTINATION ${ICON96_DIR})
install(FILES ${ICON128_DATA_PANEL} DESTINATION ${ICON128_DIR})
install(FILES ${ICON256_DATA_PANEL} DESTINATION ${ICON256_DIR})
install(FILES ${ICON16_DATA_PANEL_LIGHT} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA_PANEL_LIGHT} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA_PANEL_LIGHT} DESTINATION ${ICON24_DIR})
install(FILES ${ICON32_DATA_PANEL_LIGHT} DESTINATION ${ICON32_DIR})
install(FILES ${ICON48_DATA_PANEL_LIGHT} DESTINATION ${ICON48_DIR})
install(FILES ${ICON64_DATA_PANEL_LIGHT} DESTINATION ${ICON64_DIR})
install(FILES ${ICON72_DATA_PANEL_LIGHT} DESTINATION ${ICON72_DIR})
install(FILES ${ICON96_DATA_PANEL_LIGHT} DESTINATION ${ICON96_DIR})
install(FILES ${ICON128_DATA_PANEL_LIGHT} DESTINATION ${ICON128_DIR})
install(FILES ${ICON256_DATA_PANEL_LIGHT} DESTINATION ${ICON256_DIR})

set(REMMINA_BINARY_PATH ${CMAKE_INSTALL_FULL_BINDIR}/remmina)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${REMMINA_APP_ID}.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/${REMMINA_APP_ID}.desktop @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina-file.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/remmina-file.desktop @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/remmina-file-wrapper.in
    ${CMAKE_CURRENT_BINARY_DIR}/remmina-file-wrapper @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${REMMINA_APP_ID}.desktop
    DESTINATION "${REMMINA_DATADIR}/applications")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina-file.desktop
    DESTINATION "${REMMINA_DATADIR}/applications")
install(FILES ${REMMINA_APP_ID}.appdata.xml
    DESTINATION "${REMMINA_DATADIR}/metainfo")
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/remmina-file-wrapper
    DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE
    )
if(WITH_MANPAGES)
    install(
        FILES remmina-file-wrapper.1
        DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1
        )
endif()

# Kiosk Session Files
option(WITH_KIOSK_SESSION "Enable Kiosk desktop session files" OFF)
if(WITH_KIOSK_SESSION)
    message(STATUS "Kiosk desktop session files will be installed")
    install(
        FILES remmina-gnome-xsession.desktop
        DESTINATION ${REMMINA_DATADIR}/xsessions
        RENAME remmina-gnome.desktop
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        )
    install(
        FILES remmina-gnome.desktop
        DESTINATION ${REMMINA_DATADIR}/applications
        RENAME remmina-gnome.desktop
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        )
    install(
        FILES remmina-gnome.session
        DESTINATION ${REMMINA_DATADIR}/gnome-session/sessions
        RENAME remmina-gnome.session
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        )
    install(
        FILES remmina-gnome
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        RENAME remmina-gnome
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
        )
    install(
        FILES gnome-session-remmina
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        RENAME gnome-session-remmina
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
        )
    if(WITH_MANPAGES)
        install(
            FILES remmina-gnome.1
            DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1
            )
        install(
            FILES gnome-session-remmina.1
            DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man1
            )
    endif()
else()
    message(STATUS "Kiosk desktop session files won't be installed")
endif()

# XML mime types
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
set( XDG_MIME_INSTALL_DIR "${REMMINA_DATADIR}/mime/packages" )
find_package( SharedMimeInfo )
if( SHARED_MIME_INFO_FOUND )
    install( FILES remmina-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif()
if(WITH_ICON_CACHE)
    gtk_update_icon_cache("${REMMINA_DATADIR}/icons/hicolor")
endif()
if(WITH_UPDATE_DESKTOP_DB)
    find_program(PROG_UPDATE_DESKTOP_DATABASE update-desktop-database)
    if( PROG_UPDATE_DESKTOP_DATABASE )
        install(
            CODE
            "message(STATUS \"Updating desktop file database\")"
            CODE
            "execute_process(COMMAND ${PROG_UPDATE_DESKTOP_DATABASE}
            -q ${CMAKE_INSTALL_PREFIX}/share/applications)"
            )
    endif()
endif()
