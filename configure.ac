#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gles2apps], [0.1], [revogelaar@gmail.com])
AM_INIT_AUTOMAKE([foreign])
LT_INIT
AC_CONFIG_SRCDIR([gles2app1/src/gles2app1.c])
#AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [log10])

case "$host" in
  arm*-*-*)
	TARGET=ARM;
	;;
  mips*-*-linux*)
	TARGET=MIPS;
	;;
  i686-*-*)
	TARGET=686;
esac

# Checks for pkg-config
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([egl], [egl], [], [AC_MSG_ERROR([Unable to find libEGL])])
PKG_CHECK_MODULES([glesv2], [glesv2], [], [AC_MSG_ERROR([Unable to find libGLESv2])])
PKG_CHECK_MODULES([freetype2], [freetype2], [], [AC_MSG_ERROR([Unable to find libfreetype])])
PKG_CHECK_MODULES([libpng], [libpng >= 1.2.0], [], [AC_MSG_ERROR([Unable to find libpng])])

if test x$TARGET = x686; then
PKG_CHECK_MODULES([gdl], [gdl], [], [AC_MSG_ERROR([Unable to find libgdl])])
fi

AM_CONDITIONAL(IS686, test x$TARGET = x686)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h bcm_host.h refsw/nexus_config.h])

AC_DEFINE([EQUATION], ["$a > $b"],
            [Equation string.])


# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memset pow sqrt strcspn strdup strerror strstr strtol strtoul])

AC_CONFIG_FILES([
    Makefile
    gles2app1/src/Makefile
    gles2app2/src/Makefile
    gles2app3/src/Makefile
    gles2app4/src/Makefile
    gles2app5/src/Makefile
    gles2app6/src/Makefile
])

AC_OUTPUT

