# Copyright © 2011 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

dnl Process this file with autoconf to create configure.

AC_PREREQ([2.59])
AC_INIT([glw], [8.0.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([dist-bzip2])

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([motif],
    [AS_HELP_STRING([--enable-motif],
        [use Motif widgets @<:@default=disabled@:>@])],
    [enable_motif="$enableval"],
    [enable_motif=no])
AM_CONDITIONAL([ENABLE_MOTIF], [test "$enable_motif" = yes])

AC_PATH_X
if test "x$x_includes" != "x"
then
  CFLAGS="$CFLAGS -I$x_includes"
fi
if test "x$x_libraries" != "x"
then
  LDFLAGS="$LDFLAGS -L$x_libraries"
fi

AC_CHECK_HEADER([GL/gl.h],
    [],
    AC_MSG_ERROR([GL not found]))
AC_SEARCH_LIBS([glBegin], [GL],
    [GL_LIBS=-lGL],
    AC_MSG_ERROR([GL required]))

AC_SEARCH_LIBS([IceOpenConnection], [ICE])
AC_SEARCH_LIBS([SmcOpenConnection], [SM])
AC_SEARCH_LIBS([XCreateWindow], [X11])
AC_SEARCH_LIBS([XtCreateWidget], [Xt])

MOTIF_CFLAGS=
if test "x$enable_motif" = xyes; then
    AC_PATH_PROG([MOTIF_CONFIG], [motif-config], [no])
    if test "x$MOTIF_CONFIG" != xno; then
        MOTIF_CFLAGS=`$MOTIF_CONFIG --cflags`
        MOTIF_LIBS=`$MOTIF_CONFIG --libs`
    else
        AC_CHECK_HEADER([Xm/PrimitiveP.h], [],
            [AC_MSG_ERROR([Can't locate Motif headers])])
        AC_SEARCH_LIBS([XmGetPixmap], [Xm], [MOTIF_LIBS="-lXm"],
            [AC_MSG_ERROR([Can't locate Motif Xm library])])
    fi
fi

dnl Set up C warning flags.  Copy of XORG_CWARNFLAGS.
if  test "x$GCC" = xyes ; then
    CWARNFLAGS="-Wall -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
-Wmissing-declarations -Wnested-externs -fno-strict-aliasing \
-Wbad-function-cast"
    case `$CC -dumpversion` in
    3.4.* | 4.*)
	CWARNFLAGS="$CWARNFLAGS -Wold-style-definition -Wdeclaration-after-statement"
	;;
    esac
else
    AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
    if test "x$SUNCC" = "xyes"; then
	CWARNFLAGS="-v"
    fi
fi
CFLAGS="$CFLAGS $CWARNFLAGS"

AC_SEARCH_LIBS([XextAddDisplay], [Xext], [x11_enabled=yes], [enable_x11=no])

AC_CONFIG_FILES()
AC_OUTPUT([
	Makefile
])
