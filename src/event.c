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
		   const gchar *icon,
		   const gchar *category)
{
  if (app->settings.disable_notifications)
    return;

  const char *sender = GNUNET_CHAT_contact_get_name(contact);

  NotifyNotification *notification = notify_notification_new(
      sender? sender : "(unknown)", text, icon
  );

  if (!notification)
    return;

  app->notifications = g_list_append(app->notifications, notification);

  notify_notification_set_category(notification, category);

  g_signal_connect(
      notification,
      "closed",
      G_CALLBACK(_close_notification),
      app
  );

  notify_notification_show(notification, NULL);
}

void
event_handle_warning(MESSENGER_Application *app,
		     struct GNUNET_CHAT_Context *context,
		     const struct GNUNET_CHAT_Message *msg)
{
  const gchar *text = GNUNET_CHAT_message_get_text(msg);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  fprintf(stderr, "ERROR: %s\n", text);

  _show_notification(
      app,
      context,
      contact,
      text,
      "dialog-warning-symbolic",
      "im.error"
  );
}

static gboolean
_idle_refresh_accounts(gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!(app->ui.messenger.main_window))
    goto refresh_exit;

  if (gtk_widget_is_visible(GTK_WIDGET(app->ui.messenger.main_window)))
    ui_messenger_refresh(app, &(app->ui.messenger));
  else
    ui_accounts_dialog_refresh(app, &(app->ui.accounts));

refresh_exit:
  app->ui.messenger.account_refresh = 0;
  return FALSE;
}

void
event_refresh_accounts(MESSENGER_Application *app)
{
  if (app->ui.messenger.account_refresh)
    g_source_remove(app->ui.messenger.account_refresh);

  if (app->ui.messenger.main_window)
    app->ui.messenger.account_refresh = g_idle_add(
	G_SOURCE_FUNC(_idle_refresh_accounts),
	app
    );
  else
    app->ui.messenger.account_refresh = 0;
}

static gboolean
_select_chat_to_activate(gpointer user_data)
{
  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) user_data;

  if (!(entry->chat))
    return FALSE;

  MESSENGER_Application *app = entry->chat->app;

  if (!app)
    return FALSE;

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  gtk_list_box_select_row(ui->chats_listbox, row);
  gtk_list_box_invalidate_filter(ui->chats_listbox);

  gtk_widget_activate(GTK_WIDGET(row));

  ui->chat_selection = 0;
  return FALSE;
}

static gboolean
_idle_chat_entry_update(gpointer user_data)
{
  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) user_data;

  if ((!(entry->chat)) || (!(entry->chat->app)) ||
      (!(entry->chat->send_text_view)))
    goto update_exit;

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) (
      g_object_get_qdata(
	  G_OBJECT(entry->chat->send_text_view),
	  entry->chat->app->quarks.data
      )
  );

  ui_chat_entry_update(entry, entry->chat->app, context);

update_exit:
  entry->update = 0;
  return FALSE;
}

static void
enqueue_chat_entry_update(UI_CHAT_ENTRY_Handle *entry)
{
  if (entry->update)
    g_source_remove(entry->update);

  entry->update = g_idle_add(
      G_SOURCE_FUNC(_idle_chat_entry_update),
      entry
  );
}

static void
_add_new_chat_entry(MESSENGER_Application *app,
		    struct GNUNET_CHAT_Context *context)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new(app);

  enqueue_chat_entry_update(entry);

  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);
  GNUNET_CHAT_context_set_user_pointer(context, entry);

  gtk_container_add(
      GTK_CONTAINER(ui->chats_stack),
      entry->chat->chat_box
  );

  g_object_set_qdata(
      G_OBJECT(entry->chat->send_text_view),
      app->quarks.data,
      context
  );

  ui->chat_entries = g_list_append(ui->chat_entries, entry);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(
      G_OBJECT(row),
      app->quarks.ui,
      entry
  );

  if (ui->chat_selection)
    g_source_remove(ui->chat_selection);

  ui->chat_selection = g_idle_add(
      G_SOURCE_FUNC(_select_chat_to_activate),
      entry
  );
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

static void
_clear_chat_entry(GtkWidget *widget,
		  gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;
  GtkListBoxRow *row = GTK_LIST_BOX_ROW(widget);

  if (!gtk_list_box_row_get_selectable(row))
    return;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) g_object_get_qdata(
      G_OBJECT(row),
      app->quarks.ui
  );

  ui_chat_entry_dispose(entry, app);
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
      _clear_chat_entry,
      app
  );

  gtk_stack_set_visible_child(ui->chats_stack, ui->no_chat_box);

  GNUNET_CHAT_iterate_contacts(chat->handle, _iterate_profile_contacts, app);
  GNUNET_CHAT_iterate_groups(chat->handle, _iterate_profile_groups, app);
}

void
event_update_chats(MESSENGER_Application *app,
		   struct GNUNET_CHAT_Context *context,
		   const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  const enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(
      msg
  );

  if (GNUNET_CHAT_KIND_JOIN == kind)
    if (!handle)
      _add_new_chat_entry(app, context);
    else
      enqueue_chat_entry_update(handle);
  else if (handle)
    _clear_chat_entry(gtk_widget_get_parent(handle->entry_box), app);

  contact_create_info(GNUNET_CHAT_message_get_sender(msg));
}

static void
_update_contact_context(struct GNUNET_CHAT_Contact *contact)
{
  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (!context)
    return;

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  enqueue_chat_entry_update(handle);
}

void
event_presence_contact(MESSENGER_Application *app,
		       struct GNUNET_CHAT_Context *context,
		       const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  if (!contact)
    return;

  const enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(
      msg
  );

  UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) (
      GNUNET_CHAT_member_get_user_pointer(context, contact)
  );

  if (message)
    ui_chat_remove_message(handle->chat, app, message);

  message = ui_message_new(app, UI_MESSAGE_STATUS);
  ui_message_update(message, app, msg);

  contact_create_info(contact);
  _update_contact_context(contact);

  contact_add_name_avatar_to_info(contact, message->sender_avatar);
  contact_add_name_label_to_info(contact, message->sender_label);

  const gchar *presence_message = (
      GNUNET_CHAT_KIND_JOIN == kind? _("joined the chat") : _("left the chat")
  );

  if (!ui_messenger_is_context_active(&(app->ui.messenger), context))
    _show_notification(
	app,
	context,
	contact,
	presence_message,
	"avatar-default-symbolic",
	"presence.online"
    );

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
      msg
  );

  const gchar *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  gtk_label_set_text(message->text_label, presence_message);
  gtk_label_set_text(message->timestamp_label, time? time : "");

  ui_chat_add_message(handle->chat, app, message);

  GNUNET_CHAT_member_set_user_pointer(context, contact, message);

  enqueue_chat_entry_update(handle);
}

void
event_update_contacts(UNUSED MESSENGER_Application *app,
		      struct GNUNET_CHAT_Context *context,
		      const struct GNUNET_CHAT_Message *msg)
{
  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
      msg
  );

  if (!contact)
    return;

  contact_update_info(contact);
  _update_contact_context(contact);

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  enqueue_chat_entry_update(handle);
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
	"mail-message-new-symbolic",
	"im.received"
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

  enqueue_chat_entry_update(handle);
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
  const gchar *text = GNUNET_CHAT_message_get_text(msg);

  if ((text) && (!(*text)))
    goto skip_message;

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

  const gchar *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  if ((!ui_messenger_is_context_active(&(app->ui.messenger), context)) &&
      (GNUNET_YES != sent))
    _show_notification(
	app,
	context,
	contact,
	text,
	"mail-unread-symbolic",
	"im.received"
    );

  gtk_label_set_text(message->text_label, text? text : "");
  gtk_label_set_text(message->timestamp_label, time? time : "");

  ui_chat_add_message(handle->chat, app, message);

skip_message:
  enqueue_chat_entry_update(handle);
}

void
event_delete_message(MESSENGER_Application *app,
		     struct GNUNET_CHAT_Context *context,
		     const struct GNUNET_CHAT_Message *msg)
{
  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!handle) || (!(handle->chat)))
    return;

  GList *messages = handle->chat->messages;

  while (messages)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) (messages->data);

    if ((message) && (message->msg == GNUNET_CHAT_message_get_target(msg)))
    {
      ui_chat_remove_message(handle->chat, app, message);
      break;
    }

    messages = messages->next;
  }

  enqueue_chat_entry_update(handle);
}
