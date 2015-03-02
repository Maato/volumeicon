# Process this file with autoconf to produce a configure script
AC_INIT([volumeicon], [0.5.1], [http://www.softwarebakery.com/maato/volumeicon.html])

AM_INIT_AUTOMAKE

# Check for programs
AC_LANG([C])
AC_PROG_CC

# Check for functions and types
AC_CHECK_HEADERS([stdlib.h])
AC_C_CONST
AC_FUNC_MALLOC
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_FUNC_FORK

AC_PROG_INTLTOOL([0.23])

GETTEXT_PACKAGE=volumeicon
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

# Checks for command line options
AC_ARG_ENABLE([oss],
  [  --disable-oss    disable oss],
  [oss=${enableval}],
  [oss=no])
AC_ARG_ENABLE([notify],
  [  --disable-notify   disable notify],
  [notify=${enableval}],
  [notify=no])

# Check for gtk
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Check for X11
PKG_CHECK_MODULES([X11], [x11])
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

if test "x${oss}" = xno; then
# Check for alsa
PKG_CHECK_MODULES([ALSA], [alsa])
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)
OSS_CFLAGS=""
else
AC_ARG_WITH([oss-include-path],
  [AS_HELP_STRING(
    [--with-oss-include-path],
    [location of soundcard.h, defaults to value in /etc/oss.conf and then
        /usr/lib/oss/include/sys])],
  [OSS_CPPFLAGS="-I$withval"; OSS_HEADER="${withval}/soundcard.h"],
  [
    if test -f /etc/oss.conf
    then
      . /etc/oss.conf
      OSS_CPPFLAGS="-I${OSSLIBDIR}/include/sys"
      OSS_HEADER="${OSSLIBDIR}/include/sys/soundcard.h"
    else
      OSS_CPPFLAGS="-I/usr/lib/oss/include/sys"
      OSS_HEADER="/usr/lib/oss/include/sys/soundcard.h"
    fi
  ]
)
# Use AC_CHECK_HEADER with the directory determined above for soundcard.h. We
# specify a blank but non-empty fourth parameter to prevent AC_CHECK_HEADER
# from sticking AC_INCLUDES_DEFAULT in the test file, which may cause the test
# to fail.
OLD_CPPFLAGS=${CPPFLAGS}
CPPFLAGS="${OSS_CPPFLAGS}"
AC_CHECK_HEADER([soundcard.h], [], [AC_MSG_ERROR([could not find soundcard.h])], [])
CPPFLAGS=${OLD_CPPFLAGS}

# Pass the absolute path to the OSS header file
OSS_CFLAGS="-DCOMPILEWITH_OSS -DOSS_HEADER=\\\"${OSS_HEADER}\\\""
AC_SUBST([OSS_CFLAGS])
fi

if test "x${notify}" = xyes; then
# Check for libnotify
PKG_CHECK_MODULES([NOTIFY], [libnotify >= 0.5.0])
NOTIFY_CFLAGS+="-DCOMPILEWITH_NOTIFY"
AC_SUBST(NOTIFY_CFLAGS)
AC_SUBST(NOTIFY_LIBS)
fi

AC_SUBST(OSS_CFLAGS)

AM_CONDITIONAL(ENABLE_OSS, test "$oss" = "yes")

DEFAULT_MIXERAPP="xterm -e 'alsamixer'"
AC_ARG_WITH(default-mixerapp,
[  --with-default-mixerapp=appname
                           Default command for "Open Mixer" menu item
                           (default: "xterm -e 'alsamixer'")],
DEFAULT_MIXERAPP=$withval)
AC_DEFINE_UNQUOTED(DEFAULT_MIXERAPP,"${DEFAULT_MIXERAPP}",[Default mixer application])

AC_CONFIG_FILES(Makefile \
		src/Makefile \
		data/Makefile \
		po/Makefile.in)

AC_OUTPUT
