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
 * @file event.c
 */

#include "event.h"

#include "account.h"
#include "application.h"
#include "contact.h"
#include "discourse.h"
#include "file.h"
#include "ui.h"

#include "ui/chat_entry.h"
#include "ui/chat_title.h"
#include "ui/message.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>

static void
_close_notification(NotifyNotification* notification,
		                gpointer user_data)
{
  g_assert((notification) && (user_data));

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
                   const char *text,
                   const char *icon,
                   const char *category)
{
  g_assert(
    (app) &&
    (text) &&
    (icon) &&
    (category)
  );

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
  g_assert((app) && (msg));

  const char *text = GNUNET_CHAT_message_get_text(msg);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  g_printerr("ERROR: %s\n", text);

  _show_notification(
    app,
    context,
    contact,
    text,
    "dialog-warning-symbolic",
    "im.error"
  );
}

static enum GNUNET_GenericReturnValue
_iterate_reload_account(void *cls,
                        UNUSED const struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Account *account)
{
  g_assert((cls) && (account));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  account_create_info(account);
  account_update_attributes(account, app);

  return GNUNET_YES;
}

static void
_reload_accounts(MESSENGER_Application *app)
{
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  GNUNET_CHAT_iterate_accounts(
    chat->handle,
    _iterate_reload_account,
    app
  );

  if (gtk_widget_is_visible(GTK_WIDGET(ui->main_window)))
    ui_messenger_refresh(app, ui);
  else
    ui_accounts_dialog_refresh(app, &(app->ui.accounts));

  if (GNUNET_CHAT_get_connected(chat->handle))
    application_show_window(app);
}

static gboolean
_idle_refresh_accounts(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->ui.messenger.account_refresh = 0;

  if (!(app->ui.messenger.main_window))
    return FALSE;

  _reload_accounts(app);

  return FALSE;
}

void
event_refresh_accounts(MESSENGER_Application *app)
{
  g_assert(app);

  if (app->ui.messenger.account_refresh)
    util_source_remove(app->ui.messenger.account_refresh);

  if (app->ui.messenger.main_window)
    app->ui.messenger.account_refresh = util_idle_add(
      G_SOURCE_FUNC(_idle_refresh_accounts),
      app
    );
  else
    app->ui.messenger.account_refresh = 0;
}

static gboolean
_select_chat_to_activate(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) user_data;

  if ((!entry) || (!(entry->chat)) || (!(entry->entry_box)))
    return FALSE;

  MESSENGER_Application *app = entry->chat->app;

  if (!app)
    return FALSE;

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  if (!row)
    return FALSE;

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  gtk_list_box_select_row(ui->chats_listbox, row);
  gtk_list_box_invalidate_filter(ui->chats_listbox);

  gtk_widget_activate(GTK_WIDGET(row));

  ui->chat_selection = 0;
  return FALSE;
}

static gboolean
_idle_chat_entry_update(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) user_data;

  if ((!(entry->chat)) || (!(entry->chat->app)) ||
      (!(entry->chat->send_text_view)))
    goto update_exit;

  ui_chat_entry_update(entry, entry->chat->app);

update_exit:
  entry->update = 0;
  return FALSE;
}

static void
enqueue_chat_entry_update(UI_CHAT_ENTRY_Handle *entry)
{
  g_assert(entry);

  if (entry->update)
    util_source_remove(entry->update);

  entry->update = util_idle_add(
    G_SOURCE_FUNC(_idle_chat_entry_update),
    entry
  );
}

static void
_add_new_chat_entry(MESSENGER_Application *app,
		                struct GNUNET_CHAT_Context *context)
{
  g_assert((app) && (context));

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  UI_CHAT_ENTRY_Handle *entry = ui_chat_entry_new(app, context);

  enqueue_chat_entry_update(entry);

  gtk_container_add(GTK_CONTAINER(ui->chats_listbox), entry->entry_box);

  gtk_container_add(
    GTK_CONTAINER(ui->chats_stack),
    entry->chat->chat_box
  );

  gtk_container_add(
    GTK_CONTAINER(ui->chat_title_stack),
    entry->chat->title->chat_title_box
  );

  ui->chat_entries = g_list_append(ui->chat_entries, entry);

  GtkWidget *row = gtk_widget_get_parent(entry->entry_box);

  g_object_set_qdata(
    G_OBJECT(row),
    app->quarks.ui,
    entry
  );

  if (ui->chat_selection)
    util_source_remove(ui->chat_selection);

  ui->chat_selection = util_idle_add(
    G_SOURCE_FUNC(_select_chat_to_activate),
    entry
  );
}

static int
_iterate_profile_contacts(void *cls,
			                    UNUSED struct GNUNET_CHAT_Handle *handle,
			                    struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

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
  g_assert(cls);

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
  g_assert(app);

  if (app->ui.new_account.dialog)
    ui_new_account_dialog_update(app, &(app->ui.new_account));

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  _reload_accounts(app);

  const char *name = GNUNET_CHAT_get_name(chat->handle);

  account_switch_name_avatar_to_info(
    GNUNET_CHAT_get_connected(chat->handle),
    ui->profile_avatar
  );

  ui_label_set_text(ui->profile_label, name);

  const char *key = GNUNET_CHAT_get_key(chat->handle);

  ui_label_set_text(ui->profile_key_label, key);

  gtk_stack_set_visible_child(ui->chats_stack, ui->no_chat_box);
  
  GList *children = gtk_container_get_children(GTK_CONTAINER(ui->leaflet_chat));

  if (children) {
    hdy_leaflet_set_visible_child(ui->leaflet_chat, GTK_WIDGET(children->data));
    g_list_free(children);
  }

  GNUNET_CHAT_iterate_contacts(chat->handle, _iterate_profile_contacts, app);
  GNUNET_CHAT_iterate_groups(chat->handle, _iterate_profile_groups, app);
}

static void
_clear_chat_entry(GtkWidget *widget,
		              gpointer user_data)
{
  g_assert((widget) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(widget);

  if (!gtk_list_box_row_get_selectable(row))
    return;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) g_object_get_qdata(
    G_OBJECT(row),
    app->quarks.ui
  );

  ui->chat_entries = g_list_remove(ui->chat_entries, entry);

  ui_chat_entry_dispose(entry, app);
}

static int
_cleanup_profile_contacts(void *cls,
			                    UNUSED struct GNUNET_CHAT_Handle *handle,
			                    struct GNUNET_CHAT_Contact *contact)
{
  if (contact)
    contact_destroy_info(contact);
  return GNUNET_YES;
}

static int
_cleanup_profile_files(void *cls,
			                    UNUSED struct GNUNET_CHAT_Handle *handle,
			                    struct GNUNET_CHAT_File *file)
{
  if (file)
    file_destroy_info(file);
  return GNUNET_YES;
}

void
event_cleanup_profile(MESSENGER_Application *app)
{
  g_assert(app);

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  account_remove_name_avatar_from_info(
    GNUNET_CHAT_get_connected(chat->handle),
    ui->profile_avatar
  );

  GList *entries = gtk_container_get_children(
    GTK_CONTAINER(ui->chats_listbox)
  );

  g_list_foreach(entries, (GFunc) _clear_chat_entry, app);
  g_list_free(entries);

  GNUNET_CHAT_iterate_contacts(chat->handle, _cleanup_profile_contacts, NULL);
  GNUNET_CHAT_iterate_files(chat->handle, _cleanup_profile_files, NULL);
}

void
event_select_profile(MESSENGER_Application *app,
		                 struct GNUNET_CHAT_Context *context,
		                 const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (!context) && (msg));

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  const struct GNUNET_CHAT_Account *account = GNUNET_CHAT_message_get_account(msg);

  if (GNUNET_CHAT_KIND_CREATED_ACCOUNT == GNUNET_CHAT_message_get_kind(msg))
    GNUNET_CHAT_connect(chat->handle, account);

  ui_messenger_refresh(app, ui);
}

gboolean
_delayed_context_drop(gpointer user_data)
{
  g_assert(user_data);

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) user_data;

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_context_get_group(context);
  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_context_get_contact(context);

  if (group)
    GNUNET_CHAT_group_leave(group);
  else if (contact)
    GNUNET_CHAT_contact_delete(contact);

  return FALSE;
}

void
event_update_chats(MESSENGER_Application *app,
		               struct GNUNET_CHAT_Context *context,
		               const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  const enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(
    msg
  );

  if (GNUNET_CHAT_KIND_JOIN == kind)
  {
    if (!handle)
      _add_new_chat_entry(app, context);
    else
      enqueue_chat_entry_update(handle);

    if (app->settings.leave_chats_delay > 0)
      util_timeout_add_seconds(
        app->settings.leave_chats_delay,
        _delayed_context_drop,
        context
      );
  }
  else if ((handle) && (handle->entry_box))
    _clear_chat_entry(gtk_widget_get_parent(handle->entry_box), app);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  if (!contact)
    return;

  if (GNUNET_YES == contact_create_info(contact))
    contact_update_attributes(contact, app);
}

static void
_update_contact_context(struct GNUNET_CHAT_Contact *contact)
{
  g_assert(contact);

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
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  if (!contact)
    return;

  contact_create_info(contact);

  if (!handle)
    return;

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_context_get_group(context);

  UI_MESSAGE_Handle *message = NULL;
  gboolean previous_presence = FALSE;

  if (group)
    message = (UI_MESSAGE_Handle*) (
	    GNUNET_CHAT_member_get_user_pointer(group, contact)
    );
  else
    message = (UI_MESSAGE_Handle*) contact_get_last_message_from_info(contact);

  if (message)
  {
    ui_chat_remove_message(handle->chat, app, message);
    previous_presence = TRUE;
  }

  message = ui_message_new(app, UI_MESSAGE_STATUS);
  ui_message_update(message, app, msg);

  contact_update_attributes(contact, app);
  _update_contact_context(contact);

  ui_message_set_contact(message, contact);

  const enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(
    msg
  );

  const char *text = (
    GNUNET_CHAT_KIND_JOIN == kind? _("joined the chat") : _("left the chat")
  );

  if ((!ui_messenger_is_context_active(&(app->ui.messenger), context)) &&
      ((!previous_presence) || (GNUNET_CHAT_KIND_LEAVE == kind)) &&
      (GNUNET_YES == GNUNET_CHAT_message_is_recent(msg)))
    _show_notification(
      app,
      context,
      contact,
      text,
      "avatar-default-symbolic",
      "presence.online"
    );

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
    msg
  );

  const char *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  ui_label_set_text(message->text_label, text);
  ui_label_set_text(message->timestamp_label, time);

  ui_chat_add_message(handle->chat, app, message);

  if (group)
    GNUNET_CHAT_member_set_user_pointer(group, contact, message);
  else
    contact_set_last_message_to_info(contact, message);

  enqueue_chat_entry_update(handle);
}

void
event_update_contacts(UNUSED MESSENGER_Application *app,
		                  struct GNUNET_CHAT_Context *context,
		                  const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (msg));

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  if (!contact)
    return;

  if (GNUNET_CHAT_KIND_SHARED_ATTRIBUTES == GNUNET_CHAT_message_get_kind(msg))
    contact_update_attributes(contact, app);

  contact_update_info(contact);
  _update_contact_context(contact);

  if (!context)
    return;

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  enqueue_chat_entry_update(handle);
}

static void
_event_invitation_accept_click(UNUSED GtkButton *button,
			                         gpointer user_data)
{
  g_assert(user_data);

  struct GNUNET_CHAT_Invitation *invitation = (
    (struct GNUNET_CHAT_Invitation*) user_data
  );

  GNUNET_CHAT_invitation_accept(invitation);
}

static void
_event_invitation_deny_click(UNUSED GtkButton *button,
                             gpointer user_data)
{
  g_assert(user_data);

  struct GNUNET_CHAT_Invitation *invitation = (
    (struct GNUNET_CHAT_Invitation*) user_data
  );

  GNUNET_CHAT_invitation_reject(invitation);
}

void
event_invitation(MESSENGER_Application *app,
                 struct GNUNET_CHAT_Context *context,
                 const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  struct GNUNET_CHAT_Invitation *invitation;
  invitation = GNUNET_CHAT_message_get_invitation(msg);

  if (!invitation)
    return;

  if (app->settings.delete_invitations_delay > 0)
    GNUNET_CHAT_message_delete(msg, GNUNET_TIME_relative_multiply(
      GNUNET_TIME_relative_get_second_(),
      app->settings.delete_invitations_delay
    ));

  const int sent = GNUNET_CHAT_message_is_sent(msg);

  if ((GNUNET_YES != sent) && (app->settings.send_read_receipts))
    GNUNET_CHAT_context_send_read_receipt(context, msg);

  UI_MESSAGE_Handle *message = ui_message_new(app, UI_MESSAGE_STATUS);
  ui_message_update(message, app, msg);

  const struct GNUNET_CHAT_Contact *sender = GNUNET_CHAT_message_get_sender(
    msg
  );

  const struct GNUNET_CHAT_Contact *recipient = GNUNET_CHAT_message_get_recipient(
    msg
  );

  ui_message_set_contact(message, sender);

  const char *invite_message = _("invited %s to a chat");
  const char *recipient_name = (
    (recipient) && 
    (GNUNET_YES != GNUNET_CHAT_contact_is_owned(recipient))
  )? GNUNET_CHAT_contact_get_name(recipient) : _("you");

  GString *message_string = g_string_new(NULL);
  g_string_printf(message_string, invite_message, recipient_name);

  if ((!ui_messenger_is_context_active(&(app->ui.messenger), context)) &&
      (GNUNET_YES == GNUNET_CHAT_message_is_recent(msg)))
    _show_notification(
	    app,
      context,
      sender,
      message_string->str,
      "mail-message-new-symbolic",
      "im.received"
    );

  ui_label_set_text(message->text_label, message_string->str);
  g_string_free(message_string, TRUE);

  g_signal_connect(
    message->accept_button,
    "clicked",
    G_CALLBACK(_event_invitation_accept_click),
    invitation
  );

  g_signal_connect(
    message->deny_button,
    "clicked",
    G_CALLBACK(_event_invitation_deny_click),
    invitation
  );

  ui_chat_add_message(handle->chat, app, message);

  enqueue_chat_entry_update(handle);
}

void
event_receive_message(MESSENGER_Application *app,
                      struct GNUNET_CHAT_Context *context,
                      const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if (!handle)
    return;

  const int sent = GNUNET_CHAT_message_is_sent(msg);

  if ((sent) && (app->settings.auto_delete_delay > 0))
    GNUNET_CHAT_message_delete(msg, GNUNET_TIME_relative_multiply(
      GNUNET_TIME_relative_get_second_(),
      app->settings.auto_delete_delay
    ));

  const gchar *text = GNUNET_CHAT_message_get_text(msg);

  if ((text) && (!(*text)))
    goto skip_message;

  if ((GNUNET_YES != sent) && (app->settings.send_read_receipts))
    GNUNET_CHAT_context_send_read_receipt(context, msg);

  const UI_MESSAGE_Type type = (
    GNUNET_YES == sent? UI_MESSAGE_SENT : UI_MESSAGE_DEFAULT
  );

  UI_MESSAGE_Handle *message = ui_message_new(app, type);

  struct GNUNET_CHAT_File *file = GNUNET_CHAT_message_get_file(msg);

  if (file)
  {
    file_create_info(file);
    file_add_ui_message_to_info(file, message);

    if (app->settings.delete_files_delay > 0)
      GNUNET_CHAT_message_delete(msg, GNUNET_TIME_relative_multiply(
        GNUNET_TIME_relative_get_second_(),
        app->settings.delete_files_delay
      ));
  }

  ui_message_update(message, app, msg);

  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_message_get_sender(
    msg
  );

  ui_message_set_contact(message, contact);

  struct GNUNET_TIME_Absolute timestamp = GNUNET_CHAT_message_get_timestamp(
    msg
  );

  const char *time = GNUNET_STRINGS_absolute_time_to_string(timestamp);

  if ((!ui_messenger_is_context_active(&(app->ui.messenger), context)) &&
      (GNUNET_YES == GNUNET_CHAT_message_is_recent(msg)) &&
      (GNUNET_YES != sent))
    _show_notification(
      app,
      context,
      contact,
      text,
      "mail-unread-symbolic",
      "im.received"
    );

  ui_label_set_markup_text(message->text_label, text);
  ui_label_set_text(message->timestamp_label, time);

  ui_chat_add_message(handle->chat, app, message);

skip_message:
  enqueue_chat_entry_update(handle);
}

static void
_event_update_tag_message_state(MESSENGER_Application *app,
                                struct GNUNET_CHAT_Context *context,
                                const struct GNUNET_CHAT_Message *msg)
{
  g_assert((msg) && (GNUNET_CHAT_KIND_TAG == GNUNET_CHAT_message_get_kind(msg)));

  const struct GNUNET_CHAT_Message *target;
  target = GNUNET_CHAT_message_get_target(msg);

  if (!target)
    return;

  struct GNUNET_CHAT_Contact *contact;
  contact = GNUNET_CHAT_message_get_sender(target);

  if (contact)
    contact_update_info(contact);

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!handle) || (!(handle->chat)))
    return;

  GList *rows = gtk_container_get_children(
    GTK_CONTAINER(handle->chat->messages_listbox)
  );

  for (GList *row = rows; row; row = row->next)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row->data), app->quarks.ui
    );

    if ((message) && (message->msg == target))
    {
      if (GNUNET_YES == GNUNET_CHAT_message_is_deleted(msg))
        ui_message_remove_tag(message, app, msg);
      else
        ui_message_add_tag(message, app, msg);

      break;
    }
  }

  if (rows)
    g_list_free(rows);
}

void
event_delete_message(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!handle) || (!(handle->chat)))
    return;

  GList *rows = gtk_container_get_children(
    GTK_CONTAINER(handle->chat->messages_listbox)
  );

  for (GList *row = rows; row; row = row->next)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row->data), app->quarks.ui
    );

    if ((message) && (message->msg == msg))
    {
      ui_chat_remove_message(handle->chat, app, message);
      break;
    }
  }

  if (rows)
    g_list_free(rows);

  if (GNUNET_CHAT_KIND_TAG == GNUNET_CHAT_message_get_kind(msg))
    _event_update_tag_message_state(app, context, msg);

  enqueue_chat_entry_update(handle);
}

void
event_tag_message(MESSENGER_Application *app,
                  struct GNUNET_CHAT_Context *context,
                  const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  UI_CHAT_ENTRY_Handle *handle = GNUNET_CHAT_context_get_user_pointer(context);

  const struct GNUNET_CHAT_Message *target = GNUNET_CHAT_message_get_target(msg);

  _event_update_tag_message_state(app, context, msg);

  if ((!handle) || (!(handle->chat)))
    return;

  GList *rows = gtk_container_get_children(
    GTK_CONTAINER(handle->chat->messages_listbox)
  );

  for (GList *row = rows; row; row = row->next)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row->data), app->quarks.ui
    );

    if ((message) && (message->msg == target))
    {
      ui_message_update(message, app, message->msg);
      break;
    }
  }

  if (rows)
    g_list_free(rows);

  enqueue_chat_entry_update(handle);
}

static enum GNUNET_GenericReturnValue
_iterate_contacts_update_own(void *cls,
                             UNUSED struct GNUNET_CHAT_Handle *handle,
                             struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (GNUNET_YES != GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  contact_update_attributes(contact, app);
  return GNUNET_YES;
}

void
event_update_attributes(MESSENGER_Application *app)
{
  g_assert(app);

  CHAT_MESSENGER_Handle *chat = &(app->chat.messenger);

  const struct GNUNET_CHAT_Account *account = GNUNET_CHAT_get_connected(
    chat->handle
  );

  if (account)
    account_update_attributes(account, app);

  GNUNET_CHAT_iterate_contacts(
    chat->handle,
    _iterate_contacts_update_own,
    app
  );
}

void
event_discourse(MESSENGER_Application *app,
                struct GNUNET_CHAT_Context *context,
                const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  struct GNUNET_CHAT_Discourse *discourse = GNUNET_CHAT_message_get_discourse(
    msg
  );

  if (GNUNET_YES == GNUNET_CHAT_message_is_sent(msg))
  {
    if (GNUNET_YES == GNUNET_CHAT_discourse_is_open(discourse))
      discourse_create_info(discourse);
    else
      discourse_destroy_info(discourse);
  }

  if (context == app->ui.discourse.context)
    ui_discourse_window_update(&(app->ui.discourse), context);
}

void
event_discourse_data(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     const struct GNUNET_CHAT_Message *msg)
{
  g_assert((app) && (context) && (msg));

  if (GNUNET_YES == GNUNET_CHAT_message_is_sent(msg))
    return;

  // TODO
}
