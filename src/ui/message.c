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
 * @file ui/message.c
 */

#include "message.h"

#include "../application.h"

UI_MESSAGE_Handle*
ui_message_new(UI_MESSAGE_Type type,
	       UI_MESSAGE_ContentType content_type)
{
  UI_MESSAGE_Handle* handle = g_malloc(sizeof(UI_MESSAGE_Handle));

  handle->type = type;

  const char *ui_builder_file;

  switch (handle->type)
  {
    case UI_MESSAGE_SENT:
      ui_builder_file = "resources/ui/message-sent.ui";
      break;
    case UI_MESSAGE_STATUS:
      ui_builder_file = "resources/ui/message-status.ui";
      break;
    default:
      ui_builder_file = "resources/ui/message.ui";
      break;
  }

  handle->builder = gtk_builder_new_from_file(ui_builder_file);

  handle->message_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "message_box")
  );

  handle->sender_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "sender_avatar")
  );

  handle->sender_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "sender_label")
  );

  if (UI_MESSAGE_STATUS == handle->type)
  {
    handle->deny_revealer = GTK_REVEALER(
	gtk_builder_get_object(handle->builder, "deny_revealer")
    );

    handle->accept_revealer = GTK_REVEALER(
    	gtk_builder_get_object(handle->builder, "accept_revealer")
    );

    handle->deny_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "deny_button")
    );

    handle->accept_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "accept_button")
    );
  }
  else
  {
    handle->deny_revealer = NULL;
    handle->accept_revealer = NULL;

    handle->deny_button = NULL;
    handle->accept_button = NULL;
  }

  GtkContainer *content_box = GTK_CONTAINER(
      gtk_builder_get_object(handle->builder, "content_box")
  );

  GtkBuilder *builder = gtk_builder_new_from_file(
      "resources/ui/message_content.ui"
  );

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(builder, "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(builder, "read_receipt_image")
  );

  handle->content_stack = GTK_STACK(
      gtk_builder_get_object(builder, "content_stack")
  );

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(builder, "text_label")
  );

  handle->file_revealer = GTK_REVEALER(
      gtk_builder_get_object(builder, "file_revealer")
  );

  handle->preview_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(builder, "preview_drawing_area")
  );

  switch (handle->type)
  {
    case UI_MESSAGE_STATUS:
      gtk_widget_set_visible(GTK_WIDGET(handle->timestamp_label), FALSE);
      break;
    default:
      break;
  }

  switch (content_type)
  {
    case UI_MESSAGE_CONTENT_TEXT:
      gtk_stack_set_visible_child(
	  handle->content_stack,
	  GTK_WIDGET(handle->text_label)
      );
      break;
    case UI_MESSAGE_CONTENT_FILE:
      gtk_stack_set_visible_child(
	  handle->content_stack,
	  GTK_WIDGET(handle->file_revealer)
      );

      gtk_revealer_set_reveal_child(handle->file_revealer, TRUE);
      break;
    case UI_MESSAGE_CONTENT_PREVIEW:
      gtk_stack_set_visible_child(
	  handle->content_stack,
	  GTK_WIDGET(handle->preview_drawing_area)
      );
      break;
    default:
      break;
  }

  gtk_container_add(content_box, GTK_WIDGET(
      gtk_builder_get_object(builder, "message_content_box")
  ));

  g_object_unref(builder);
  return handle;
}

void
ui_message_update(UI_MESSAGE_Handle *handle,
		  struct GNUNET_CHAT_Message *msg)
{
  struct GNUNET_CHAT_File *file = GNUNET_CHAT_message_get_file(msg);

  if (!file)
    return;

  gtk_stack_set_visible_child(
      handle->content_stack,
      GTK_WIDGET(handle->file_revealer)
  );

  gtk_revealer_set_reveal_child(handle->file_revealer, TRUE);
}

void
ui_message_delete(UI_MESSAGE_Handle *handle)
{
  g_object_unref(handle->builder);

  g_free(handle);
}
