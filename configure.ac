AC_INIT([spread-sheet-widget], [0.3])

AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([aclocal-aux])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC_STDC
LT_INIT

m4_pattern_forbid([PKG_CHECK_MODULES])

gtk_dependency="gtk+-3.0 >= 3.18.0"
glib_dependency="glib-2.0 >= 2.44"

PKG_CHECK_MODULES([GTK3], [$gtk_dependency])
PKG_CHECK_MODULES([GLIB2], [$glib_dependency])

AC_CONFIG_FILES([Makefile spread-sheet-widget.pc])

AC_SUBST([gtk_dependency])
AC_SUBST([glib_dependency])

AC_OUTPUT
