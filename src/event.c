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

#include "contact.h"

#include "ui/chat_entry.h"
#include "ui/contact_entry.h"
#include "ui/message.h"
#include "ui/profile_entry.h"

static void
_add_new_chat_entry(MESSENGER_Application *app,
		    struct GNUNET_CHAT_Context *context)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new(app);

  ui_chat_entry_update(entry, app, context);

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

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  gtk_list_box_select_row(ui->chats_listbox, row);
  gtk_widget_activate(GTK_WIDGET(row));
}

static int
_iterate_profile_contacts(void *cls,
			  UNUSED struct GNUNET_CHAT_Handle *handle,
			  struct GNUNET_CHAT_Contact *contact)
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
event_update_profile(MESSENGER_Application *app)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  const char *name = GNUNET_CHAT_get_name(chat->handle);

  if (name)
  {
    hdy_avatar_set_text(ui->profile_avatar, name);
    gtk_label_set_text(ui->profile_label, name);

    UI_PROFILE_ENTRY_Handle *profile = ui_profile_entry_new();

    hdy_avatar_set_text(profile->entry_avatar, name);
    gtk_label_set_text(profile->entry_label, name);

    gtk_list_box_prepend(ui->accounts_listbox, profile->entry_box);

    GtkListBoxRow *row = GTK_LIST_BOX_ROW(
	gtk_widget_get_parent(profile->entry_box)
    );

    gtk_list_box_select_row(ui->accounts_listbox, row);

    ui_profile_entry_delete(profile);
  }

  const char *key = GNUNET_CHAT_get_key(chat->handle);

  if (key)
    gtk_label_set_text(ui->profile_key_label, key);

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
		   struct GNUNET_CHAT_Context *context,
		   const struct GNUNET_CHAT_Message *msg)
{
  if (GNUNET_CHAT_context_get_user_pointer(context))
    return;

  contact_create_info(GNUNET_CHAT_message_get_sender(msg));

  _add_new_chat_entry(app, context);
}

static void
_update_contact_context(MESSENGER_Application *app,
			struct GNUNET_CHAT_Contact *contact)
{
  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (!context)
    return;

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  ui_chat_entry_update(handle, app, context);
}

void
event_joining_contact(MESSENGER_Application *app,
		      struct GNUNET_CHAT_Context *context,
		      const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  ui_chat_entry_update(handle, app, context);

  UI_MESSAGE_Handle *message = ui_message_new(UI_MESSAGE_STATUS);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  contact_create_info(contact);
  _update_contact_context(app, contact);

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  gtk_label_set_text(message->text_label, "joined the chat");

  gtk_container_add(
      GTK_CONTAINER(handle->chat->messages_listbox),
      message->message_box
  );

  ui_message_delete(message);
}

void
event_update_contacts(MESSENGER_Application *app,
		      struct GNUNET_CHAT_Context *context,
		      const struct GNUNET_CHAT_Message *msg)
{
  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  contact_update_info(contact);
  _update_contact_context(app, contact);

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  ui_chat_entry_update(handle, app, context);
}

static void
_event_invitation_accept_click(UNUSED GtkButton *button,
			       gpointer user_data)
{
  struct GNUNET_CHAT_Invitation *invitation = (
      (struct GNUNET_CHAT_Invitation*) user_data
  );

  GNUNET_CHAT_invitation_accept(invitation);
}

void
event_invitation(UNUSED MESSENGER_Application *app,
		 struct GNUNET_CHAT_Context *context,
		 const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  struct GNUNET_CHAT_Invitation *invitation;
  invitation = GNUNET_CHAT_message_get_invitation(msg);

  if (!invitation)
    return;

  UI_MESSAGE_Handle *message = ui_message_new(UI_MESSAGE_STATUS);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  gtk_label_set_text(message->text_label, "invited you to a chat");

  g_signal_connect(
      message->accept_button,
      "clicked",
      G_CALLBACK(_event_invitation_accept_click),
      invitation
  );

  gtk_widget_show(GTK_WIDGET(message->deny_button));
  gtk_widget_show(GTK_WIDGET(message->accept_button));

  gtk_container_add(
      GTK_CONTAINER(handle->chat->messages_listbox),
      message->message_box
  );

  ui_message_delete(message);
}

void
event_receive_message(UNUSED MESSENGER_Application *app,
		      struct GNUNET_CHAT_Context *context,
		      const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  const int sent = GNUNET_CHAT_message_is_sent(msg);

  UI_MESSAGE_Handle *message = ui_message_new(
      GNUNET_YES == sent? UI_MESSAGE_SENT : UI_MESSAGE_DEFAULT
  );

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

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

  ui_message_delete(message);

  gtk_label_set_text(handle->text_label, text? text : "");
  gtk_label_set_text(handle->timestamp_label, time? time : "");

  gtk_widget_set_visible(
      GTK_WIDGET(handle->read_receipt_image),
      GNUNET_YES == sent
  );
}
