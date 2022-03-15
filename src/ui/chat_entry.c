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
 * @file ui/chat_entry.c
 */

#include "chat_entry.h"

#include "../application.h"
#include "../contact.h"

#include "message.h"

UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(MESSENGER_Application *app)
{
  UI_CHAT_ENTRY_Handle* handle = g_malloc(sizeof(UI_CHAT_ENTRY_Handle));

  memset(handle, 0, sizeof(*handle));

  handle->chat = ui_chat_new(app);
  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/chat_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->entry_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "entry_avatar")
  );

  handle->title_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "title_label")
  );

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "timestamp_label")
  );

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "text_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "read_receipt_image")
  );

  return handle;
}

void
ui_chat_entry_update(UI_CHAT_ENTRY_Handle *handle,
		     MESSENGER_Application *app,
		     const struct GNUNET_CHAT_Context *context)
{
  const struct GNUNET_CHAT_Contact* contact;
  const struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(context);
  group = GNUNET_CHAT_context_get_group(context);

  const char *title = NULL;
  const char *icon = "action-unavailable-symbolic";

  if (contact)
  {
    title = GNUNET_CHAT_contact_get_name(contact);
    icon = "avatar-default-symbolic";
  }
  else if (group)
  {
    title = GNUNET_CHAT_group_get_name(group);

    if ((title) && ('#' == *title))
      icon = "network-wired-symbolic";
    else
      icon = "system-users-symbolic";
  }

  gtk_label_set_text(handle->title_label, title? title : "");

  hdy_avatar_set_text(handle->entry_avatar, title? title : "");
  hdy_avatar_set_icon_name(handle->entry_avatar, icon);

  if (!(handle->chat))
    return;

  ui_chat_update(handle->chat, app, context);

  hdy_avatar_set_text(handle->chat->chat_avatar, title? title : "");
  hdy_avatar_set_icon_name(handle->chat->chat_avatar, icon);

  if (!(handle->chat->messages))
    return;

  UI_MESSAGE_Handle *message = (
      (UI_MESSAGE_Handle*) handle->chat->messages->data
  );

  const gchar *text = gtk_label_get_text(message->text_label);
  const gchar *time = gtk_label_get_text(message->timestamp_label);

  if (group)
  {
    GString *message_text = g_string_new(
	gtk_label_get_text(message->sender_label)
    );

    g_string_append_printf(
	message_text,
    	_(": %s"),
	text
    );

    gtk_label_set_text(handle->text_label, message_text->str);
    g_string_free(message_text, TRUE);
  }
  else
    gtk_label_set_text(handle->text_label, text);

  gtk_label_set_text(handle->timestamp_label, time);

  gtk_widget_set_visible(
      GTK_WIDGET(handle->read_receipt_image),
      message->read_receipt_image? gtk_widget_is_visible(
	  GTK_WIDGET(message->read_receipt_image)
      ) : FALSE
  );
}

void
ui_chat_entry_delete(UI_CHAT_ENTRY_Handle *handle)
{
  ui_chat_delete(handle->chat);

  g_object_unref(handle->builder);

  if (handle->update)
    g_source_remove(handle->update);

  g_free(handle);
}
