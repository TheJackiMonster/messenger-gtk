# This Makefile.am is in the public domain

SUBDIRS = icon

desktopdir = $(datadir)/applications
desktop_DATA = \
  org.gnunet.Messenger.desktop

appdatadir = $(datadir)/appdata
appdata_DATA = \
  org.gnunet.Messenger.appdata.xml

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

EXTRA_DIST = \
  org.gnunet.Messenger.desktop \
  org.gnunet.Messenger.appdata.xml
