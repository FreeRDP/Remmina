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

find_path(PTHREAD_INCLUDE_DIR NAMES pthread.h)

find_library(PTHREAD_LIBRARY NAMES pthread)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PTHREAD DEFAULT_MSG PTHREAD_LIBRARY PTHREAD_INCLUDE_DIR)

if(PTHREAD_FOUND)
	set(PTHREAD_LIBRARIES ${PTHREAD_LIBRARY})
	set(PTHREAD_INCLUDE_DIRS ${PTHREAD_INCLUDE_DIR})
endif()

mark_as_advanced(PTHREAD_INCLUDE_DIR PTHREAD_LIBRARY)

