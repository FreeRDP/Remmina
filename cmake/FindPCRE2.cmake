find_path(PCRE2_INCLUDE_DIR
  NAMES pcre2.h
  PATHS ../../../../libs
        /usr
  PATH_SUFFIXES include)

find_library(PCRE2_LIBRARY
  NAMES pcre2-8
  PATHS ../../../../libs
        /usr
  PATH_SUFFIXES lib)

if (PCRE2_INCLUDE_DIR AND PCRE2_LIBRARY)
  message(STATUS "Found pcre2 headers at ${PCRE2_INCLUDE_DIR}")
  message(STATUS "Found pcre2 libraries at ${PCRE2_LIBRARY}")
  set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
  set(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
  set(PCRE2_FOUND yes)
else()
  set(PCRE2_INCLUDE_DIRS)
  set(PCRE2_LIBRARIES)
  set(PCRE2_FOUND no)
endif()
