#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([archlist], [1.0])
AC_CONFIG_SRCDIR([main.cpp])
AM_INIT_AUTOMAKE

dnl AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# FIXME: Replace `main' with a function in `-linotifytools':
# AC_CHECK_LIB([inotifytools], [main])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([getcwd realpath strcasecmp strdup])

AC_CHECK_HEADER(magic.h)
dnl AC_CHECK_LIB([magic],[magic],[MAGIC_LIBS=-lmagic],AC_MSG_ERROR(magic not found))
AC_CHECK_LIB([magic], [main],[MAGIC_LIBS=-lmagic],AC_MSG_ERROR(magic not found))
AC_SUBST([MAGIC_LIBS])dnl

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
