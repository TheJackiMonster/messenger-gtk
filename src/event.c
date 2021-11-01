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
 * @file event.c
 */

#include "event.h"

#include "ui/chat_entry.h"
#include "ui/message.h"

static int
_iterate_profile_contacts(void *cls,
			  UNUSED struct GNUNET_CHAT_Handle *handle,
			  UNUSED struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new();

  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);

  g_free(entry); //TODO: add to a list or similar?

  return GNUNET_YES;
}

static int
_iterate_profile_groups(void *cls,
			UNUSED struct GNUNET_CHAT_Handle *handle,
			UNUSED struct GNUNET_CHAT_Group *group)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new();

  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);

  g_free(entry); //TODO: add to a list or similar?

  return GNUNET_YES;
}

static void
_clear_each_widget(GtkWidget *widget,
		   gpointer user_data)
{
  GtkContainer *container = GTK_CONTAINER(user_data);

  gtk_container_remove(container, widget);
}

void
event_update_profile(MESSENGER_Application *app)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  const char *name = GNUNET_CHAT_get_name(chat->handle);

  if (name)
  {
    hdy_avatar_set_text(ui->profile_avatar, name);
    gtk_label_set_text(ui->profile_label, name);
  }

  gtk_container_foreach(
      GTK_CONTAINER(ui->chats_listbox),
      _clear_each_widget,
      ui->chats_listbox
  );

  GNUNET_CHAT_iterate_contacts(chat->handle, _iterate_profile_contacts, app);
  GNUNET_CHAT_iterate_groups(chat->handle, _iterate_profile_groups, app);

  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new();
  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);
  g_free(entry);

  for (int i = 0; i < 8; i++) {
    UI_MESSAGE_Handle *message = ui_message_new(app, i % 2 == 0);
    gtk_container_add(GTK_CONTAINER(ui->messages_listbox), message->message_box);
    g_free(message);
  }
}
