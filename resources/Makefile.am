# This Makefile.am is in the public domain

css.c ui.c: %.c: %.gresource.xml
	glib-compile-resources --sourcedir=$(top_srcdir)/resources $< --generate-source
	glib-compile-resources --sourcedir=$(top_srcdir)/resources $< --generate-header

clean-local:
	rm css.c css.h
	rm ui.c ui.h

AM_CFLAGS = \
  -I$(top_srcdir)/resources \
  @GLIB_CFLAGS@

noinst_LIBRARIES = libresources.a

libresources_a_SOURCES = \
  css.c css.h \
  ui.c ui.h
