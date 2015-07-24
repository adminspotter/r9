SD = proto server
if WANT_CLIENT
if WANT_BUNDLED_XERCES
SD += xerces-c
endif
if WANT_BUNDLED_LIBGLW
SD += glw
endif
SD += client
endif

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = $(SD)
EXTRA_DIST = autogen.sh
