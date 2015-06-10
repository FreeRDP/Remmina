# desktop/remmina - The GTK+ Remote Desktop Client
# See LICENSE and COPYING files for copyright and license details.

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

install(FILES remmina.desktop DESTINATION "${REMMINA_DATADIR}/applications")
install(FILES remmina.appdata.xml DESTINATION "${REMMINA_DATADIR}/appdata")
