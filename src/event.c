/*
   This file is part of GNUnet.
   Copyright (C) 2021--2022 GNUnet e.V.

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
#include "file.h"
#include "ui/account_entry.h"

#include "ui/chat_entry.h"
#include "ui/contact_entry.h"
#include "ui/message.h"

static void
_close_notification(NotifyNotification* notification,
		    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->notifications = g_list_remove(app->notifications, notification);

  notify_notification_clear_actions(notification);
  notify_notification_clear_hints(notification);

  g_object_unref(notification);
}

static void
_show_notification(MESSENGER_Application *app,
		   UNUSED struct GNUNET_CHAT_Context *context,
		   const struct GNUNET_CHAT_Contact *contact,
		   const gchar *text,
		   const gchar *icon)
{
  const char *sender = GNUNET_CHAT_contact_get_name(contact);

  NotifyNotification *notification = notify_notification_new(
      sender? sender : "(unknown)", text, icon
  );

  if (!notification)
    return;

  app->notifications = g_list_append(app->notifications, notification);

  if (0 == g_strcmp0(icon, "avatar-default-symbolic"))
    notify_notification_set_category(notification, "presence.online");
  else
    notify_notification_set_category(notification, "im.received");

  g_signal_connect(
      notification,
      "closed",
      G_CALLBACK(_close_notification),
      app
  );

  notify_notification_show(notification, NULL);
}

static void
_clear_each_selectable_widget(GtkWidget *widget,
			      gpointer user_data)
{
  GtkContainer *container = GTK_CONTAINER(user_data);
  GtkListBoxRow *row = GTK_LIST_BOX_ROW(widget);

  if (gtk_list_box_row_get_selectable(row))
    gtk_container_remove(container, widget);
}

static int
_iterate_accounts(void *cls,
		  const struct GNUNET_CHAT_Handle *handle,
		  struct GNUNET_CHAT_Account *account)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  const gchar *name = GNUNET_CHAT_account_get_name(account);

  UI_ACCOUNT_ENTRY_Handle *entry = ui_account_entry_new(app);

  hdy_avatar_set_text(entry->entry_avatar, name);
  gtk_label_set_text(entry->entry_label, name);

  gtk_list_box_prepend(ui->accounts_listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_hash_table_insert(ui->bindings, row, account);

  if ((account == GNUNET_CHAT_get_connected(handle)) ||
      ((app->chat.identity) && (0 == g_strcmp0(app->chat.identity, name))))
    gtk_widget_activate(GTK_WIDGET(row));

  ui_account_entry_delete(entry);
  return GNUNET_YES;
}

void
event_refresh_accounts(MESSENGER_Application *app)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  if (!(ui->accounts_listbox))
    return;

  gtk_list_box_unselect_all(ui->accounts_listbox);

  gtk_container_foreach(
      GTK_CONTAINER(ui->accounts_listbox),
      _clear_each_selectable_widget,
      ui->accounts_listbox
  );

  GNUNET_CHAT_iterate_accounts(chat->handle, _iterate_accounts, app);
}

static void
_add_new_chat_entry(MESSENGER_Application *app,
		    struct GNUNET_CHAT_Context *context)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new(app);

  ui_chat_entry_update(entry, app, context);

  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);
  GNUNET_CHAT_context_set_user_pointer(context, entry);

  gtk_container_add(
      GTK_CONTAINER(ui->chats_stack),
      entry->chat->chat_box
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

  g_hash_table_insert(
      app->ui.bindings,
      row,
      entry
  );

  gtk_list_box_select_row(ui->chats_listbox, row);
  gtk_list_box_invalidate_filter(ui->chats_listbox);

  gtk_widget_activate(GTK_WIDGET(row));
}

static int
_iterate_profile_contacts(void *cls,
			  UNUSED struct GNUNET_CHAT_Handle *handle,
			  struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
    return GNUNET_YES;

  _add_new_chat_entry(app, context);
  return GNUNET_YES;
}

static int
_iterate_profile_groups(void *cls,
			UNUSED struct GNUNET_CHAT_Handle *handle,
			UNUSED struct GNUNET_CHAT_Group *group)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_group_get_context(
      group
  );

  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
    return GNUNET_YES;

  _add_new_chat_entry(app, GNUNET_CHAT_group_get_context(group));
  return GNUNET_YES;
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

  const char *key = GNUNET_CHAT_get_key(chat->handle);

  if (key)
    gtk_label_set_text(ui->profile_key_label, key);

  gtk_container_foreach(
      GTK_CONTAINER(ui->chats_listbox),
      _clear_each_selectable_widget,
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
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    _add_new_chat_entry(app, context);
  else
    ui_chat_entry_update(handle, app, context);

  contact_create_info(GNUNET_CHAT_message_get_sender(msg));
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

  UI_MESSAGE_Handle *message = ui_message_new(app, UI_MESSAGE_STATUS);
  ui_message_update(message, app, msg);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  contact_create_info(contact);
  _update_contact_context(app, contact);

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  const gchar *join_message = _("joined the chat");

  if (!ui_messenger_is_context_active(&(app->ui.messenger), context))
    _show_notification(
	app,
	context,
	contact,
	join_message,
	"avatar-default-symbolic"
    );

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
      msg
  );

  const gchar *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  gtk_label_set_text(message->text_label, join_message);
  gtk_label_set_text(message->timestamp_label, time? time : "");

  ui_chat_add_message(handle->chat, app, message);
  ui_chat_entry_update(handle, app, context);
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
event_invitation(MESSENGER_Application *app,
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

  UI_MESSAGE_Handle *message = ui_message_new(app, UI_MESSAGE_STATUS);
  ui_message_update(message, app, msg);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  const gchar *invite_message = _("invited you to a chat");

  if (!ui_messenger_is_context_active(&(app->ui.messenger), context))
    _show_notification(
	app,
	context,
	contact,
	invite_message,
	"mail-message-new-symbolic"
    );

  gtk_label_set_text(message->text_label, invite_message);

  g_signal_connect(
      message->accept_button,
      "clicked",
      G_CALLBACK(_event_invitation_accept_click),
      invitation
  );

  gtk_widget_show(GTK_WIDGET(message->deny_button));
  gtk_widget_show(GTK_WIDGET(message->accept_button));

  ui_chat_add_message(handle->chat, app, message);
  ui_chat_entry_update(handle, app, context);
}

void
event_receive_message(MESSENGER_Application *app,
		      struct GNUNET_CHAT_Context *context,
		      const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  const int sent = GNUNET_CHAT_message_is_sent(msg);

  const UI_MESSAGE_Type type = (
      GNUNET_YES == sent? UI_MESSAGE_SENT : UI_MESSAGE_DEFAULT
  );

  UI_MESSAGE_Handle *message = ui_message_new(app, type);

  struct GNUNET_CHAT_File *file = GNUNET_CHAT_message_get_file(msg);

  if (file)
  {
    file_create_info(file);
    file_add_ui_message_to_info(file, message);
  }

  ui_message_update(message, app, msg);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
      msg
  );

  const gchar *text = GNUNET_CHAT_message_get_text(msg);
  const gchar *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  if ((!ui_messenger_is_context_active(&(app->ui.messenger), context)) &&
      (GNUNET_YES != sent))
    _show_notification(
	app,
	context,
	contact,
	text,
	"mail-unread-symbolic"
    );

  gtk_label_set_text(message->text_label, text? text : "");
  gtk_label_set_text(message->timestamp_label, time? time : "");

  ui_chat_add_message(handle->chat, app, message);
  ui_chat_entry_update(handle, app, context);
}
