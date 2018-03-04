# - Try to find the shared-mime-info package
#
#  SHARED_MIME_INFO_MINIMUM_VERSION - Set this to the minimum version you need, default is 0.18
#
# Once done this will define
#
#  SHARED_MIME_INFO_FOUND - system has the shared-mime-info package
#  UPDATE_MIME_DATABASE_EXECUTABLE - the update-mime-database executable

# Copyright (c) 2007, Pino Toscano, <toscano.pino@tiscali.it>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# the minimum version of shared-mime-database we require
if (NOT SHARED_MIME_INFO_MINIMUM_VERSION)
    set(SHARED_MIME_INFO_MINIMUM_VERSION "0.18")
endif (NOT SHARED_MIME_INFO_MINIMUM_VERSION)

if (UPDATE_MIME_DATABASE_EXECUTABLE)

    # in cache already
    set(SHARED_MIME_INFO_FOUND TRUE)

else (UPDATE_MIME_DATABASE_EXECUTABLE)

    include (MacroEnsureVersion)

    find_program (UPDATE_MIME_DATABASE_EXECUTABLE NAMES update-mime-database)

    if (UPDATE_MIME_DATABASE_EXECUTABLE)

        exec_program (${UPDATE_MIME_DATABASE_EXECUTABLE} ARGS -v RETURN_VALUE _null OUTPUT_VARIABLE _smiVersionRaw)

        string(REGEX REPLACE "update-mime-database \\([a-zA-Z\\-]+\\) ([0-9]\\.[0-9]+).*"
            "\\1" smiVersion "${_smiVersionRaw}")
        set (SHARED_MIME_INFO_FOUND TRUE)
    endif (UPDATE_MIME_DATABASE_EXECUTABLE)

    if (SHARED_MIME_INFO_FOUND)
        if (NOT SharedMimeInfo_FIND_QUIETLY)
            message(STATUS "Found shared-mime-info version: ${smiVersion}")
            macro_ensure_version(${SHARED_MIME_INFO_MINIMUM_VERSION} ${smiVersion} _smiVersion_OK)
            if (NOT _smiVersion_OK)
                message(FATAL_ERROR "The found version of shared-mime-info (${smiVersion}) is below the minimum required (${SHARED_MIME_INFO_MINIMUM_VERSION})")
            endif (NOT _smiVersion_OK)

        endif (NOT SharedMimeInfo_FIND_QUIETLY)
    else (SHARED_MIME_INFO_FOUND)
        if (SharedMimeInfo_FIND_REQUIRED)
            message(FATAL_ERROR "Could NOT find shared-mime-info. See http://freedesktop.org/wiki/Software/shared-mime-info.")
        endif (SharedMimeInfo_FIND_REQUIRED)
    endif (SHARED_MIME_INFO_FOUND)

endif (UPDATE_MIME_DATABASE_EXECUTABLE)

mark_as_advanced(UPDATE_MIME_DATABASE_EXECUTABLE)


macro(UPDATE_XDG_MIMETYPES _path)
    get_filename_component(_xdgmimeDir "${_path}" NAME)
    if("${_xdgmimeDir}" STREQUAL packages )
        get_filename_component(_xdgmimeDir "${_path}" PATH)
    else("${_xdgmimeDir}" STREQUAL packages )
        set(_xdgmimeDir "${_path}")
    endif("${_xdgmimeDir}" STREQUAL packages )

    install(CODE "
    set(DESTDIR_VALUE \"\$ENV{DESTDIR}\")
    if (NOT DESTDIR_VALUE)
        # under Windows relative paths are used, that's why it runs from CMAKE_INSTALL_PREFIX
        execute_process(COMMAND ${UPDATE_MIME_DATABASE_EXECUTABLE} ${_xdgmimeDir}
            WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}\")
    endif (NOT DESTDIR_VALUE)
    ")
endmacro (UPDATE_XDG_MIMETYPES)
