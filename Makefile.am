SD = proto server
if WANT_CLIENT
if WANT_BUNDLED_LIBGLW
SD += glw
endif
SD += client
endif
if WANT_LOCALES
SD += po
endif
SD += test

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = $(SD)
EXTRA_DIST = autogen.sh
