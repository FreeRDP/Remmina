# See LICENSE and COPYING files for copyright and license details.

cmake_minimum_required(VERSION 2.8)

set(CMAKE_COLOR_MAKEFILE ON)

project(Remmina C)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindPkgConfig)
include(CheckCCompilerFlag)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

include(FindOptionalPackage)
include(ConfigOptions)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	if(WITH_SSE2)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
	endif()
	CHECK_C_COMPILER_FLAG(-Wno-unused-result Wno-unused-result)
	if(Wno-unused-result)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
	endif()
	CHECK_C_COMPILER_FLAG(-Wno-unused-but-set-variable Wno-unused-but-set-variable)
	if(Wno-unused-but-set-variable)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
	endif()
	CHECK_C_COMPILER_FLAG(-Wno-unused-function Wno-unused-function)
	if(Wno-unused-function)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
	endif()
endif()

check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(netdb.h HAVE_NETDB_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/un.h HAVE_SYS_UN_H)
check_include_files(errno.h HAVE_ERRNO_H)

include_directories(.)
include_directories(remmina/include)

# Versioning
set(WITH_LIBRARY_VERSIONING "ON")
set(REMMINA_VERSION_MAJOR "1")
set(REMMINA_VERSION_MINOR "2")
set(REMMINA_VERSION_REVISION "0")
set(REMMINA_VERSION_SUFFIX "rcgit.3")
set(REMMINA_VERSION "${REMMINA_VERSION_MAJOR}.${REMMINA_VERSION_MINOR}.${REMMINA_VERSION_REVISION}-${REMMINA_VERSION_SUFFIX}")

set(REMMINA_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}")
set(REMMINA_LOCALEDIR "${CMAKE_INSTALL_FULL_DATADIR}/locale")
set(REMMINA_PLUGINDIR "${CMAKE_INSTALL_FULL_LIBDIR}/remmina/plugins")
set(REMMINA_UIDIR "${CMAKE_INSTALL_FULL_DATADIR}/remmina/ui")

find_required_package(GTK)
find_required_package(PTHREAD)
find_suggested_package(GCRYPT)
find_suggested_package(AVAHI)

if(WITH_TRACE_CALLS)
	message(STATUS "Enabling trace calls.")
	add_definitions(-DWITH_TRACE_CALLS)
endif()

if(GCRYPT_FOUND)
	add_definitions(-DHAVE_LIBGCRYPT)
endif()

if(AVAHI_FOUND)
	add_definitions(-DHAVE_LIBAVAHI_UI)
	add_definitions(-DHAVE_LIBAVAHI_CLIENT)
endif()

if(GTK_FOUND)
	add_subdirectory(remmina)
	add_subdirectory(remmina-plugins)
	add_subdirectory(remmina-plugins-gnome)
endif()

if(WITH_EXAMPLES)
	message(STATUS "Enabling examples and test plugins.")
	add_definitions(-DWITH_EXAMPLES)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
