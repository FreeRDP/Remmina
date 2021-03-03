# icons/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
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


set(APPICONSCALE_ACTIONS_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/actions")
set(APPICONSCALE_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/emblems")

set(APPICONSCALE_ACTIONS_DATA
    scalable/actions/remmina-camera-photo-symbolic.svg
    scalable/actions/remmina-connect-symbolic.svg
    scalable/actions/remmina-disconnect-symbolic.svg
    scalable/actions/remmina-document-save-symbolic.svg
    scalable/actions/remmina-document-send-symbolic.svg
    scalable/actions/remmina-duplicate-symbolic.svg
    scalable/actions/remmina-dynres-symbolic.svg
    scalable/actions/remmina-fit-window-symbolic.svg
    scalable/actions/remmina-fullscreen-symbolic.svg
    scalable/actions/remmina-multi-monitor-symbolic.svg
    scalable/actions/remmina-go-bottom-symbolic.svg
    scalable/actions/remmina-keyboard-symbolic.svg
    scalable/actions/remmina-pan-down-symbolic.svg
    scalable/actions/remmina-pan-up-symbolic.svg
    scalable/actions/remmina-pin-down-symbolic.svg
    scalable/actions/remmina-pin-up-symbolic.svg
    scalable/actions/remmina-preferences-system-symbolic.svg
    scalable/actions/remmina-scale-symbolic.svg
    scalable/actions/remmina-switch-page-symbolic.svg
    scalable/actions/remmina-system-run-symbolic.svg
    scalable/actions/view-list.svg
    )

set(APPICONSCALE_EMBLEMS_DATA
    scalable/emblems/remmina-sftp-symbolic.svg
    scalable/emblems/remmina-ssh-symbolic.svg
    )

install(FILES ${APPICONSCALE_ACTIONS_DATA} DESTINATION ${APPICONSCALE_ACTIONS_DIR})
install(FILES ${APPICONSCALE_EMBLEMS_DATA} DESTINATION ${APPICONSCALE_EMBLEMS_DIR})

if(WITH_ICON_CACHE)
    gtk_update_icon_cache("${REMMINA_DATADIR}/icons/hicolor")
endif()
