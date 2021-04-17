bin_PROGRAMS = r9_keygen

localedir = @datadir@/locale

r9_keygen_SOURCES = r9_keygen.cc
r9_keygen_CXXFLAGS = $(CXXFLAGS) -DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALE_DIR=\"$(localedir)\"
r9_keygen_LDADD = $(LOCALE_LDLIBS) ../proto/libr9_proto.la

clean-local:
	rm -f *.gcno *.gcda *.gcov
