SD = proto util
SD_NO_TEST = proto util
if WANT_SERVER
SD += server
SD_NO_TEST += server
endif
if WANT_CLIENT
SD += client
SD_NO_TEST += client
endif
SD += po test
SD_NO_TEST += po

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = $(SD)
EXTRA_DIST = autogen.sh

all-local:
	$(MAKE) -C po update-po

install-recursive: SUBDIRS = $(SD_NO_TEST)

if HAVE_LCOV
.PHONY: lcov

lcov: check
	lcov -c -q --directory . --output-file lcov.info --no-external
	genhtml lcov.info --output-directory lcov
	@echo "Open file://$$PWD/lcov/index.html in your browser"

clean-local:
	rm -rf lcov.info lcov
endif
