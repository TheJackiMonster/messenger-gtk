/*
   This file is part of GNUnet.
   Copyright (C) 2021 GNUnet e.V.

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

#include "chat_entry.h"
#include "contact_entry.h"
#include "../application.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
			  gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean
_open_new_contact_dialog(gpointer user_data)
{
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
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
  {
    g_idle_add(G_SOURCE_FUNC(_open_new_contact_dialog), app);
    goto close_dialog;
  }

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_hash_table_lookup(app->ui.bindings, row)
  );

  if ((!contact) || (!GNUNET_CHAT_contact_get_key(contact)) ||
      (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact)))
    goto close_dialog;

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (!context)
    goto close_dialog;

  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
    GNUNET_CHAT_context_request(context);

close_dialog:
  gtk_window_close(GTK_WINDOW(app->ui.contacts.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_contacts_dialog_cleanup((UI_CONTACTS_Handle*) user_data);
}

static int
_iterate_contacts(void *cls,
		  UNUSED struct GNUNET_CHAT_Handle *handle,
		  struct GNUNET_CHAT_Contact *contact)
{
  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  const char *title;
  title = GNUNET_CHAT_contact_get_name(contact);

  const char *key = GNUNET_CHAT_contact_get_key(contact);

  UI_CONTACT_ENTRY_Handle *entry = ui_contact_entry_new();
  gtk_list_box_prepend(
      app->ui.contacts.contacts_listbox,
      entry->entry_box
  );

  if (title)
  {
    gtk_label_set_text(entry->title_label, title);
    hdy_avatar_set_text(entry->entry_avatar, title);
  }

  if (key)
    gtk_label_set_text(entry->subtitle_label, key);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  g_hash_table_insert(app->ui.bindings, row, contact);

  app->ui.contacts.contact_entries = g_list_append(
      app->ui.contacts.contact_entries,
      entry
  );

  return GNUNET_YES;
}

void
ui_contacts_dialog_init(MESSENGER_Application *app,
			UI_CONTACTS_Handle *handle)
{
  handle->contact_entries = NULL;

  handle->builder = gtk_builder_new_from_file("resources/ui/contacts.ui");

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "contacts_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      _("Contacts")
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
}

void
ui_contacts_dialog_cleanup(UI_CONTACTS_Handle *handle)
{
  g_object_unref(handle->builder);

  GList *list = handle->contact_entries;

  while (list) {
    if (list->data)
      ui_contact_entry_delete((UI_CONTACT_ENTRY_Handle*) list->data);

    list = list->next;
  }

  if (handle->contact_entries)
    g_list_free(handle->contact_entries);
}
