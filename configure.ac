#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Revision9], [0.1], [], [r9], [])
AC_CONFIG_SRCDIR([client/cache.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall])

AC_GNU_SOURCE
AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_LANG(C++)

# Libtool init
LT_INIT([pic-only])
LT_LANG([C++])
LT_SYS_MODULE_EXT

# Check for c++11 language support
AX_CXX_COMPILE_STDCXX_11

# Check for 'code' member in std::ios_base::failure
AC_MSG_CHECKING([if std::ios_base::failure has the code member])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
        [
#include <cassert>
#include <system_error>
#include <ios>
        ],
        [
std::ios_base::failure f("whoa", std::error_code(5, std::system_category()));

assert(f.code().value() == 5);
        ])
    ],
    [
      AC_MSG_RESULT([yes])
      AC_DEFINE(HAVE_IOS_BASE_FAILURE_CODE, [1], [g++ has a bug in its std::ios_base::failure implementation])
    ],
    [AC_MSG_RESULT([no])])


# Find the compiled-in g++ extension include dir
if test "x$CXX" == "xg++"
then
  AC_MSG_CHECKING([g++ extension include directory])
  ax_gxx_include_dir=$(g++ --version 2>&1 | sed -ne '/Configured with/{s/^.*with-gxx-include-dir=//;s/ .*//;p;}')
  if test -s "$ax_gxx_include_dir"
  then
    GXX_EXT_CXXFLAGS="-I$ax_gxx_include_dir"
    AC_MSG_RESULT([$ax_gxx_include_dir])
    AC_SUBST(GXX_EXT_CXXFLAGS)
  else
    AC_MSG_RESULT([none found])
  fi
else
  AC_MSG_NOTICE([g++ not configured, skipping extension test])
fi


# Arguments
AC_ARG_ENABLE([editres],
              AS_HELP_STRING([--enable-editres], [Enable Editres support]),
              [],
              [enable_editres="yes"])

AC_ARG_WITH([tcl],
            AS_HELP_STRING([--with-tcl], [Build the Tcl server module]),
            [],
            [with_tcl="no"])
AC_ARG_WITH([perl],
            AS_HELP_STRING([--with-perl], [Build the Perl server module]),
            [],
            [with_perl="no"])
AC_ARG_WITH([mysql],
            AS_HELP_STRING([--with-mysql], [Build the MySQL server module]),
            [],
            [with_mysql="no"])
AC_ARG_WITH([pgsql],
            AS_HELP_STRING([--with-pgsql], [Build the PostgreSQL server module]),
            [],
            [with_pgsql="no"])

AC_ARG_WITH([client],
            AS_HELP_STRING([--with-client], [Build the client program]),
            [],
            [with_client="no"])

AC_ARG_WITH([xerces],
            AS_HELP_STRING([--with-xerces=<path>],
                           [Build using libxerces-c at <path>]),
            [
              if test "x$with_xerces_includes" == "x"
              then
                with_xerces_includes="${with_xerces}/include"
              fi
              if test "x$with_xerces_libraries" == "x"
              then
                with_xerces_libraries="${with_xerces}/lib"
              fi
            ],
            [])
AC_ARG_WITH([xerces-includes],
            AS_HELP_STRING([--with-xerces-includes=<path>],
                           [Use libxerces-c includes at <path>]))
AC_ARG_WITH([xerces-libraries],
            AS_HELP_STRING([--with-xerces-libraries=<path>],
                           [Use libxerces-c libraries at <path>]))

AC_ARG_WITH([GLw],
            AS_HELP_STRING([--with-GLw=<path>],
                           [Build using the system libGLw (default 'bundled')]),
            [
              if test "x$with_GLw_includes" == "x"
              then
                with_GLw_includes="${with_GLw}/include"
              fi
              if test "x$with_GLw_libraries" == "x"
              then
                with_GLw_libraries="${with_GLw}/lib"
              fi
            ],
            [with_GLw="bundled"])
AC_ARG_WITH([GLw-includes],
            AS_HELP_STRING([--with-GLw-includes=<path>],
                           [Use libGLw includes at <path>]))
AC_ARG_WITH([GLw-libraries],
            AS_HELP_STRING([--with-GLw-libraries=<path>],
                           [Use libGLw libraries at <path>]))

AC_ARG_WITH([eigen],
            AS_HELP_STRING([--with-eigen=<path>],
                           [Build using system Eigen installation at <path> (default 'bundled')]),
            [],
            [with_eigen="bundled"])

# Client program
if test "$with_client" == "yes"
then
  AC_MSG_NOTICE([building with client enabled])

  CLIENT_CXXFLAGS_SAVE="$CXXFLAGS"
  CLIENT_LDFLAGS_SAVE="$LDFLAGS"
  CLIENT_LIBS_SAVE="$LIBS"
  CLIENT_CXXFLAGS="$CXXFLAGS"
  CLIENT_INCLUDES=""
  CLIENT_LDLIBS=""

  AC_SEARCH_LIBS([sqrt], [m],
      [
          if test "x$ac_cv_search_sqrt" == "xnone required"
          then
            AC_MSG_NOTICE([libm not needed, skipping])
          else
            CLIENT_LDLIBS="$ac_cv_search_sqrt $CLIENT_LDLIBS"
          fi
      ],
      [AC_MSG_ERROR([libm not valid, can not build client program.])])

  AC_MSG_NOTICE([checking for xerces])
  # Try to use the specified libxerces-c
  if test "x$with_xerces_includes" != "x"
  then
    XERCES_INCLUDES="-I$with_xerces_includes"
  fi
  if test "x$with_xerces_libraries" != "x"
  then
    XERCES_LDFLAGS="-L$with_xerces_libraries"
  fi

  XERCES_CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CLIENT_CXXFLAGS $XERCES_INCLUDES"
  AC_CHECK_HEADERS([xercesc/sax/AttributeList.hpp],
      [CLIENT_INCLUDES="$CLIENT_INCLUDES $XERCES_INCLUDES"],
      [AC_MSG_ERROR([xerces includes not valid, can not build client program])])

  # This is an inline of a hacked version of AC_SEARCH_LIBS
  AS_VAR_PUSHDEF([ac_Search], [ac_cv_search_XMLPlatformUtils__Initialize])
  AC_CACHE_CHECK(
    [for library containing XMLPlatformUtils::Initialize],
    [ac_Search],
    [
      ac_func_search_save_LIBS=$LIBS
      AC_LANG_CONFTEST([AC_LANG_PROGRAM(
        [
#include <xercesc/util/PlatformUtils.hpp>
#ifdef XERCES_CPP_NAMESPACE_USE
XERCES_CPP_NAMESPACE_USE
#endif
        ],
        [
try
{
   XMLPlatformUtils::Initialize();
}
catch (...)
{
   // ...
}
XMLPlatformUtils::Terminate();
        ])])
      for ac_lib in '' xerces-c
      do
        if test -z "$ac_lib"
        then
          ac_res="none required"
        else
          ac_res=-l$ac_lib
          LIBS="-l$ac_lib $XERCES_LDFLAGS $ac_func_search_save_LIBS"
        fi
        AC_LINK_IFELSE([], [AS_VAR_SET([ac_Search], [$ac_res])])
        AS_VAR_SET_IF([ac_Search], [break])
      done
      AS_VAR_SET_IF([ac_Search], , [AS_VAR_SET([ac_Search], [no])])
      rm conftest.$ac_ext
      LIBS=$ac_func_search_save_LIBS
    ])
  ac_res=AS_VAR_GET([ac_Search])
  AS_IF(
    [test "$ac_res" != no],
    [test "$ac_res" = "none required" || LIBS="$ac_res $LIBS"],
    [AC_MSG_ERROR([libxerces-c not valid, can not build client program])])
  AS_VAR_POPDEF([ac_Search])
  CXXFLAGS="$XERCES_CXXFLAGS_SAVE"

  CLIENT_LDFLAGS="$CLIENT_LDFLAGS $XERCES_LDFLAGS"
  CLIENT_LDLIBS="$ac_cv_search_XMLPlatformUtils__Initialize $CLIENT_LDLIBS"

  AC_PATH_X
  if test "x$x_includes" != "x"
  then
    CLIENT_INCLUDES="$CLIENT_INCLUDES -I$x_includes"
  fi
  if test "x$x_libraries" != "x"
  then
    CLIENT_LDFLAGS="$CLIENT_LDFLAGS -L$x_libraries"
  fi

  AC_SEARCH_LIBS([XpmReadFileToPixmap], [Xpm],
      [CLIENT_LDLIBS="$ac_cv_search_XpmReadFileToPixmap $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXpm not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([IceOpenConnection], [ICE],
      [CLIENT_LDLIBS="$ac_cv_search_IceOpenConnection $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libICE not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([SmcOpenConnection], [SM],
      [CLIENT_LDLIBS="$ac_cv_search_SmcOpenConnection $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libSM not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([XCreateWindow], [X11],
      [CLIENT_LDLIBS="$ac_cv_search_XCreateWindow $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libX11 not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([XtCreateWidget], [Xt],
      [CLIENT_LDLIBS="$ac_cv_search_XtCreateWidget $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXt not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([XextAddDisplay], [Xext],
      [CLIENT_LDLIBS="$ac_cv_search_XextAddDisplay $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXext not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  if test "$enable_editres" == "yes"
  then
    AC_SEARCH_LIBS([_XEditResCheckMessages], [Xmu],
        [
            CLIENT_LDLIBS="$ac_cv_search__XEditResCheckMessages $CLIENT_LDLIBS"
            AC_DEFINE(WANT_EDITRES, [1], [Enable editres support])
        ],
        [AC_MSG_ERROR([libXmu not valid, can not enable editres support.])],
        [$CLIENT_LDFLAGS])
  fi

  AC_SEARCH_LIBS([XmCreateMainWindow], [Xm],
      [CLIENT_LDLIBS="$ac_cv_search_XmCreateMainWindow $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libXm not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([glNewList], [GL],
      [CLIENT_LDLIBS="$ac_cv_search_glNewList $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libGL not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([gluPerspective], [GLU],
      [CLIENT_LDLIBS="$ac_cv_search_gluPerspective $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libGLU not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_SEARCH_LIBS([glutSolidSphere], [glut],
      [CLIENT_LDLIBS="$ac_cv_search_glutSolidSphere $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([libglut not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  AC_MSG_NOTICE([checking for GLw])
  case $with_GLw in
    bundled)
      use_bundled_GLw="yes"
      ;;
    *)
      # Try to use the specified libGLw
      GLW_CXXFLAGS_SAVE="$CXXFLAGS"
      CXXFLAGS="$CLIENT_INCLUDES -I$with_GLw_includes"
      AC_CHECK_HEADERS([GL/GLwMDrawA.h],
          [],
          [use_bundled_GLw="yes"],
          [
              #include <X11/Xlib.h>
              #include <X11/StringDefs.h>
              #include <X11/Intrinsic.h>
          ])
      CXXFLAGS="$GLW_CXXFLAGS_SAVE"
      AC_SEARCH_LIBS([GLwDrawingAreaMakeCurrent], [GLw],
          [],
          [use_bundled_GLw="yes"],
          [$CLIENT_LDFLAGS -L$with_GLw_libraries])
  esac
  if test "$use_bundled_GLw" == "yes"
  then
    AC_MSG_NOTICE([using bundled GLw])
    subdirs="glw $subdirs"
    ac_configure_args="$ac_configure_args --enable-motif --disable-shared"
    AC_CONFIG_SUBDIRS([glw])
    ABSOLUTE_SRCDIR=$(pwd)
    CLIENT_INCLUDES="$CLIENT_INCLUDES -I${ABSOLUTE_SRCDIR}/glw"
    CLIENT_LDFLAGS="$CLIENT_LDFLAGS -L${ABSOLUTE_SRCDIR}/glw"
    CLIENT_LDLIBS="-lGLw $CLIENT_LDLIBS"
  else
    AC_MSG_NOTICE([GLw includes $with_GLw_includes, libraries $with_GLw_libraries])
    CLIENT_INCLUDES="$CLIENT_INCLUDES -I$with_GLw_includes"
    CLIENT_LDFLAGS="$CLIENT_LDFLAGS -L$with_GLw_libraries"
    CLIENT_LDLIBS="$ac_cv_search_GLwDrawingAreaMakeCurrent $CLIENT_LDLIBS"
  fi

  AC_MSG_NOTICE([using CLIENT_CXXFLAGS=$CLIENT_CXXFLAGS])
  AC_MSG_NOTICE([using CLIENT_INCLUDES=$CLIENT_INCLUDES])
  AC_MSG_NOTICE([using CLIENT_LDFLAGS=$CLIENT_LDFLAGS])
  AC_MSG_NOTICE([using CLIENT_LDLIBS=$CLIENT_LDLIBS])

  LIBS="$CLIENT_LIBS_SAVE"
  LDFLAGS="$CLIENT_LDFLAGS_SAVE"
  CXXFLAGS="$CLIENT_CXXFLAGS_SAVE"

  AC_SUBST(CLIENT_CXXFLAGS)
  AC_SUBST(CLIENT_INCLUDES)
  AC_SUBST(CLIENT_LDFLAGS)
  AC_SUBST(CLIENT_LDLIBS)
fi
AM_CONDITIONAL([WANT_BUNDLED_LIBGLW], [test "x$use_bundled_libGLw" == "xyes"])
AM_CONDITIONAL([WANT_CLIENT], [test "x$with_client" == "xyes"])

# MySQL server module
if test "$with_mysql" == "yes"
then
  MYSQL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_SEARCH_LIBS([mysql_init], [mysqlclient_r],
      [
          MYSQL_LDLIBS="$ac_cv_lib_mysqlclient_r_mysql_init"
          AC_SUBST(MYSQL_LDLIBS)
          want_mysql="yes"
      ],
      [AC_MSG_ERROR([libmysqlclient_r not valid, can not enable MySQL module.])])
  LIBS="$MYSQL_LIBS_SAVE"
fi
AM_CONDITIONAL([WANT_MYSQL], [test "x$want_mysql" == "xyes"])

# PostgreSQL server module
if test "$with_pgsql" == "yes"
then
  PGSQL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_SEARCH_LIBS([PQsetdbLogin], [pq],
      [
          PGSQL_LDLIBS="$ac_cv_search_PQsetdbLogin"
          AC_SUBST(PGSQL_LDLIBS)
          want_pgsql="yes"
      ],
      [AC_MSG_ERROR([libpq not valid, can not enable PostgreSQL module.])])
  LIBS="$PGSQL_LIBS_SAVE"
fi
AM_CONDITIONAL([WANT_PGSQL], [test "x$want_pgsql" == "xyes"])

# Tcl server module
if test "$with_tcl" == "yes"
then
  TCL_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_SEARCH_LIBS([Tcl_CreateInterp], [tcl],
      [
          TCL_LDLIBS="$ac_cv_search_Tcl_CreateInterp"
          AC_SUBST(TCL_LDLIBS)
          want_tcl="yes"
      ],
      [AC_MSG_ERROR([libtcl not valid, can not enable Tcl module.])])
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

AC_SEARCH_LIBS([pthread_create], [pthread],
    [CHECK_LDLIBS="$CHECK_LDLIBS $ac_cv_search_pthread_create"],
    [AC_MSG_ERROR([can not build without pthreads.])])

AC_SEARCH_LIBS([dlopen], [dl],
    [],
    [AC_MSG_ERROR([can not build without dlopen.])])

if test "$with_eigen" != ""
then
  EIGEN_INCLUDES="-I$with_eigen"
fi
EIGEN_CXXFLAGS_SAVE="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $EIGEN_INCLUDES"
AC_CHECK_HEADERS([Eigen/Core], [],
    [AC_MSG_NOTICE([no valid Eigen installation found, can not continue])])
CXXFLAGS="$EIGEN_CXXFLAGS_SAVE"

AC_CONFIG_SUBDIRS([test/gmock-1.7.0])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h byteswap.h endian.h execinfo.h ext/stdio_filebuf.h fcntl.h grp.h limits.h netdb.h netinet/in.h pwd.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/syslimits.h sys/time.h syslog.h unistd.h])

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
AC_CHECK_FUNCS([backtrace dup2 getcwd gethostname gettimeofday htonll memset mkdir ntohll select socket strdup strerror strtoull])

AC_CONFIG_FILES([Makefile
                 client/Makefile
                 client/motif/Makefile
                 client/pixmaps/Makefile
                 proto/Makefile
                 server/Makefile
                 server/classes/Makefile
                 server/classes/actions/Makefile
                 server/classes/modules/Makefile
                 test/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
