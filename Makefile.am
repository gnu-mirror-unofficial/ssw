AUTOMAKE_OPTIONS = 1.10.1 subdir-objects
# std-options 
# check-news

ACLOCAL_AMFLAGS = -I aclocal-aux ${ACLOCAL_FLAGS}

demo_demo_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS)  -I ${top_srcdir}/src

lib_LTLIBRARIES = libspread-sheet-widget.la
libspread_sheet_widget_la_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS) -I ${top_builddir}/src
libspread_sheet_widget_la_SOURCES = src/ssw-axis-model.c \
	src/ssw-sheet-axis.c \
	src/ssw-sheet-body.c \
	src/ssw-sheet-single.c \
	src/ssw-sheet.c \
	src/ssw-datum.c \
	src/ssw-constraint.c \
	src/ssw-virtual-model.c \
	src/ssw-cell.c


# FIXME: Most of the following need not be installed?
# Move to libspread-sheet-widget_la_SOURCES
include_HEADERS = config.h \
	src/ssw-axis-model.h \
	src/ssw-sheet-axis.h \
	src/ssw-sheet-body.h \
	src/ssw-sheet-single.h \
	src/ssw-sheet.h \
	src/ssw-datum.h \
	src/ssw-constraint.h \
	src/ssw-virtual-model.h \
	src/ssw-cell.h

noinst_PROGRAMS = demo/demo
demo_demo_LDADD = $(GTK3_LIBS) $(GLIB2_LIBS) -lm -lspread-sheet-widget
demo_demo_SOURCES = demo/main.c

BUILT_SOURCES = \
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

spread-sheet-widget.pc: spread-sheet-widget.pc.in configure.ac
	$(SED) -e 's|%version%|$(PACKAGE_VERSION)|' \
	 -e 's|%prefix%|$(prefix)|' \
	 -e 's|%glib_dependency%|'"$(glib_dependency)"'|' \
	 -e 's|%gtk_dependency%|'"$(gtk_dependency)"'|' \
	 $< > $@,tmp
	mv $@,tmp $@

#dist_data_DATA = spread-sheet-widget.pc
#pkglib_LIBRARIES = spread-sheet-widget.pc

CLEANFILES = spread-sheet-widget.pc

install-exec-local: spread-sheet-widget.pc
	chmod u+w $(DESTDIR)$(libdir)
	$(INSTALL) -D $< $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

uninstall-local:
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

EXTRA_DIST=TODO src/marshall-list spread-sheet-widget.pc.in NEWS

DISTCLEANFILES=$(BUILT_SOURCES)
