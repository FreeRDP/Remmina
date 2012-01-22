
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(TestBigEndian)

#set(PACKAGE_NAME           "LibVNCServer")
#set(FULL_PACKAGE_NAME      "LibVNCServer")
#set(PACKAGE_VERSION        "0.9.8.2")
#set(PROJECT_BUGREPORT_PATH "http://sourceforge.net/projects/libvncserver")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/common)

find_package(ZLIB)
find_package(JPEG)
find_package(GnuTLS)
find_package(Threads)
find_package(X11)
find_library(LIBGCRYPT_LIBRARIES gcrypt)

if(Threads_FOUND)
  option(TIGHTVNC_FILETRANSFER "Enable filetransfer" ON)
endif(Threads_FOUND)
if(ZLIB_FOUND)
  set(LIBVNCSERVER_HAVE_LIBZ 1)
endif(ZLIB_FOUND)
if(JPEG_FOUND)
  set(LIBVNCSERVER_HAVE_LIBJPEG 1)
endif(JPEG_FOUND)
option(LIBVNCSERVER_ALLOW24BPP "Allow 24 bpp" ON)
if(GNUTLS_FOUND)
  set(LIBVNCSERVER_WITH_CLIENT_TLS 1)
endif(GNUTLS_FOUND)
if(LIBGCRYPT_LIBRARIES)
  message(STATUS "Found libgcrypt: ${LIBGCRYPT_LIBRARIES}")
  set(LIBVNCSERVER_WITH_CLIENT_GCRYPT 1)
endif(LIBGCRYPT_LIBRARIES)

check_include_file("fcntl.h"       LIBVNCSERVER_HAVE_FCNTL_H)
check_include_file("netinet/in.h"  LIBVNCSERVER_HAVE_NETINET_IN_H)
check_include_file("sys/socket.h"  LIBVNCSERVER_HAVE_SYS_SOCKET_H)
check_include_file("sys/stat.h"    LIBVNCSERVER_HAVE_SYS_STAT_H)
check_include_file("sys/time.h"    LIBVNCSERVER_HAVE_SYS_TIME_H)
check_include_file("sys/types.h"   LIBVNCSERVER_HAVE_SYS_TYPES_H)
check_include_file("sys/wait.h"    LIBVNCSERVER_HAVE_SYS_WAIT_H)
check_include_file("unistd.h"      LIBVNCSERVER_HAVE_UNISTD_H)

# headers needed for check_type_size()
check_include_file("arpa/inet.h"   HAVE_ARPA_INET_H)
check_include_file("stdint.h"      HAVE_STDINT_H)
check_include_file("stddef.h"      HAVE_STDDEF_H)
check_include_file("sys/types.h"   HAVE_SYS_TYPES_H)

check_function_exists(gettimeofday    LIBVNCSERVER_HAVE_GETTIMEOFDAY)

if(CMAKE_USE_PTHREADS_INIT)
	set(LIBVNCSERVER_HAVE_LIBPTHREAD 1)
endif(CMAKE_USE_PTHREADS_INIT)
if(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
	# socklen_t
	list(APPEND CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
endif(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
if(HAVE_ARPA_INET_H)
	# in_addr_t
	list(APPEND CMAKE_EXTRA_INCLUDE_FILES "arpa/inet.h")
endif(HAVE_ARPA_INET_H)

check_type_size(pid_t     LIBVNCSERVER_PID_T)
check_type_size(size_t    LIBVNCSERVER_SIZE_T)
check_type_size(socklen_t LIBVNCSERVER_SOCKLEN_T)
check_type_size(in_addr_t LIBVNCSERVER_IN_ADDR_T)

if(NOT HAVE_LIBVNCSERVER_IN_ADDR_T)
	set(LIBVNCSERVER_NEED_INADDR_T 1)
endif(NOT HAVE_LIBVNCSERVER_IN_ADDR_T)

TEST_BIG_ENDIAN(LIBVNCSERVER_WORDS_BIGENDIAN)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbconfig.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbconfig.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbint.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbint.h)

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(LIBVNCCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libvncclient)

set(LIBVNCCLIENT_SOURCES
	${LIBVNCCLIENT_DIR}/cursor.c
	${LIBVNCCLIENT_DIR}/listen.c
	${LIBVNCCLIENT_DIR}/rfbproto.c
	${LIBVNCCLIENT_DIR}/sockets.c
	${LIBVNCCLIENT_DIR}/vncviewer.c
	${COMMON_DIR}/minilzo.c
	${LIBVNCCLIENT_DIR}/tls.c
)

add_library(vncclient STATIC ${LIBVNCCLIENT_SOURCES})

if(WIN32)
	set(ADDITIONAL_LIBS ws2_32)
endif(WIN32)

target_link_libraries(vncclient ${ADDITIONAL_LIBS} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES})

