# Gettext support: Create/Update pot file and
#
# To use: INCLUDE(Gettext)
#
# Most of the gettext support code is from FindGettext.cmake of cmake,
# but it is included here because:
#
# 1. Some system like RHEL5 does not have FindGettext.cmake
# 2. Bug of GETTEXT_CREATE_TRANSLATIONS make it unable to be include in 'All'
# 3. It does not support xgettext
#
#===================================================================
# Variables:
#  XGETTEXT_OPTIONS: Options pass to xgettext
#      Default:  --language=C --keyword=_ --keyword=N_ --keyword=C_:1c,2 --keyword=NC_:1c,2
#  GETTEXT_MSGMERGE_EXECUTABLE: the full path to the msgmerge tool.
#  GETTEXT_MSGFMT_EXECUTABLE: the full path to the msgfmt tool.
#  GETTEXT_FOUND: True if gettext has been found.
#  XGETTEXT_EXECUTABLE: the full path to the xgettext.
#  XGETTEXT_FOUND: True if xgettext has been found.
#
#===================================================================
# Macros:
# GETTEXT_CREATE_POT(potFile
#    [OPTION xgettext_options]
#    SRC list_of_source_file_that_contains_msgid
# )
#
# Generate .pot file.
#    OPTION xgettext_options: Override XGETTEXT_OPTIONS
#
# * Produced targets: pot_file
#
#-------------------------------------------------------------------
# GETTEXT_CREATE_TRANSLATIONS ( outputFile [ALL] locale1 ... localeN
#     [COMMENT comment] )
#
#     This will create a target "translations" which will convert the
#     given input po files into the binary output mo file. If the
#     ALL option is used, the translations will also be created when
#     building the default target.
#
# * Produced targets: translations
#-------------------------------------------------------------------
FIND_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
FIND_PROGRAM(XGETTEXT_EXECUTABLE xgettext)
IF (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE )
    SET(GETTEXT_FOUND TRUE)
ELSE (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE)
    SET(GETTEXT_FOUND FALSE)
    IF (GetText_REQUIRED)
	MESSAGE(FATAL_ERROR "GetText not found")
    ENDIF (GetText_REQUIRED)
ENDIF (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE )
IF(XGETTEXT_EXECUTABLE)
    SET(XGETTEXT_FOUND TRUE)
ELSE(XGETTEXT_EXECUTABLE)
    MESSAGE(STATUS "xgettext not found.")
    SET(XGETTTEXT_FOUND FALSE)
ENDIF(XGETTEXT_EXECUTABLE)
IF(NOT DEFINED XGETTEXT_OPTIONS)
    SET(XGETTEXT_OPTIONS --language=C --keyword=_ --keyword=N_ --keyword=C_:1c,2 --keyword=NC_:1c,2 -s)
ENDIF(NOT DEFINED XGETTEXT_OPTIONS)
IF(XGETTEXT_FOUND)
    MACRO(GETTEXT_CREATE_POT _potFile _pot_options )
	SET(_xgettext_options_list)
	SET(_src_list)
	SET(_src_list_abs)
	SET(_stage "SRC")
	FOREACH(_pot_option ${_pot_options} ${ARGN})
	    IF(_pot_option STREQUAL "OPTION")
		SET(_stage "OPTION")
	    ELSEIF(_pot_option STREQUAL "SRC")
		SET(_stage "SRC")
	    ELSE(_pot_option STREQUAL "OPTION")
		IF(_stage STREQUAL "OPTION")
		    SET(_xgettext_options_list ${_xgettext_options_list} ${_pot_option})
		ELSE(_stage STREQUAL "OPTION")
		    FILE(RELATIVE_PATH _relFile ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${_pot_option})
		    GET_FILENAME_COMPONENT(_absFile ${_pot_option} ABSOLUTE)
		    SET(_src_list ${_src_list} ${_relFile})
		    SET(_src_list_abs ${_src_list_abs} ${_absFile})
		ENDIF(_stage STREQUAL "OPTION")
	    ENDIF(_pot_option STREQUAL "OPTION")
	ENDFOREACH(_pot_option ${_pot_options} ${ARGN})
	IF (_xgettext_options_list)
	    SET(_xgettext_options ${_xgettext_options_list})
	ELSE(_xgettext_options_list)
	    SET(_xgettext_options ${XGETTEXT_OPTIONS})
	ENDIF(_xgettext_options_list)
	#MESSAGE("${XGETTEXT_EXECUTABLE} ${_xgettext_options_list} -o ${_potFile} ${_src_list}")
	ADD_CUSTOM_COMMAND(OUTPUT ${_potFile}
	    COMMAND ${XGETTEXT_EXECUTABLE} ${_xgettext_options} -o ${_potFile} ${_src_list}
	    DEPENDS ${_src_list_abs}
	    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    )
	ADD_CUSTOM_TARGET(pot_file
	    COMMAND ${XGETTEXT_EXECUTABLE} ${_xgettext_options_list} -o ${_potFile} ${_src_list}
	    DEPENDS ${_src_list_abs}
	    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    COMMENT "Extract translatable messages to ${_potFile}"
	    )
    ENDMACRO(GETTEXT_CREATE_POT _potFile _pot_options)
    MACRO(GETTEXT_CREATE_TRANSLATIONS _potFile _firstLang)
	SET(_gmoFiles)
	GET_FILENAME_COMPONENT(_potBasename ${_potFile} NAME_WE)
	GET_FILENAME_COMPONENT(_absPotFile ${_potFile} ABSOLUTE)
	SET(_addToAll)
	SET(_is_comment FALSE)
	FOREACH (_currentLang ${_firstLang} ${ARGN})
	    IF(_currentLang STREQUAL "ALL")
		SET(_addToAll "ALL")
	    ELSEIF(_currentLang STREQUAL "COMMENT")
		SET(_is_comment TRUE)
	    ELSEIF(_is_comment)
		SET(_is_comment FALSE)
		SET(_comment ${_currentLang})
	    ELSE()
		SET(_lang ${_currentLang})
		GET_FILENAME_COMPONENT(_absFile ${_currentLang}.po ABSOLUTE)
		GET_FILENAME_COMPONENT(_abs_PATH ${_absFile} PATH)
		SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo)
		#MESSAGE("_absFile=${_absFile} _abs_PATH=${_abs_PATH} _lang=${_lang} curr_bin=${CMAKE_CURRENT_BINARY_DIR}")
		ADD_CUSTOM_COMMAND(
		    OUTPUT ${_gmoFile}
		    COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s ${_absFile} ${_absPotFile}
		    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_gmoFile} ${_absFile}
		    DEPENDS ${_absPotFile} ${_absFile}
		    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		    )
		INSTALL(FILES ${_gmoFile} DESTINATION share/locale/${_lang}/LC_MESSAGES RENAME ${_potBasename}.mo)
		SET(_gmoFiles ${_gmoFiles} ${_gmoFile})
	    ENDIF()
	ENDFOREACH (_currentLang )
	IF(DEFINED _comment)
	    ADD_CUSTOM_TARGET(translations ${_addToAll} DEPENDS ${_gmoFiles} COMMENT ${_comment})
	ELSE(DEFINED _comment)
	    ADD_CUSTOM_TARGET(translations ${_addToAll} DEPENDS ${_gmoFiles})
	ENDIF(DEFINED _comment)
    ENDMACRO(GETTEXT_CREATE_TRANSLATIONS )
ENDIF(XGETTEXT_FOUND)

