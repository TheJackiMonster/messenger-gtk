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
ui_message_new(UI_MESSAGE_Type type)
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

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "text_label")
  );

  if (UI_MESSAGE_STATUS == handle->type)
  {
    handle->deny_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "deny_button")
    );

    handle->accept_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "accept_button")
    );
  }
  else
  {
    handle->deny_button = NULL;
    handle->accept_button = NULL;
  }

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "read_receipt_image")
  );

  return handle;
}

void
ui_message_delete(UI_MESSAGE_Handle *handle)
{
  g_object_unref(handle->builder);

  g_free(handle);
}
