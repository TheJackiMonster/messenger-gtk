# This Makefile.am is in the public domain

SUBDIRS = \
  chat \
  ui

AM_CFLAGS = \
  -I$(top_srcdir)/src \
  -I$(top_srcdir)/resources \
  @GLIB_CFLAGS@ \
  @GTK_CFLAGS@ \
  @GNUNET_CFLAGS@ \
  @HANDY_CFLAGS@ \
  @GST_CFLAGS@ \
  @NOTIFY_CFLAGS@ \
  @QRENCODE_CFLAGS@

bin_PROGRAMS = messenger-gtk

messenger_gtk_SOURCES = \
  application.c application.h \
  contact.c contact.h \
  event.c event.h \
  file.c file.h \
  resources.c resources.h \
  ui.c ui.h \
  util.h \
  messenger_gtk.c

messenger_gtk_LDADD = \
  ../resources/libresources.a \
  chat/libchat.a \
  ui/libui.a \
  -lgnunetchat \
  -lgnunetutil \
  @GLIB_LIBS@ \
  @GTK_LIBS@ \
  @HANDY_LIBS@ \
  @GST_LIBS@ \
  @NOTIFY_LIBS@ \
  @QRENCODE_LIBS@

messenger_gtk_CFLAGS = \
  -pedantic -Wall -Wextra -Wno-overlength-strings

if DEBUG
messenger_gtk_CFLAGS += \
  -O0 -D _DEBUG -ggdb3
else
messenger_gtk_CFLAGS += \
  -O2 -D NDEBUG
endif
