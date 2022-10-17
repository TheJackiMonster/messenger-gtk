# This Makefile.am is in the public domain

AM_CFLAGS = \
  -I$(top_srcdir)/src \
  -I$(top_srcdir)/submodules/gnome-characters/lib \
  @GTK_CFLAGS@ \
  @GNUNET_CFLAGS@ \
  @HANDY_CFLAGS@ \
  @GST_CFLAGS@ \
  @GST_APP_CFLAGS@ \
  @NOTIFY_CFLAGS@ \
  @QRENCODE_CFLAGS@

noinst_LIBRARIES = libui.a

libui_a_SOURCES = \
  about.c about.h \
  account_entry.c account_entry.h \
  accounts.c accounts.h \
  chat.c chat.h \
  chat_entry.c chat_entry.h \
  contact_entry.c contact_entry.h \
  contact_info.c contact_info.h \
  contacts.c contacts.h \
  delete_messages.c delete_messages.h \
  file_load_entry.c file_load_entry.h \
  invite_contact.c invite_contact.h \
  message.c message.h \
  messenger.c messenger.h \
  new_account.c new_account.h \
  new_contact.c new_contact.h \
  new_group.c new_group.h \
  new_lobby.c new_lobby.h \
  new_platform.c new_platform.h \
  picker.c picker.h \
  send_file.c send_file.h \
  settings.c settings.h

