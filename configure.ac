#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([mkpdf], [0.0.1], [rich@sky-visions.com], [mkpdf])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/mkpdf.cpp])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB([png], [png_get_io_ptr])
AC_CHECK_LIB(z,zlibVersion,,AC_MSG_ERROR([Cannot find libz]))

# Get xml2 library and include locations
PKG_CHECK_MODULES([LIBXML2], [libxml-2.0 >= 2.6],,AC_MSG_ERROR([Caannot find libxml2]))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
