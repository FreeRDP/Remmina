# remmina-plugin-telepathy - The GTK+ Remote Desktop Client
# See LICENSE and COPYING files for copyright and license details.

set(REMMINA_PLUGIN_TELEPATHY_SRCS
	telepathy_plugin.c
	)

add_library(remmina-plugin-telepathy ${REMMINA_PLUGIN_TELEPATHY_SRCS})
set_target_properties(remmina-plugin-telepathy PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-telepathy PROPERTIES NO_SONAME 1)

find_required_package(GTK3)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${TELEPATHY_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-telepathy ${REMMINA_COMMON_LIBRARIES} ${TELEPATHY_LIBRARIES})

install(TARGETS remmina-plugin-telepathy DESTINATION ${REMMINA_PLUGINDIR})
