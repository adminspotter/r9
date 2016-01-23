SD = proto server
SD_NO_TEST = proto server
if WANT_CLIENT
if WANT_BUNDLED_LIBGLW
SD += glw
SD_NO_TEST += glw
endif
SD += client
SD_NO_TEST += client
endif
if WANT_LOCALES
SD += po
SD_NO_TEST += po
endif
SD += test

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = $(SD)
EXTRA_DIST = autogen.sh

install-recursive: SUBDIRS = $(SD_NO_TEST)
