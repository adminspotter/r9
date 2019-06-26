#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Revision9], [0.1], [], [r9], [])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall subdir-objects foreign])

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

# Check for std::put_time existence
AC_MSG_CHECKING([if std::put_time exists])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
        [
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
        ],
        [
std::time_t t = std::time(nullptr);

std::cout << std::put_time(std::localtime(&t), "%c %Z") << std::endl;
        ])
    ],
    [
      AC_MSG_RESULT([yes])
      AC_DEFINE(HAVE_STD_PUT_TIME, [1], [older versions of g++ do not provide std::put_time])
    ],
    [AC_MSG_RESULT([no])])


# Arguments
AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [Build binaries with debugging symbols]),
              [],
              [enable_debug="no"])
AC_ARG_ENABLE([profile],
              AS_HELP_STRING([--enable-profile], [Build binaries with profiling support]),
              [],
              [enable_profile="no"])

AC_ARG_WITH([server],
            AS_HELP_STRING([--with-server], [Build the server program]),
            [],
            [with_server="no"])

AC_ARG_WITH([mysql],
            AS_HELP_STRING([--with-mysql], [Build the MySQL server module]),
            [],
            [with_mysql="no"])
AC_ARG_WITH([pgsql],
            AS_HELP_STRING([--with-pgsql], [Build the PostgreSQL server module]),
            [],
            [with_pgsql="no"])

AC_ARG_WITH([lua],
            AS_HELP_STRING([--with-lua], [Build the Lua server module]),
            [],
            [with_lua="no"])
AC_ARG_WITH([python],
            AS_HELP_STRING([--with-python], [Build the Python server module]),
            [],
            [with_python="no"])
AC_ARG_WITH([tcl],
            AS_HELP_STRING([--with-tcl], [Build the Tcl server module]),
            [],
            [with_tcl="no"])

AC_ARG_WITH([client],
            AS_HELP_STRING([--with-client], [Build the client program]),
            [],
            [with_client="no"])

AC_ARG_WITH([glm-includes],
            AS_HELP_STRING([--with-glm-includes=<path>], [GLM include path]))

# Development flags (debugging, profiling)
if test "$enable_debug" == "yes"
then
  AC_MSG_NOTICE([building with debug enabled])
  DEBUG_CFLAGS="-g"
  DEBUG_CXXFLAGS="-g"
  DEBUG_LDFLAGS="-g"
fi

if test "$enable_profile" == "yes"
then
  if test "$CXX" == "clang++"
  then
    AC_SEARCH_LIBS([llvm_start_basic_block_tracing], [profile_rt],
        [
          if test "x$ac_cv_search_llvm_start_basic_block_tracing" == "xnone required"
          then
            AC_MSG_NOTICE([libprofile_rt not needed])
          else
            PROFILE_LDLIBS="$ac_cv_search_llvm_start_basic_block_tracing"
          fi
          PROFILE_CFLAGS="-fprofile-arcs -ftest-coverage"
          PROFILE_CXXFLAGS="-fprofile-arcs -ftest-coverage"
          PROFILE_LDFLAGS="--coverage"
          AC_MSG_NOTICE([building with profiling enabled])
        ],
        [AC_MSG_NOTICE([no libprofile_rt available, not enabling profiling])])
  else
    PROFILE_CFLAGS="-fprofile-arcs -ftest-coverage"
    PROFILE_CXXFLAGS="-fprofile-arcs -ftest-coverage"
    PROFILE_LDFLAGS="--coverage"
    PROFILE_LDLIBS="-lgcov"
    AC_MSG_NOTICE([building with profiling enabled])
  fi
fi

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

  # Check for GLFW
  GLFW_CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CXXFLAGS"
  AC_CHECK_HEADERS([GLFW/glfw3.h], [], [AC_MSG_ERROR([no valid GLFW headers found, can not continue])])
  CPPFLAGS="$GLFW_CPPFLAGS_SAVE"
  AC_SEARCH_LIBS([glfwInit], [glfw],
      [CLIENT_LDLIBS="$ac_cv_search_glfwInit $CLIENT_LDLIBS"],
      [AC_MSG_ERROR([ not valid, can not build client program.])],
      [$CLIENT_LDFLAGS])

  CLIENT_CXXFLAGS="$DEBUG_CXXFLAGS $PROFILE_CXXFLAGS $CLIENT_CXXFLAGS"
  CLIENT_LDFLAGS="$DEBUG_LDFLAGS $PROFILE_LDFLAGS $CLIENT_LDFLAGS"
  CLIENT_LDLIBS="$CLIENT_LDLIBS $PROFILE_LDLIBS"

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
  AC_SUBST(FREETYPE_LDLIBS)
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
    AC_CHECK_HEADERS([mysql.h], [],
        [AC_MSG_ERROR([no valid MySQL headers found, can not continue])])
    MYSQL_LIBS_SAVE="$LIBS"
    LIBS=""
    AC_SEARCH_LIBS([mysql_init], [mysqlclient_r mysqlclient],
        [
            MYSQL_LDLIBS="$ac_cv_search_mysql_init"
            AC_SUBST(MYSQL_LDLIBS)
            want_mysql="yes"
        ],
        [AC_MSG_ERROR([MySQL client lib not valid, can not enable MySQL module.])])
    LIBS="$MYSQL_LIBS_SAVE"
  fi

  # PostgreSQL server module
  if test "$with_pgsql" == "yes"
  then
    AC_CHECK_HEADERS([postgresql/libpq-fe.h], [],
        [AC_MSG_ERROR([no valid PostgreSQL headers found, can not continue])])
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

  # Lua server module
  if test "$with_lua" == "yes"
  then
    AC_CHECK_HEADERS([lua.h lauxlib.h lualib.h lua.hpp], [],
        [AC_MSG_ERROR([no valid Lua headers found, can not continue])])
    LUA_LIBS_SAVE="$LIBS"
    LIBS=""
    AC_SEARCH_LIBS([luaL_newstate], [lua lua5.2 lua5.3],
        [
            LUA_LDLIBS="$ac_cv_search_luaL_newstate"
            AC_SUBST(LUA_LDLIBS)
            want_lua="yes"
        ],
        [AC_MSG_ERROR([liblua not valid, can not enable Lua module.])])
    LIBS="$LUA_LIBS_SAVE"
  fi

  # Python server module
  if test "$with_python" == "yes"
  then
    AC_CHECK_PROG(PYTHON3_CONFIG_CHECK, [python3-config], [yes])
    if test "x$PYTHON3_CONFIG_CHECK" != "xyes"
    then
      AC_MSG_ERROR([No python3-config script found, can not continue])
    fi
    PYTHON_CFLAGS=$(python3-config --cflags)
    PYTHON_LDFLAGS=$(python3-config --ldflags)
    AC_SUBST(PYTHON_CFLAGS)
    AC_SUBST(PYTHON_LDFLAGS)
    want_python="yes"
  fi

  # Tcl server module
  if test "$with_tcl" == "yes"
  then
    AC_CHECK_HEADERS([tcl.h], [],
        [AC_MSG_ERROR([no valid Tcl headers found, can not continue])])
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

  AC_SEARCH_LIBS([dlopen], [dl],
      [test "$ac_cv_search_dlopen" = "none required" || SERVER_LDLIBS="$ac_cv_search_dlopen $LIBS"],
      [AC_MSG_ERROR([can not build without dlopen.])])

  SERVER_LDLIBS="$SERVER_LDLIBS $PROFILE_LDLIBS"

  AC_MSG_NOTICE([using SERVER_LDLIBS=$SERVER_LDLIBS])

  LIBS="$SERVER_LIBS_SAVE"

  AC_SUBST(SERVER_LDLIBS)
fi
AM_CONDITIONAL([WANT_SERVER], [test "x$with_server" == "xyes"])
AM_CONDITIONAL([HAVE_LIBWRAP], [test "x$have_libwrap" == "xyes"])
AM_CONDITIONAL([WANT_MYSQL], [test "x$want_mysql" == "xyes"])
AM_CONDITIONAL([WANT_PGSQL], [test "x$want_pgsql" == "xyes"])
AM_CONDITIONAL([WANT_LUA], [test "x$want_lua" == "xyes"])
AM_CONDITIONAL([WANT_PYTHON], [test "x$want_python" == "xyes"])
AM_CONDITIONAL([WANT_TCL], [test "x$want_tcl" == "xyes"])

# We want GLM in either the client or the server
if test "$with_client" = "yes" -o "$with_server" = "yes"
then
  # We use the GLM for our vector-related mathematics
  GLM_CPPFLAGS_SAVE="$CPPFLAGS"
  GLM_INCLUDES="-DGLM_FORCE_RADIANS"
  AS_IF([test "x$with_glm_includes" != "xno" && test "x$with_glm_includes" != "x"],
        [GLM_INCLUDES="-I$with_glm_includes $GLM_INCLUDES"])
  CXXFLAGS="$CXXFLAGS $GLM_INCLUDES"
  CPPFLAGS="$CXXFLAGS"
  AC_CHECK_HEADERS([glm/vec3.hpp glm/mat4x4.hpp glm/gtc/matrix_transform.hpp glm/gtc/type_ptr.hpp glm/gtx/quaternion.hpp], [], [AC_MSG_ERROR([no valid GLM installation found, can not continue])])
  CPPFLAGS="$GLM_CPPFLAGS_SAVE"
fi

# Make sure we have a new enough version of OpenSSL
AC_SEARCH_LIBS([EVP_PKEY_derive], [openssl ssl crypto],
    [test "$ac_cv_search_EVP_PKEY_derive" = "none required" || LIBS="$ac_cv_search_EVP_PKEY_derive $LIBS"],
    [AC_MSG_ERROR([can not build without OpenSSL >= 1.0.0])])
OPENSSL_CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$CXXFLAGS"
AC_CHECK_HEADERS([openssl/evp.h openssl/ec.h], [], [AC_MSG_ERROR([no valid OpenSSL installation found, can not continue])])
CPPFLAGS="$OPENSSL_CPPFLAGS_SAVE"

AC_SEARCH_LIBS([pthread_create], [pthread],
    [test "$ac_cv_search_pthread_create" = "none required" || LIBS="$ac_cv_search_pthread_create $LIBS"],
    [AC_MSG_ERROR([can not build without pthreads.])])

CFLAGS="$DEBUG_CXXFLAGS $PROFILE_CXXFLAGS $CFLAGS"
CXXFLAGS="$DEBUG_CXXFLAGS $PROFILE_CXXFLAGS $CXXFLAGS"
LDFLAGS="$DEBUG_LDFLAGS $PROFILE_LDFLAGS $LDFLAGS"
LIBS="$LIBS $PROFILE_LDLIBS"

AC_CONFIG_SUBDIRS([client/cuddly-gl])

# Checks for header files.
AC_CHECK_HEADERS([architecture/byte_order.h arpa/inet.h byteswap.h endian.h execinfo.h fcntl.h getopt.h grp.h inttypes.h limits.h netdb.h netinet/in.h pwd.h signal.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/resource.h sys/socket.h sys/syslimits.h sys/time.h syslog.h termios.h unistd.h])

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
AC_CHECK_FUNCS([backtrace dup2 getcwd gethostname getopt_long gettimeofday getrlimit htonll kill memset mkdir nanosleep ntohll select socket strdup strerror_r strtoull])

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
                 po/Makefile.in
                 proto/Makefile
                 server/Makefile
                 server/classes/Makefile
                 server/classes/actions/Makefile
                 server/classes/modules/Makefile
                 util/Makefile
                 test/Makefile
                 test/tap++/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_OUTPUT
