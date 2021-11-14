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

static void
_add_new_chat_entry(MESSENGER_Application *app,
		    struct GNUNET_CHAT_Context *context)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new(app);
  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);
  GNUNET_CHAT_context_set_user_pointer(context, entry);

  char context_id [9];
  g_snprintf(context_id, sizeof(context_id), "%08lx", (gulong) context);

  gtk_widget_set_name(entry->entry_box, context_id);

  gtk_stack_add_named(
      ui->chats_stack,
      entry->chat->chat_box,
      context_id
  );

  g_hash_table_insert(
      app->ui.bindings,
      entry->chat->send_text_view,
      context
  );

  ui->chat_entries = g_list_append(ui->chat_entries, entry);
}

static int
_iterate_profile_contacts(void *cls,
			  UNUSED struct GNUNET_CHAT_Handle *handle,
			  UNUSED struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  _add_new_chat_entry(app, GNUNET_CHAT_contact_get_context(contact));
  return GNUNET_YES;
}

static int
_iterate_profile_groups(void *cls,
			UNUSED struct GNUNET_CHAT_Handle *handle,
			UNUSED struct GNUNET_CHAT_Group *group)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  _add_new_chat_entry(app, GNUNET_CHAT_group_get_context(group));
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
event_update_profile(MESSENGER_Application *app,
		     UNUSED int argc,
		     UNUSED void **argv)
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
}

void
event_update_chats(MESSENGER_Application *app,
		   int argc,
		   void **argv)
{
  if (argc < 1)
    return;

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) argv[0];

  if (GNUNET_CHAT_context_get_user_pointer(context))
    return;

  _add_new_chat_entry(app, context);
}

void
event_receive_message(MESSENGER_Application *app,
		      int argc,
		      void **argv)
{
  if (argc < 2)
    return;

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) argv[0];

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  const struct GNUNET_CHAT_Message *msg;
  msg = *((const struct GNUNET_CHAT_Message**) argv[1]);

  const int sent = GNUNET_CHAT_message_is_sent(msg);

  UI_MESSAGE_Handle *message = ui_message_new(app, GNUNET_YES == sent);

  if (GNUNET_YES != sent)
  {
    const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
	msg
    );

    const char *sender = GNUNET_CHAT_contact_get_name(contact);

    hdy_avatar_set_text(message->sender_avatar, sender? sender : "");
    gtk_label_set_text(message->sender_label, sender? sender : "");
  }

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
      msg
  );

  const char *text = GNUNET_CHAT_message_get_text(msg);
  const char *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  gtk_label_set_text(message->text_label, text? text : "");
  gtk_label_set_text(message->timestamp_label, time? time : "");

  if (message->read_receipt_image)
    // TODO: check read receipt

  gtk_container_add(
      GTK_CONTAINER(handle->chat->messages_listbox),
      message->message_box
  );

  g_free(message); // TODO: this is just a test!

  gtk_label_set_text(handle->text_label, text? text : "");
  gtk_label_set_text(handle->timestamp_label, time? time : "");
}
