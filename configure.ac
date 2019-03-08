#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([structuredDataC], [0.1], [SamuelGfroerer@googlemail.com])

AC_CONFIG_SRCDIR([src/global/LinkedList.h])
AC_CONFIG_AUX_DIR([build-aux])

# Checks for programs.
AC_PROG_CC
# check for an archiver prog:
AM_PROG_AR

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h])
AC_CHECK_HEADER([m_pd.h],,[AC_MSG_FAILURE([missing m_pd.h. Is pd installed?])])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([memset select strerror])

# files being generated from *.in files:
AC_CONFIG_FILES([Makefile])

# use AUTOMAKE
# (which will generate a Makefile.in from Makefile.ac):
# -Wall -Werror: force correct Makefile.am
# foreign: dont complain if files missing for GNU standards
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# initialize libtool, since we are creating a library:
LT_INIT([disable-static])

AC_OUTPUT
