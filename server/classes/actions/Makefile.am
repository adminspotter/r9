action_libs = libr9_actions.la

actionlibdir = $(pkglibdir)/server/actions

actionlib_LTLIBRARIES = $(action_libs)

libr9_actions_la_SOURCES = register.cc control_object.cc move.cc register.h
libr9_actions_la_LDFLAGS = -module -avoid-version

install-data-hook:
	cd $(actionlibdir) && rm -f $(action_libs)

clean-local:
	rm -f *.gcno *.gcda *.gcov
