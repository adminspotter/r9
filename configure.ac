#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Revision9], [0.1], [], [r9], [])
AC_CONFIG_SRCDIR([client/cache.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall subdir-objects])

AC_GNU_SOURCE
AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

m4_ifdef([AM_GNU_GETTEXT], [AM_GNU_GETTEXT([external])])
AM_GNU_GETTEXT_VERSION(0.17)

AC_LANG(C++)

# Libtool init
LT_INIT([pic-only])
LT_LANG([C++])
LT_SYS_MODULE_EXT

# Set the host type for OSX-specific flags
AC_CANONICAL_HOST

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


# Arguments
AC_ARG_WITH([server],
            AS_HELP_STRING([--with-server], [Build the server program]),
            [],
            [with_server="no"])

AC_ARG_WITH([tcl],
            AS_HELP_STRING([--with-tcl], [Build the Tcl server module]),
            [],
            [with_tcl="no"])
AC_ARG_WITH([perl],
            AS_HELP_STRING([--with-perl], [Build the Perl server module.  NOTE: the Perl interpreter can not currently run a simple script, so it may cause crashing in the server.]),
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

  # We're currently only doing l10n in the client, so we'll check that
  # header here.
  AC_CHECK_HEADERS([libintl.h], [want_locales="yes"])
  AC_CHECK_FUNCS([setlocale])

  # OpenGL libs
  case $host_os in
    darwin*)
      CLIENT_LDLIBS="$CLIENT_LDLIBS -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo"
      ;;
    *)
      AC_SEARCH_LIBS([glBindVertexArray], [GL],
          [CLIENT_LDLIBS="$ac_cv_search_glBindVertexArray $CLIENT_LDLIBS"],
          [AC_MSG_ERROR([libGL not valid, can not build client program.])],
          [$CLIENT_LDFLAGS])

      AC_SEARCH_LIBS([gluPerspective], [GLU],
          [CLIENT_LDLIBS="$ac_cv_search_gluPerspective $CLIENT_LDLIBS"],
          [AC_MSG_ERROR([libGLU not valid, can not build client program.])],
          [$CLIENT_LDFLAGS])
      ;;
  esac

  # Check for GLM headers
  GLM_CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CXXFLAGS"
  AC_CHECK_HEADERS([glm/vec3.hpp glm/mat4x4.hpp glm/gtc/matrix_transform.hpp glm/gtc/type_ptr.hpp glm/gtx/quaternion.hpp], [], [AC_MSG_NOTICE([no valid GLM installation found, can not continue])])
  CPPFLAGS="$GLM_CPPFLAGS_SAVE"

  # Check for GLFW
  GLFW_CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CXXFLAGS"
  AC_CHECK_HEADERS([GLFW/glfw3.h], [], [AC_MSG_NOTICE([no valid GLFW headers found, can not continue])])
  CPPFLAGS="$GLFW_CPPFLAGS_SAVE"
  AC_SEARCH_LIBS([glfwInit], [glfw],
      [CLIENT_LDLIBS="$ac_cv_search_glfwInit $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([ not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  # Check for Freetype2
  FREETYPE_CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CXXFLAGS"
  AC_CHECK_HEADERS([ft2build.h], [], [AC_MSG_NOTICE([no valid Freetype2 headers found, can not continue])])
  CPPFLAGS="$FREETYPE_CPPFLAGS_SAVE"
  AC_SEARCH_LIBS([FT_Init_FreeType], [freetype],
      [CLIENT_LDLIBS="$ac_cv_search_FT_Init_FreeType $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([ not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

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
AM_CONDITIONAL([WANT_CLIENT], [test "x$with_client" == "xyes"])
AM_CONDITIONAL([WANT_LOCALES], [test "x$want_locales" == "xyes"])

if test "$with_server" == "yes"
then
  AC_MSG_NOTICE([building with server enabled])

  SERVER_LIBS_SAVE="$LIBS"
  SERVER_LDLIBS=""

  # We can use libwrap in the server console, if it's available
  WRAP_LIBS_SAVE="$LIBS"
  LIBS=""
  AC_CHECK_HEADERS([tcpd.h], [have_libwrap="yes"])
  AC_SEARCH_LIBS([hosts_ctl], [wrap], [have_libwrap="yes"])
  if test "x$have_libwrap" == "xyes"
  then
    AC_DEFINE(HAVE_LIBWRAP, [1], [Enable libwrap support])
    WRAP_LDLIBS="$LIBS"
    AC_SUBST(WRAP_LDLIBS)
  fi
  LIBS="$WRAP_LIBS_SAVE"

  # MySQL server module
  if test "$with_mysql" == "yes"
  then
    MYSQL_LIBS_SAVE="$LIBS"
    LIBS=""
    AC_SEARCH_LIBS([mysql_init], [mysqlclient_r],
        [
            MYSQL_LDLIBS="$ac_cv_search_mysql_init"
            AC_SUBST(MYSQL_LDLIBS)
            want_mysql="yes"
        ],
        [AC_MSG_ERROR([libmysqlclient_r not valid, can not enable MySQL module.])])
    LIBS="$MYSQL_LIBS_SAVE"
  fi

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

  AC_SEARCH_LIBS([dlopen], [dl],
      [test "$ac_cv_search_dlopen" = "none required" || SERVER_LDLIBS="$ac_cv_search_dlopen $LIBS"],
      [AC_MSG_ERROR([can not build without dlopen.])])

  if test "$with_eigen" != ""
  then
    EIGEN_INCLUDES="-I$with_eigen"
  fi
  EIGEN_CPPFLAGS_SAVE="$CPPFLAGS"
  EIGEN_CXXFLAGS_SAVE="$CXXFLAGS"
  CPPFLAGS="$CXXFLAGS $EIGEN_INCLUDES"
  CXXFLAGS="$CXXFLAGS $EIGEN_INCLUDES"
  AC_CHECK_HEADERS([Eigen/Core], [],
      [AC_MSG_ERROR([no valid Eigen installation found, can not continue])])
  CPPFLAGS="$EIGEN_CPPFLAGS_SAVE"
  CXXFLAGS="$EIGEN_CXXFLAGS_SAVE"

  AC_MSG_NOTICE([using SERVER_LDLIBS=$SERVER_LDLIBS])

  LIBS="$SERVER_LIBS_SAVE"

  AC_SUBST(SERVER_LDLIBS)
fi
AM_CONDITIONAL([WANT_SERVER], [test "x$with_server" == "xyes"])
AM_CONDITIONAL([HAVE_LIBWRAP], [test "x$have_libwrap" == "xyes"])
AM_CONDITIONAL([WANT_MYSQL], [test "x$want_mysql" == "xyes"])
AM_CONDITIONAL([WANT_PGSQL], [test "x$want_pgsql" == "xyes"])
AM_CONDITIONAL([WANT_TCL], [test "x$want_tcl" == "xyes"])
AM_CONDITIONAL([WANT_PERL], [test "x$want_perl" == "xyes"])

AC_SEARCH_LIBS([pthread_create], [pthread],
    [test "$ac_cv_search_pthread_create" = "none required" || LIBS="$ac_cv_search_pthread_create $LIBS"],
    [AC_MSG_ERROR([can not build without pthreads.])])

AC_CONFIG_SUBDIRS([test/gmock-1.7.0])

# Checks for header files.
AC_CHECK_HEADERS([architecture/byte_order.h arpa/inet.h byteswap.h endian.h execinfo.h fcntl.h grp.h limits.h netdb.h netinet/in.h pwd.h signal.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/resource.h sys/socket.h sys/syslimits.h sys/time.h syslog.h unistd.h])

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
AC_CHECK_FUNCS([backtrace dup2 getcwd gethostname gettimeofday getrlimit htonll kill memset mkdir ntohll select socket strdup strerror strtoull])

# One of the mocks in our test suite needs a possibly-different
# prototype, depending on the system we're building on.
AC_MSG_CHECKING([prototype of getnameinfo])
GETNAMEINFO_CXXFLAGS_SAVE="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Wall"
AC_COMPILE_IFELSE(
    [
        AC_LANG_PROGRAM(
            [
#include <string.h>
#include <netdb.h>

struct sockaddr_in a;
socklen_t b = sizeof(struct sockaddr_in), d, f;
char *c = "hello", *e = "goodbye";
unsigned int g = UINT_MAX;
            ],
            [
  int ret;

  d = strlen(c);
  f = strlen(e);
  ret = getnameinfo((const struct sockaddr *)&a, b, c, d, e, f, g);
            ])
    ],
    [
        AC_MSG_RESULT([unsigned flags])
        AC_DEFINE(
            [GETNAMEINFO_FLAGS_TYPE],
            [unsigned int],
            [Define to the type of getnameinfo's flags argument])
    ],
    [
        AC_MSG_RESULT([int flags])
        AC_DEFINE(
            [GETNAMEINFO_FLAGS_TYPE],
            [int],
            [Define to the type of getnameinfo's flags argument])
    ])
CXXFLAGS="$GETNAMEINFO_CXXFLAGS_SAVE"

AC_CONFIG_FILES([Makefile
                 client/Makefile
                 client/shaders/Makefile
                 client/ui/Makefile
                 po/Makefile.in
                 proto/Makefile
                 server/Makefile
                 server/classes/Makefile
                 server/classes/actions/Makefile
                 server/classes/modules/Makefile
                 test/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
