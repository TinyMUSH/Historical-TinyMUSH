dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.62)
AC_INIT(TinyMUSH, 3.2, tinymush-bugs@lists.sourceforge.net)
AC_CONFIG_SRCDIR([rawdump.c])

../shtool echo -e "Configuring %B${PACKAGE_NAME}%b version %B${PACKAGE_VERSION}%b."
echo
echo "Copyright (c) 2005 : David Passmore, Lydia Leong, Robby Griffin,"
echo "                     Scott Dorr, Eddy Beaupre."
echo "         More info : http://tinymush.sourceforge.net"
echo

AC_PROG_CC
AC_PATH_PROGS(AWK, mawk gawk nawk awk)
if test $AWK = ""; then
    AC_MSG_ERROR(You do not have an awk interpreter installed. Sorry.)
fi

SAVELDFLAGS="$LDFLAGS"

LDFLAGS="-L../gdbm-1.8.0/.libs $SAVELDFLAGS"
AC_CHECK_LIB(tinygdbm, main,,AC_MSG_ERROR(TinyGDBM not found.))

TOOLDIR=`pwd`

AC_SUBST(TOOLDIR)

AC_OUTPUT(histogrammer report.sh Makefile, chmod +x histogrammer report.sh)
