#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Revision9], [0.1], [], [r9], [])
AC_CONFIG_SRCDIR([client/cache.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall])

AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR

AC_LANG(C++)

# Libtool init
LT_INIT([pic-only])
LT_LANG([C++])

# Check for c++11 language support
#AX_CXX_COMPILE_STDCXX_11
# TODO:  alter macro above to test C++11 library support

# Arguments
AC_ARG_ENABLE(editres, AS_HELP_STRING([--enable-editres], [Enable Editres support]), [], [enable_editres="yes"])

AC_ARG_WITH(tcl, AS_HELP_STRING([--with-tcl], [Build the Tcl server module]))
AC_ARG_WITH(perl, AS_HELP_STRING([--with-perl], [Build the Perl server module]))
AC_ARG_WITH(mysql, AS_HELP_STRING([--with-mysql], [Build the MySQL server module]))
AC_ARG_WITH(pgsql, AS_HELP_STRING([--with-pgsql], [Build the PostgreSQL server module]))
AC_ARG_WITH(client, AS_HELP_STRING([--with-client], [Build the client program]))

AC_ARG_WITH(xerces,
            AS_HELP_STRING([--with-xerces=<path>],
                           [Build using the system libxerces-c (default 'bundled')]),
            [
              if test "x$with_xerces_includes" == "x"
              then
                with_xerces_includes="${with_xerces}/${includedir}"
              fi
              if test "x$with_xerces_libraries" == "x"
              then
                with_xerces_libraries="${with_xerces}/${libdir}"
              fi
            ],
            [with_xerces="bundled"])
AC_ARG_WITH(xerces-includes,
            AS_HELP_STRING([--with-xerces-includes=<path>],
                           [Use libxerces-c includes at <path>]))
AC_ARG_WITH(xerces-libraries,
            AS_HELP_STRING([--with-xerces-libraries=<path>],
                           [Use libxerces-c libraries at <path>]))

AC_ARG_WITH(GLw,
            AS_HELP_STRING([--with-GLw=<path>],
                           [Build using the system libGLw (default 'bundled')]),
            [
              if test "x$with_GLw_includes" == "x"
              then
                with_GLw_includes="${with_GLw}/${includedir}"
              fi
              if test "x$with_GLw_libraries" == "x"
              then
                with_GLw_libraries="${with_GLw}/${libdir}"
              fi
            ],
            [with_GLw="bundled"])
AC_ARG_WITH(GLw-includes,
            AS_HELP_STRING([--with-GLw-includes=<path>],
                           [Use libGLw includes at <path>]))
AC_ARG_WITH(GLw-libraries,
            AS_HELP_STRING([--with-GLw-libraries=<path>],
                           [Use libGLw libraries at <path>]))

AC_ARG_WITH(eigen,
            AS_HELP_STRING([--with-eigen=<path>],
                           [Build using system Eigen installation at <path> (default 'bundled')]),
            [],
            [with_eigen="bundled"])

# Check for libraries

# Client program
if test "$with_client" == "yes"
then
  AC_MSG_NOTICE([Building with client enabled])

  CLIENT_CXXFLAGS_SAVE="$CXXFLAGS"
  CLIENT_LDFLAGS_SAVE="$LDFLAGS"
  CLIENT_LIBS_SAVE="$LIBS"
  CLIENT_CXXFLAGS=""
  CLIENT_LDLIBS=""

  AC_PATH_X

  # TODO:  get a good non-main function for libm
  AC_SEARCH_LIBS([main], [m],
      [CLIENT_LDLIBS="$ac_cv_search_main $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libm not valid, can not build client program.])])

  if test "$with_xerces" != "bundled"
  then
    # We should use the system libxerces-c
    XERCES_CXXFLAGS_SAVE="$CXXFLAGS"
    CXXFLAGS="-I$with_xerces_includes"
    AC_CHECK_HEADERS([xercesc/sax/AttributeList.hpp],
        [
            CLIENT_CXXFLAGS="$XERCES_CXXFLAGS_SAVE -I$with_xerces_includes"
        ],
        [
            use_bundled_xerces="yes"
            CXXFLAGS="$XERCES_CXXFLAGS_SAVE"
        ])
    XERCES_LDFLAGS_SAVE="$LDFLAGS"
    LDFLAGS="$LDFLAGS -L$with_xerces_libraries"
    AC_SEARCH_LIBS([XMLPlatformUtils::Initialize], [xerces-c],
        [
            CLIENT_LDFLAGS="$XERCES_LDFLAGS_SAVE -L$with_xerces_libraries"
            CLIENT_LDLIBS="$ac_cv_search_XMLPlatformUtils__Initialize $CLIENT_LDLIBS"
        ],
        [
            use_bundled_xerces="yes"
            LDFLAGS="$XERCES_LDFLAGS_SAVE"
        ])
    if test "x$use_bundled_xerces" == "xyes"
    then
      AC_MSG_NOTICE([no valid libxerces-c installation found, enabling bundled copy.])
      with_xerces="bundled"
      subdirs="xercesc $subdirs"
    fi
  fi

  AC_SEARCH_LIBS([XpmReadFileToPixmap], [Xpm],
      [CLIENT_LDLIBS="$ac_cv_search_XpmReadFileToPixmap $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXpm not valid, can not build client program.])])
  CLIENT_LDLIBS="-lSM -lICE $CLIENT_LDLIBS"
  if test "$enable_editres" == "yes"
  then
    AC_SEARCH_LIBS([_XEditResCheckMessages], [Xmu],
        [CLIENT_LDLIBS="$ac_cv_search__XEditResCheckMessages $CLIENT_LDLIBS"],
        [AC_MSG_ERROR([libXmu not valid, can not enable editres support.])],
        [$CLIENT_LDLIBS])
  fi
  AC_SEARCH_LIBS([XmCreateMainWindow], [Xm],
      [CLIENT_LDLIBS="$ac_cv_search_XmCreateMainWindow -lXext -lXt -lX11 $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXm not valid, can not build client program.])],
      [-lXext -lXt -lX11 $CLIENT_LDLIBS])
  AC_SEARCH_LIBS([glNewList], [GL],
      [CLIENT_LDLIBS="$ac_cv_search_glNewList $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libGL not valid, can not build client program.])],
      [$CLIENT_LDLIBS])
  AC_SEARCH_LIBS([glutSolidSphere], [glut],
      [CLIENT_LDLIBS="$ac_cv_search_glutSolidSphere $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libglut not valid, can not build client program.])],
      [$CLIENT_LDLIBS])
  if test "$with_libGLw" == "yes"
  then
    # We should use the system libGLw
    AC_CHECK_HEADERS([GLwMDrawA.h],
        [],
        [use_bundled_libGLw="yes"])
    AC_SEARCH_LIBS([GLwDrawingAreaMakeCurrent], [GLw],
        [CLIENT_LDLIBS="$ac_cv_search_GLwDrawingAreaMakeCurrent $CLIENT_LDLIBS"],
        [use_bundled_libGLw="yes"],
        [$CLIENT_LDLIBS])
    if test "$use_bundled_libGLw" == "yes"
    then
      AC_MSG_NOTICE([no valid libGLw installation found, using bundled copy.])
    fi
  fi

  LIBS="$CLIENT_LIBS_SAVE"
  LDFLAGS="$CLIENT_LDFLAGS_SAVE"
  CXXFLAGS="$CLIENT_CXXFLAGS_SAVE"

  AC_SUBST(CLIENT_CXXFLAGS)
  AC_SUBST(CLIENT_LDFLAGS)
  AC_SUBST(CLIENT_LDLIBS)
fi
AM_CONDITIONAL([WANT_BUNDLED_XERCES], [test "x$use_bundled_xerces" == "xyes"])
AM_CONDITIONAL([WANT_BUNDLED_LIBGLW], [test "x$use_bundled_libGLw" == "xyes"])
AM_CONDITIONAL([WANT_CLIENT], [test "x$with_client" == "xyes"])

# MySQL server module
if test "$with_mysql" == "yes"
then
  MYSQL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_CHECK_LIB([mysqlclient_r], [mysql_init])
  if test "$ac_cv_lib_mysqlclient_r_mysql_init" != "yes"
  then
    AC_MSG_ERROR([libmysqlclient_r not valid, can not enable MySQL module.])
  else
    MYSQL_LDLIBS="$LIBS"
    AC_SUBST(MYSQL_LDLIBS)
    want_mysql="yes"
  fi
  LIBS="$MYSQL_LIBS_SAVE"
fi
AM_CONDITIONAL([WANT_MYSQL], [test "x$want_mysql" == "xyes"])

# PostgreSQL server module
if test "$with_pgsql" == "yes"
then
  PGSQL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_CHECK_LIB([pq], [PQsetdbLogin])
  if test "$ac_cv_lib_pq_PQsetdbLogin" != "yes"
  then
    AC_MSG_ERROR([libpq not valid, can not enable PostgreSQL module.])
  else
    PGSQL_LDLIBS="$LIBS"
    AC_SUBST(PGSQL_LDLIBS)
    want_pgsql="yes"
  fi
  LIBS="$PGSQL_LIBS_SAVE"
fi
AM_CONDITIONAL([WANT_PGSQL], [test "x$want_pgsql" == "xyes"])

# Tcl server module
if test "$with_tcl" == "yes"
then
  TCL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_CHECK_LIB([tcl], [Tcl_CreateInterp])
  if test "$ac_cv_lib_tcl_Tcl_CreateInterp" != "yes"
  then
    AC_MSG_ERROR([libtcl not valid, can not enable Tcl module.])
  else
    TCL_LDLIBS="$LIBS"
    AC_SUBST(TCL_LDLIBS)
    want_tcl="yes"
  fi
  LIBS="$TCL_LIBS_SAVE"
fi
AM_CONDITIONAL([WANT_TCL], [test "x$want_tcl" == "xyes"])

# Perl server module
if test "$with_perl" == "yes"
then
  AX_PROG_PERL_MODULES([ExtUtils::Embed],
      [
          PERL_CFLAGS=$(perl -MExtUtils::Embed -e ccopts)
	  PERL_LDFLAGS=$(perl -MExtUtils::Embed -e ldopts)
          AC_SUBST(PERL_CFLAGS)
          AC_SUBST(PERL_LDFLAGS)
          want_perl="yes"
      ],
      [AC_MSG_ERROR([perl does not have ExtUtils::Embed available.  Can not embed it.])])
fi
AM_CONDITIONAL([WANT_PERL], [test "x$want_perl" == "xyes"])

AC_CHECK_LIB([pthread], [pthread_create])
if test "$ac_cv_lib_pthread_pthread_create" != "yes"
then
  AC_MSG_ERROR([can not continue without libpthread.])
fi

AC_CHECK_LIB([dl], [dlopen])
if test "$ac_cv_lib_dl_dlopen" != "yes"
then
  AC_MSG_ERROR([can not continue without libdl.])
fi

if test "$with_eigen" != ""
then
  EIGEN_CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -I$with_eigen"
  AC_CHECK_HEADERS([Eigen/Core], [],
      [
          AC_MSG_NOTICE([no valid Eigen installation found, using bundled copy.])
          # All of the $srcdir type variables seem to be relative.
          # Per the autoconf docs we're guaranteed to be running
          # configure from the top directory of the source distro, so
          # we'll take an absolute path where we are.
          ABSOLUTE_SRCDIR=$(pwd)
          CXXFLAGS="$EIGEN_CXXFLAGS_SAVE -I${ABSOLUTE_SRCDIR}/eigen"
      ])
fi

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h byteswap.h endian.h fcntl.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2 getcwd gethostname gettimeofday htonll memset mkdir ntohll select socket strdup strerror strtoull])

AC_CONFIG_FILES([Makefile
                 client/Makefile
                 client/motif/Makefile
                 proto/Makefile
                 server/Makefile
                 server/classes/Makefile
                 server/classes/actions/Makefile
                 server/classes/modules/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
