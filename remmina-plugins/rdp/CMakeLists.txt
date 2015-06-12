# See LICENSE and COPYING files for copyright and license details.

set(CMAKE_THREAD_PREFER_PTHREAD)
find_package(Threads)

set(REMMINA_PLUGIN_RDP_SRCS
	rdp_plugin.c
	rdp_plugin.h
	rdp_event.c
	rdp_event.h
	rdp_file.c
	rdp_file.h
	rdp_settings.c
	rdp_settings.h
	rdp_gdi.c
	rdp_gdi.h
	rdp_graphics.c
	rdp_graphics.h
	rdp_cliprdr.c
	rdp_cliprdr.h
	rdp_channels.c
	rdp_channels.h
	)

add_library(remmina-plugin-rdp ${REMMINA_PLUGIN_RDP_SRCS})
set_target_properties(remmina-plugin-rdp PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-rdp PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${FREERDP_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-rdp ${REMMINA_COMMON_LIBRARIES} ${FREERDP_LIBRARIES})

install(TARGETS remmina-plugin-rdp DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-rdp-ssh.png 16x16/emblems/remmina-rdp.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-rdp-ssh.png 22x22/emblems/remmina-rdp.png DESTINATION ${APPICON22_EMBLEMS_DIR})
