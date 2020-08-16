# A widget to display and manipulate tabular data
# Copyright (C) 2019, 2020  John Darrington

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = 1.10.1 subdir-objects check-news std-options

EXTRA_DIST=
BUILT_SOURCES=

AM_MAKEINFOFLAGS=-I ${top_builddir}/doc
TEXI2DVI=texi2dvi -I ${top_builddir}/doc
info_TEXINFOS = doc/spread-sheet-widget.texi

.c.c-enum:
	cat -n $< > $@

noinst_PROGRAMS = doc/prog1 doc/prog2

doc_prog1_SOURCES = doc/prog1.c
doc_prog1_LDADD = libspread-sheet-widget.la $(GTK3_LIBS) $(GLIB2_LIBS) -lm
doc_prog1_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS)  -I ${top_srcdir}/src


doc_prog2_SOURCES = doc/prog2.c
doc_prog2_LDADD = libspread-sheet-widget.la $(GTK3_LIBS) $(GLIB2_LIBS) -lm
doc_prog2_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS)  -I ${top_srcdir}/src

ACLOCAL_AMFLAGS = -I aclocal-aux ${ACLOCAL_FLAGS}


lib_LTLIBRARIES = libspread-sheet-widget.la
libspread_sheet_widget_la_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS) -I ${top_builddir}/src
libspread_sheet_widget_la_LIBADD = $(GTK3_LIBS) $(GLIB2_LIBS)
libspread_sheet_widget_la_SOURCES = \
	src/ssw-html-parser.c \
	src/ssw-paste.h \
	src/ssw-axis-model.c \
	src/ssw-sheet-axis.c \
	src/ssw-sheet-body.c \
	src/ssw-sheet-single.c \
	src/ssw-sheet.c \
	src/ssw-constraint.c \
	src/ssw-virtual-model.c \
	src/ssw-cell.c \
	src/ssw-xpaned.c \
	src/ssw-sheet-body.h \
	src/ssw-sheet-single.h \
	src/ssw-constraint.h \
	src/ssw-cell.h \
	src/ssw-xpaned.h


include_HEADERS = \
	src/ssw-sheet.h \
	src/ssw-sheet-axis.h \
	src/ssw-virtual-model.h \
	src/ssw-axis-model.h


noinst_PROGRAMS += demo/demo
demo_demo_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS)  -I ${top_srcdir}/src
demo_demo_LDADD = libspread-sheet-widget.la $(GTK3_LIBS) $(GLIB2_LIBS) -lm
demo_demo_SOURCES = demo/main.c demo/custom-axis.c demo/custom-axis.h

BUILT_SOURCES += \
	src/ssw-marshaller.c \
	src/ssw-marshaller.h

nodist_libspread_sheet_widget_la_SOURCES = \
	src/ssw-marshaller.c \
	src/ssw-marshaller.h

src/ssw-marshaller.c: src/marshall-list
	glib-genmarshal --body --prefix=ssw_cclosure_marshal $< > $@,tmp
	mv $@,tmp $@

src/ssw-marshaller.h: src/marshall-list
	glib-genmarshal --header --prefix=ssw_cclosure_marshal $< > $@,tmp
	mv $@,tmp $@

install-exec-local: spread-sheet-widget.pc
	$(MKDIR_P) $(DESTDIR)$(libdir)/pkgconfig
	chmod -R u+w $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 $< $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

uninstall-local:
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

EXTRA_DIST+=TODO src/marshall-list spread-sheet-widget.pc.in NEWS

DISTCLEANFILES=$(BUILT_SOURCES)

dist-hook:
	if test -d $(top_srcdir)/.git; then                     \
		git -C $(top_srcdir)/.git log --date=short --pretty='%cd %an <%ae>%n%n        %s%n%n' > $(distdir)/cl-t || exit 1;                                        \
		cp $(distdir)/cl-t $(distdir)/ChangeLog;          \
		cat $(top_srcdir)/ChangeLog >> $(distdir)/ChangeLog; \
        fi

DISTCLEANFILES += $(built_doc_files)
built_doc_files = ${top_builddir}/doc/prog1.c-enum ${top_builddir}/doc/prog2.c-enum
doc_spread_sheet_widget_TEXINFOS = ${built_doc_files}

