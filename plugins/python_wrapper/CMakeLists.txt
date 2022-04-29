# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2016-2018 Denis Ollier
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
#  In addition, as a special exception, the copyright holders give
#  permission to link the code of portions of this program with the
#  OpenSSL library under certain conditions as described in each
#  individual source file, and distribute linked combinations
#  including the two.
#  You must obey the GNU General Public License in all respects
#  for all of the code used other than OpenSSL. *  If you modify
#  file(s) with this exception, you may extend this exception to your
#  version of the file(s), but you are not obligated to do so. *  If you
#  do not wish to do so, delete this exception statement from your
#  version. *  If you delete this exception statement from all source
#  files in the program, then also delete it here.

set(REMMINA_PLUGIN_PYTHON_WRAPPER_SRCS
    python_wrapper_common.c
    python_wrapper_common.h
    python_wrapper_entry.c
    python_wrapper_entry.h
    python_wrapper_file.c
    python_wrapper_file.h
    python_wrapper_plugin.c
    python_wrapper_plugin.h
    python_wrapper_pref.c
    python_wrapper_pref.h
    python_wrapper_protocol.c
    python_wrapper_protocol.h
    python_wrapper_protocol_widget.c
    python_wrapper_protocol_widget.h
    python_wrapper_remmina.c
    python_wrapper_remmina.h
    python_wrapper_remmina_file.c
    python_wrapper_remmina_file.h
    python_wrapper_secret.c
    python_wrapper_secret.h
    python_wrapper_tool.c
    python_wrapper_tool.h
    )

add_library(remmina-plugin-python_wrapper ${REMMINA_PLUGIN_PYTHON_WRAPPER_SRCS})
set_target_properties(remmina-plugin-python_wrapper PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-python_wrapper PROPERTIES NO_SONAME 1)

message(${REMMINA_COMMON_INCLUDE_DIRS})
include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${PYTHON_WRAPPER_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-python_wrapper ${REMMINA_COMMON_LIBRARIES} ${PYTHON_WRAPPER_LIBRARIES})

install(TARGETS remmina-plugin-python_wrapper DESTINATION ${REMMINA_PLUGINDIR})

target_include_directories(remmina-plugin-python_wrapper PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-python_wrapper ${PYTHON_LIBRARIES})
