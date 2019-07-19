# Remmina - The GTK+ Remote Desktop Client
#
#   Taken from stackoverflow https://stackoverflow.com/questions/15706318/check-if-cmake-module-exists
#
#   This makes use of the fact that if a package configuration file isn't found for VAR, then a cache variable VAR_DIR is set to VAR_DIR-NOTFOUND. So if the package configuration file is found, either this variable isn't defined, or it's set to a valid path (regardless of whether the find_package finds the requested package).
#
#   So, if you do
#
#   CheckHasModule(Spurious)
#   CheckHasModule(Threads)
#   message("\${HAS_MODULE_Spurious} - ${HAS_MODULE_Spurious}")
#   message("\${HAS_MODULE_Threads} - ${HAS_MODULE_Threads}")
#
#   your output should be:
#
#       ${HAS_MODULE_Spurious} - FALSE
#       ${HAS_MODULE_Threads} - TRUE
#
#
#
# Copyright (C) 2019 Antenore Gatta, Giovanni Panozzo
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

function(CheckHasModule Module)
  find_package(${Module} QUIET)
  if(NOT DEFINED ${Module}_DIR)
    set(HAS_MODULE_${Module} TRUE PARENT_SCOPE)
  elseif(${Module}_DIR)
    set(HAS_MODULE_${Module} TRUE PARENT_SCOPE)
  else()
    set(HAS_MODULE_${Module} FALSE PARENT_SCOPE)
  endif()
endfunction()
