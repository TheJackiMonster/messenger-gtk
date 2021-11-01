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
ui_message_new(MESSENGER_Application *app,
	       bool sent)
{
  UI_MESSAGE_Handle* handle = g_malloc(sizeof(UI_MESSAGE_Handle));
  GtkBuilder* builder;

  if (sent)
    builder = gtk_builder_new_from_file("resources/ui/message-sent.ui");
  else
    builder = gtk_builder_new_from_file("resources/ui/message.ui");

  handle->message_box = GTK_WIDGET(
      gtk_builder_get_object(builder, "message_box")
  );

  handle->sender_avatar = HDY_AVATAR(
      gtk_builder_get_object(builder, "sender_avatar")
  );

  handle->sender_label = GTK_LABEL(
      gtk_builder_get_object(builder, "sender_label")
  );

  if (sent)
  {
    const char *sender = GNUNET_CHAT_get_name(app->chat.messenger.handle);

    hdy_avatar_set_text(handle->sender_avatar, sender);
    gtk_label_set_text(handle->sender_label, sender);
  }

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(builder, "text_label")
  );

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(builder, "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(builder, "read_receipt_image")
  );

  return handle;
}
