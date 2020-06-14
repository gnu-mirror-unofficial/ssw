dnl A widget to display and manipulate tabular data
dnl Copyright (C) 2019, 2020  John Darrington

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([spread-sheet-widget], [0.4])

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
