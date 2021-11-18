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

#include "contact_entry.h"
#include "../application.h"

#include <gnunet/gnunet_identity_service.h>

static void
handle_close_button_click(UNUSED GtkButton *button,
			  gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_contacts_dialog_cleanup((UI_CONTACTS_Handle*) user_data);
}

static int
_iterate_clear_contacts(UNUSED void *cls,
			UNUSED struct GNUNET_CHAT_Handle *handle,
			struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_contact_set_user_pointer(contact, NULL);
  return GNUNET_YES;
}

static int
_iterate_contacts(void *cls,
		  UNUSED struct GNUNET_CHAT_Handle *handle,
		  struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (GNUNET_CHAT_contact_get_user_pointer(contact))
    return GNUNET_YES;

  const char *title;
  title = GNUNET_CHAT_contact_get_name(contact);

  const struct GNUNET_IDENTITY_PublicKey *key;
  key = GNUNET_CHAT_contact_get_key(contact);

  UI_CONTACT_ENTRY_Handle *entry = ui_contact_entry_new();
  gtk_container_add(
      GTK_CONTAINER(app->ui.contacts.contacts_listbox),
      entry->entry_box
  );

  GNUNET_CHAT_contact_set_user_pointer(contact, entry);

  if (title)
  {
    gtk_label_set_text(entry->title_label, title);
    hdy_avatar_set_text(entry->entry_avatar, title);
  }

  if (key)
  {
    char *key_string = GNUNET_IDENTITY_public_key_to_string(key);
    gtk_label_set_text(entry->subtitle_label, key_string);
    GNUNET_free(key_string);
  }

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
  handle->contact_entries = g_list_alloc();

  handle->builder = gtk_builder_new_from_file("resources/ui/contacts.ui");

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "contacts_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      "Contacts"
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
      _iterate_clear_contacts,
      NULL
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

  g_list_free(handle->contact_entries);
}
