# Remmina - The GTK+ Remote Desktop Client
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

find_suggested_package(Gettext)

function(gettext po_dir package_name)
	set(mo_files)
	file(GLOB po_files ${po_dir}/*.po)
	foreach(po_file ${po_files})
		get_filename_component(lang ${po_file} NAME_WE)
		set(mo_file ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
		add_custom_command(OUTPUT ${mo_file} COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${mo_file} ${po_file} DEPENDS ${po_file})
		install(FILES ${mo_file} DESTINATION share/locale/${lang}/LC_MESSAGES RENAME ${package_name}.mo)
		set(mo_files ${mo_files} ${mo_file})
	endforeach()
	set(translations-target "${package_name}-translations")
	add_custom_target(${translations-target} ALL DEPENDS ${mo_files})
endfunction()

