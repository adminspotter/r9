SD = proto
SD_NO_TEST = proto
if WANT_SERVER
SD += server
SD_NO_TEST += server
endif
if WANT_CLIENT
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
