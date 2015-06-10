# See LICENSE and COPYING files for copyright and license details.

set(LIBVNCSERVER_INCLUDE_DIRS)
set(LIBVNCSERVER_LIBRARIES vncclient)

set(REMMINA_PLUGIN_VNC_SRCS
	vnc_plugin.c
	)

add_library(remmina-plugin-vnc ${REMMINA_PLUGIN_VNC_SRCS})
set_target_properties(remmina-plugin-vnc PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-vnc PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${LIBVNCSERVER_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-vnc ${REMMINA_COMMON_LIBRARIES} ${LIBVNCSERVER_LIBRARIES})

install(TARGETS remmina-plugin-vnc DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-vnc-ssh.png 16x16/emblems/remmina-vnc.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-vnc-ssh.png 22x22/emblems/remmina-vnc.png DESTINATION ${APPICON22_EMBLEMS_DIR})
