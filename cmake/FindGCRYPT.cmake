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

find_path(GCRYPT_INCLUDE_DIR NAMES gcrypt.h)

find_library(GCRYPT_LIBRARY NAMES gcrypt)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GCRYPT DEFAULT_MSG GCRYPT_LIBRARY GCRYPT_INCLUDE_DIR)

set(GCRYPT_LIBRARIES ${GCRYPT_LIBRARY})
set(GCRYPT_INCLUDE_DIRS ${GCRYPT_INCLUDE_DIR})

mark_as_advanced(GCRYPT_INCLUDE_DIR GCRYPT_LIBRARY)

