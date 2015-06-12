# icons/remmina - The GTK+ Remote Desktop Client
# See LICENSE and COPYING files for copyright and license details.


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

