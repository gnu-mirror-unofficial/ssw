AUTOMAKE_OPTIONS = 1.10.1 subdir-objects
# std-options 
# check-news

ACLOCAL_AMFLAGS = -I aclocal-aux ${ACLOCAL_FLAGS}

demo_demo_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS)  -I ${top_srcdir}/src

lib_LTLIBRARIES = libspread-sheet-widget.la
libspread_sheet_widget_la_CFLAGS = $(GTK3_CFLAGS) $(GLIB2_CFLAGS) -I ${top_builddir}/src
libspread_sheet_widget_la_SOURCES = \
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
	src/ssw-virtual-model.h \
	src/ssw-cell.h \
	src/ssw-xpaned.h


include_HEADERS = \
	src/ssw-sheet.h \
	src/ssw-sheet-axis.h \
	src/ssw-axis-model.h


noinst_PROGRAMS = demo/demo
demo_demo_LDADD = $(GTK3_LIBS) $(GLIB2_LIBS) -lm libspread-sheet-widget.la
demo_demo_SOURCES = demo/main.c demo/custom-axis.c demo/custom-axis.h

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

CLEANFILES = spread-sheet-widget.pc

install-exec-local: spread-sheet-widget.pc
	$(MKDIR_P) $(DESTDIR)$(libdir)
	chmod u+w $(DESTDIR)$(libdir)
	$(INSTALL) -D $< $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

uninstall-local:
	$(RM) $(DESTDIR)$(libdir)/pkgconfig/spread-sheet-widget.pc

EXTRA_DIST=TODO src/marshall-list spread-sheet-widget.pc.in NEWS

DISTCLEANFILES=$(BUILT_SOURCES)

dist-hook:
	if test -d $(top_srcdir)/.git; then                     \
		git -C $(top_srcdir)/.git log --date=short --pretty='%cd %an <%ae>%n%n        %s%n%n%n' > $(distdir)/cl-t || exit 1;                                        \
		rm -f $(distdir)/ChangeLog;                           \
		mv $(distdir)/cl-t $(distdir)/ChangeLog;              \
        fi

