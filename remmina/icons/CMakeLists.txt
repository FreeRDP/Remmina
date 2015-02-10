# icons/remmina - The GTK+ Remote Desktop Client
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


set(APPICON16_ACTIONS_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/actions")
set(APPICON16_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/emblems")
set(APPICON22_ACTIONS_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/actions")
set(APPICON22_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/emblems")

set(APPICON16_ACTIONS_DATA
	16x16/actions/remmina-fit-window.png
	16x16/actions/remmina-scale.png
	16x16/actions/remmina-fullscreen.png
	16x16/actions/remmina-switch-page.png
	16x16/actions/remmina-pin-up.png
	16x16/actions/remmina-pin-down.png
	)

set(APPICON16_EMBLEMS_DATA
	16x16/emblems/remmina-sftp.png
	)

set(APPICON22_ACTIONS_DATA
	22x22/actions/remmina-fit-window.png
	22x22/actions/remmina-scale.png
	22x22/actions/remmina-fullscreen.png
	22x22/actions/remmina-switch-page.png
	)

set(APPICON22_EMBLEMS_DATA
	22x22/emblems/remmina-sftp.png
	)

install(FILES ${APPICON16_ACTIONS_DATA} DESTINATION ${APPICON16_ACTIONS_DIR})
install(FILES ${APPICON16_EMBLEMS_DATA} DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES ${APPICON22_ACTIONS_DATA} DESTINATION ${APPICON22_ACTIONS_DIR})
install(FILES ${APPICON22_EMBLEMS_DATA} DESTINATION ${APPICON22_EMBLEMS_DIR})

