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
 * @file ui/new_group.c
 */

#include "new_platform.h"

#include "contact_entry.h"
#include "../application.h"

static void
_open_new_group(GtkEntry *entry,
		GtkListBox *listbox,
		MESSENGER_Application *app)
{
  const gchar *name = gtk_entry_get_text(entry);

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_group_create(
      app->chat.messenger.handle,
      NULL
  );

  if ((name) && (strlen(name) > 0))
    GNUNET_CHAT_group_set_name(group, name);

  GList *selected = gtk_list_box_get_selected_rows(listbox);

  while (selected)
  {
    if (selected->data)
    {
      GtkListBoxRow *row = GTK_LIST_BOX_ROW(selected->data);

      struct GNUNET_CHAT_Contact* contact = g_hash_table_lookup(
	  app->ui.bindings, row
      );

      GNUNET_CHAT_group_invite_contact(group, contact);
    }

    selected = selected->next;
  }
}

static void
handle_group_entry_changed(GtkEditable *editable,
			   gpointer user_data)
{
  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  hdy_avatar_set_text(avatar, gtk_entry_get_text(entry));
}

static void
_go_page_details(UI_NEW_GROUP_Handle *handle)
{
  gtk_stack_set_visible_child(handle->stack, handle->details_box);

  gtk_widget_hide(GTK_WIDGET(handle->previous_button));
  gtk_widget_hide(GTK_WIDGET(handle->confirm_button));

  gtk_widget_show(GTK_WIDGET(handle->cancel_button));
  gtk_widget_show(GTK_WIDGET(handle->next_button));
}

static void
_go_page_contacts(UI_NEW_GROUP_Handle *handle)
{
  gtk_stack_set_visible_child(handle->stack, handle->contacts_box);

  gtk_widget_hide(GTK_WIDGET(handle->cancel_button));
  gtk_widget_hide(GTK_WIDGET(handle->next_button));

  gtk_widget_show(GTK_WIDGET(handle->previous_button));
  gtk_widget_show(GTK_WIDGET(handle->confirm_button));
}

static void
handle_group_entry_activate(UNUSED GtkEntry *entry,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _go_page_contacts(&(app->ui.new_group));
}

static void
handle_cancel_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_previous_button_click(UNUSED GtkButton *button,
			 gpointer user_data)
{
  _go_page_details((UI_NEW_GROUP_Handle*) user_data);
}

static void
handle_next_button_click(UNUSED GtkButton *button,
			 gpointer user_data)
{
  _go_page_contacts((UI_NEW_GROUP_Handle*) user_data);
}

static void
handle_confirm_button_click(UNUSED GtkButton *button,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_group(
      app->ui.new_group.group_entry,
      app->ui.new_group.contacts_listbox,
      app
  );

  gtk_window_close(GTK_WINDOW(app->ui.new_group.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_new_group_dialog_cleanup((UI_NEW_GROUP_Handle*) user_data);
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

  const char *key = GNUNET_CHAT_contact_get_key(contact);

  UI_CONTACT_ENTRY_Handle *entry = ui_contact_entry_new();
  gtk_list_box_prepend(
      app->ui.new_group.contacts_listbox,
      entry->entry_box
  );

  GNUNET_CHAT_contact_set_user_pointer(contact, entry);

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

  app->ui.new_group.contact_entries = g_list_append(
      app->ui.new_group.contact_entries,
      entry
  );

  return GNUNET_YES;
}

void
ui_new_group_dialog_init(MESSENGER_Application *app,
			 UI_NEW_GROUP_Handle *handle)
{
  handle->contact_entries = NULL;

  handle->builder = gtk_builder_new_from_file("resources/ui/new_group.ui");

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "new_group_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      "New Group"
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "new_group_stack")
  );

  handle->details_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "details_box")
  );

  handle->contacts_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "contacts_box")
  );

  handle->group_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "group_avatar")
  );

  handle->group_avatar_file = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(handle->builder, "group_avatar_file")
  );

  handle->group_entry = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "group_entry")
  );

  g_signal_connect(
      handle->group_entry,
      "changed",
      G_CALLBACK(handle_group_entry_changed),
      handle->group_avatar
  );

  g_signal_connect(
      handle->group_entry,
      "activate",
      G_CALLBACK(handle_group_entry_activate),
      app
  );

  handle->contact_search_entry = GTK_SEARCH_ENTRY(
      gtk_builder_get_object(handle->builder, "contact_search_entry")
  );

  handle->contacts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "contacts_listbox")
  );

  handle->cancel_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
      handle->cancel_button,
      "clicked",
      G_CALLBACK(handle_cancel_button_click),
      handle->dialog
  );

  handle->previous_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "previous_button")
  );

  g_signal_connect(
      handle->previous_button,
      "clicked",
      G_CALLBACK(handle_previous_button_click),
      handle
  );

  handle->next_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "next_button")
  );

  g_signal_connect(
      handle->next_button,
      "clicked",
      G_CALLBACK(handle_next_button_click),
      handle
  );

  handle->confirm_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "confirm_button")
  );

  g_signal_connect(
      handle->confirm_button,
      "clicked",
      G_CALLBACK(handle_confirm_button_click),
      app
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
ui_new_group_dialog_cleanup(UI_NEW_GROUP_Handle *handle)
{
  g_object_unref(handle->builder);

  for (GList *list = handle->contact_entries; list; list = list->next)
    ui_contact_entry_delete((UI_CONTACT_ENTRY_Handle*) list->data);

  if (handle->contact_entries)
    g_list_free(handle->contact_entries);
}