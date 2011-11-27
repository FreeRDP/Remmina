# desktop/remmina - The GTK+ Remote Desktop Client
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

set(ICON16_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/apps")
set(ICON22_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/apps")
set(ICON24_DIR "${REMMINA_DATADIR}/icons/hicolor/24x24/apps")
set(ICON32_DIR "${REMMINA_DATADIR}/icons/hicolor/32x32/apps")
set(ICON48_DIR "${REMMINA_DATADIR}/icons/hicolor/48x48/apps")
set(ICONSVG_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/apps")

set(ICON16_DATA 16x16/apps/remmina.png)
set(ICON22_DATA 22x22/apps/remmina.png)
set(ICON24_DATA 24x24/apps/remmina.png)
set(ICON32_DATA 32x32/apps/remmina.png)
set(ICON48_DATA 48x48/apps/remmina.png)
set(ICONSVG_DATA scalable/apps/remmina.svg)

install(FILES ${ICON16_DATA} DESTINATION ${ICON16_DIR})
install(FILES ${ICON22_DATA} DESTINATION ${ICON22_DIR})
install(FILES ${ICON24_DATA} DESTINATION ${ICON24_DIR})
install(FILES ${ICON32_DATA} DESTINATION ${ICON32_DIR})
install(FILES ${ICON48_DATA} DESTINATION ${ICON48_DIR})
install(FILES ${ICONSVG_DATA} DESTINATION ${ICONSVG_DIR})

