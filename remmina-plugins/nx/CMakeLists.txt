# See LICENSE and COPYING files for copyright and license details.

set(REMMINA_PLUGIN_NX_SRCS
	nx_session.c
	nx_session.h
	nx_session_manager.c
	nx_session_manager.h
	nx_plugin.c
	nx_plugin.h
	)

add_library(remmina-plugin-nx ${REMMINA_PLUGIN_NX_SRCS})
set_target_properties(remmina-plugin-nx PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-nx PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${XKBFILE_INCLUDE_DIRS} ${LIBSSH_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-nx ${REMMINA_COMMON_LIBRARIES} ${XKBFILE_LIBRARIES} ${LIBSSH_LIBRARIES})

install(TARGETS remmina-plugin-nx DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-nx.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-nx.png DESTINATION ${APPICON22_EMBLEMS_DIR})
