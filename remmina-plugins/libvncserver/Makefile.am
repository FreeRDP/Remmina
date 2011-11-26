if WITH_X11VNC
X11VNC=x11vnc
endif

SUBDIRS=libvncserver examples libvncclient vncterm classes client_examples test $(X11VNC)
DIST_SUBDIRS=libvncserver examples libvncclient vncterm classes client_examples test
EXTRA_DIST = CMakeLists.txt rfb/rfbint.h.cmake rfb/rfbconfig.h.cmake

bin_SCRIPTS = libvncserver-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libvncserver.pc libvncclient.pc

includedir=$(prefix)/include/rfb
#include_HEADERS=rfb.h rfbconfig.h rfbint.h rfbproto.h keysym.h rfbregion.h

include_HEADERS=rfb/rfb.h rfb/rfbconfig.h rfb/rfbint.h rfb/rfbproto.h \
	rfb/keysym.h rfb/rfbregion.h rfb/rfbclient.h

$(PACKAGE)-$(VERSION).tar.gz: dist

if HAVE_RPM
# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif

t:
	$(MAKE) -C test test

