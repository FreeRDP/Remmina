# Try to find Libintl functionality
# Once done this will define
#
#  LIBINTL_FOUND - system has Libintl
#  LIBINTL_INCLUDE_DIR - Libintl include directory
#  LIBINTL_LIBRARIES - Libraries needed to use Libintl
#
# TODO: This will enable translations only if Gettext functionality is
# present in libc. Must have more robust system for release, where Gettext
# functionality can also reside in standalone Gettext library, or the one
# embedded within kdelibs (cf. gettext.m4 from Gettext source).

# Copyright (c) 2006, Chusslove Illich, <caslav.ilic@gmx.net>
# Copyright (c) 2007, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)
  set(Libintl_FIND_QUIETLY TRUE)
endif(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)

find_path(LIBINTL_INCLUDE_DIR libintl.h)

set(LIBINTL_LIB_FOUND FALSE)

if(LIBINTL_INCLUDE_DIR)
  include(CheckFunctionExists)
  check_function_exists(dgettext LIBINTL_LIBC_HAS_DGETTEXT)

  if (LIBINTL_LIBC_HAS_DGETTEXT)
    set(LIBINTL_LIBRARIES)
    set(LIBINTL_LIB_FOUND TRUE)
  else (LIBINTL_LIBC_HAS_DGETTEXT)
    find_library(LIBINTL_LIBRARIES NAMES intl libintl )
    if(LIBINTL_LIBRARIES)
      set(LIBINTL_LIB_FOUND TRUE)
    endif(LIBINTL_LIBRARIES)
  endif (LIBINTL_LIBC_HAS_DGETTEXT)

endif(LIBINTL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libintl  DEFAULT_MSG  LIBINTL_LIBRARIES  LIBINTL_LIB_FOUND)

mark_as_advanced(LIBINTL_INCLUDE_DIR  LIBINTL_LIBRARIES  LIBINTL_LIBC_HAS_DGETTEXT  LIBINTL_LIB_FOUND)

