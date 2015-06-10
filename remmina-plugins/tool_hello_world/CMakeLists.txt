# remmina-plugin-tool_hello_world - The GTK+ Remote Desktop Client
# See LICENSE and COPYING files for copyright and license details.
# Some rights reserved to Fabio Castelli


set(REMMINA_PLUGIN_SRCS
	plugin.h
	plugin.c
	)

add_library(remmina-plugin-tool_hello_world ${REMMINA_PLUGIN_SRCS})
set_target_properties(remmina-plugin-tool_hello_world PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-tool_hello_world PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-tool_hello_world ${REMMINA_COMMON_LIBRARIES})

install(TARGETS remmina-plugin-tool_hello_world DESTINATION ${REMMINA_PLUGINDIR})
