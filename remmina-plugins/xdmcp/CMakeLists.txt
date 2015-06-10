# See LICENSE and COPYING files for copyright and license details.

set(REMMINA_PLUGIN_XDMCP_SRCS
	xdmcp_plugin.c
	)

add_library(remmina-plugin-xdmcp ${REMMINA_PLUGIN_XDMCP_SRCS})
set_target_properties(remmina-plugin-xdmcp PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-xdmcp PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-xdmcp ${REMMINA_COMMON_LIBRARIES})

install(TARGETS remmina-plugin-xdmcp DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-xdmcp-ssh.png 16x16/emblems/remmina-xdmcp.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-xdmcp-ssh.png 22x22/emblems/remmina-xdmcp.png DESTINATION ${APPICON22_EMBLEMS_DIR})
