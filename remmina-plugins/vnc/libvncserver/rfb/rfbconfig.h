#ifndef _RFB_RFBCONFIG_H
/* #undef _RFB_RFBCONFIG_H */
 
/* rfb/rfbconfig.h. Generated automatically by cmake. */

/* Enable 24 bit per pixel in native framebuffer */
#define LIBVNCSERVER_ALLOW24BPP  1 

/* work around when write() returns ENOENT but does not mean it */
/* #undef LIBVNCSERVER_ENOENT_WORKAROUND */

/* Define to 1 if you have the <fcntl.h> header file. */
#define LIBVNCSERVER_HAVE_FCNTL_H  1 

/* Define to 1 if you have the `gettimeofday' function. */
#define LIBVNCSERVER_HAVE_GETTIMEOFDAY  1 

/* Define to 1 if you have the `jpeg' library (-ljpeg). */
#define LIBVNCSERVER_HAVE_LIBJPEG  1 

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define LIBVNCSERVER_HAVE_LIBPTHREAD  1 

/* Define to 1 if you have the `z' library (-lz). */
#define LIBVNCSERVER_HAVE_LIBZ  1 

/* Define to 1 if you have the <netinet/in.h> header file. */
#define LIBVNCSERVER_HAVE_NETINET_IN_H  1 

/* Define to 1 if you have the <sys/socket.h> header file. */
#define LIBVNCSERVER_HAVE_SYS_SOCKET_H  1 

/* Define to 1 if you have the <sys/stat.h> header file. */
#define LIBVNCSERVER_HAVE_SYS_STAT_H  1 

/* Define to 1 if you have the <sys/time.h> header file. */
#define LIBVNCSERVER_HAVE_SYS_TIME_H  1 

/* Define to 1 if you have the <sys/types.h> header file. */
#define LIBVNCSERVER_HAVE_SYS_TYPES_H  1 

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define LIBVNCSERVER_HAVE_SYS_WAIT_H  1 

/* Define to 1 if you have the <unistd.h> header file. */
#define LIBVNCSERVER_HAVE_UNISTD_H  1 

/* Need a typedef for in_addr_t */
/* #undef LIBVNCSERVER_NEED_INADDR_T */

/* Define to the full name and version of this package. */
#define LIBVNCSERVER_PACKAGE_STRING  " "

/* Define to the version of this package. */
#define LIBVNCSERVER_PACKAGE_VERSION  ""

/* Define to 1 if libgcrypt is present */
#define LIBVNCSERVER_WITH_CLIENT_GCRYPT 1

/* Define to 1 if GnuTLS is present */
#define LIBVNCSERVER_WITH_CLIENT_TLS 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef LIBVNCSERVER_WORDS_BIGENDIAN */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
//#ifndef __cplusplus
/* #undef inline */
//#endif

/* Define to `int' if <sys/types.h> does not define. */
#define HAVE_LIBVNCSERVER_PID_T 1
#ifndef HAVE_LIBVNCSERVER_PID_T
typedef int pid_t;
#endif

/* The type for size_t */
#define HAVE_LIBVNCSERVER_SIZE_T 1
#ifndef HAVE_LIBVNCSERVER_SIZE_T
typedef int size_t;
#endif

/* The type for socklen */
#define HAVE_LIBVNCSERVER_SOCKLEN_T 1
#ifndef HAVE_LIBVNCSERVER_SOCKLEN_T
typedef int socklen_t;
#endif

/* once: _RFB_RFBCONFIG_H */
#endif
