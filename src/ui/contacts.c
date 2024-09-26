/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author Tobias Frisch
 * @file ui/contacts.c
 */

#include "contacts.h"

#include "contact_entry.h"
#include "../application.h"
#include "../ui.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean
_open_new_contact_dialog(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_contact_dialog_init(app, &(app->ui.new_contact));

  gtk_widget_show(GTK_WIDGET(app->ui.new_contact.dialog));
  return FALSE;
}

static void
handle_contacts_listbox_row_activated(UNUSED GtkListBox* listbox,
                                      GtkListBoxRow* row,
                                      gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
  {
    util_idle_add(G_SOURCE_FUNC(_open_new_contact_dialog), app);
    goto close_dialog;
  }

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  if (!contact)
    goto close_dialog;

  application_chat_lock(app);
  const gboolean closing = (
    (!GNUNET_CHAT_contact_get_key(contact)) ||
    (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
  );
  application_chat_unlock(app);

  if (closing)
    goto close_dialog;

  application_chat_lock(app);
  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );
  application_chat_unlock(app);

  if (!context)
    goto close_dialog;

  application_chat_lock(app);
  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
    GNUNET_CHAT_context_request(context);
  application_chat_unlock(app);

close_dialog:
  gtk_window_close(GTK_WINDOW(app->ui.contacts.dialog));
}

struct FilterTags
{
  const gchar *filter;
  gboolean matching;
};

static enum GNUNET_GenericReturnValue
_iterate_contact_tags(void *cls,
                      struct GNUNET_CHAT_Contact *contact,
                      const char *tag)
{
  g_assert((cls) && (contact) && (tag));

  struct FilterTags *filterTags = (struct FilterTags*) cls;

  gchar *_tag = g_locale_to_utf8(tag, -1, NULL, NULL, NULL);
  if (!_tag)
    return GNUNET_YES;

  if (g_strstr_len(_tag, -1, filterTags->filter))
    filterTags->matching = TRUE;

  g_free(_tag);
  return filterTags->matching? GNUNET_NO : GNUNET_YES;
}

static gboolean
handle_contacts_listbox_filter_func(GtkListBoxRow *row,
                                    gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
    return TRUE;

  const gchar *filter = gtk_entry_get_text(
    GTK_ENTRY(app->ui.contacts.contact_search_entry)
  );

  if (!filter)
    return TRUE;

  UI_CONTACT_ENTRY_Handle *entry = (UI_CONTACT_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  if (!entry)
    return FALSE;

  const gchar *name = gtk_label_get_text(entry->title_label);

  gboolean result = FALSE;

  if (name)
    result |= g_str_match_string(filter, name, TRUE);

  if (('#' == *filter) && (entry->contact))
  {
    struct FilterTags filterTags;
    filterTags.filter = &(filter[1]);
    filterTags.matching = FALSE;

    application_chat_lock(app);

    GNUNET_CHAT_contact_get_tags(
      entry->contact,
      _iterate_contact_tags,
      &filterTags
    );

    application_chat_unlock(app);

    result |= filterTags.matching;
  }

  return result;
}

static void
handle_contact_search_entry_search_changed(UNUSED GtkSearchEntry* search_entry,
                                           gpointer user_data)
{
  g_assert(user_data);

  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_invalidate_filter(listbox);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_contacts_dialog_cleanup((UI_CONTACTS_Handle*) user_data);
}

static enum GNUNET_GenericReturnValue
_iterate_contacts(void *cls,
                  UNUSED struct GNUNET_CHAT_Handle *handle,
                  struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  UI_CONTACT_ENTRY_Handle *entry = ui_contact_entry_new(app);
  ui_contact_entry_set_contact(entry, contact);

  gtk_list_box_prepend(
    app->ui.contacts.contacts_listbox,
    entry->entry_box
  );

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), app->quarks.data, contact);

  g_object_set_qdata_full(
    G_OBJECT(row),
    app->quarks.ui,
    entry,
    (GDestroyNotify) ui_contact_entry_delete
  );

  return GNUNET_YES;
}

void
ui_contacts_dialog_init(MESSENGER_Application *app,
                        UI_CONTACTS_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/contacts.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "contacts_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->contact_search_entry = GTK_SEARCH_ENTRY(
    gtk_builder_get_object(handle->builder, "contact_search_entry")
  );

  handle->contacts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "contacts_listbox")
  );

  gtk_list_box_set_filter_func(
    handle->contacts_listbox,
    handle_contacts_listbox_filter_func,
    app,
    NULL
  );

  g_signal_connect(
    handle->contact_search_entry,
    "search-changed",
    G_CALLBACK(handle_contact_search_entry_search_changed),
    handle->contacts_listbox
  );

  g_signal_connect(
    handle->contacts_listbox,
    "row-activated",
    G_CALLBACK(handle_contacts_listbox_row_activated),
    app
  );

  handle->close_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
    handle->close_button,
    "clicked",
    G_CALLBACK(handle_close_button_click),
    handle->dialog
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );

  GNUNET_CHAT_iterate_contacts(
    app->chat.messenger.handle,
    _iterate_contacts,
    app
  );

  gtk_list_box_invalidate_filter(handle->contacts_listbox);
}

void
ui_contacts_dialog_cleanup(UI_CONTACTS_Handle *handle)
{
  g_assert(handle);

  if (handle->builder)
    g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
