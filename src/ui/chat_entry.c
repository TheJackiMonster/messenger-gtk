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
 * @file ui/chat_entry.c
 */

#include "chat_entry.h"

#include "../application.h"

UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(MESSENGER_Application *app)
{
  UI_CHAT_ENTRY_Handle* handle = g_malloc(sizeof(UI_CHAT_ENTRY_Handle));

  handle->chat = ui_chat_new(app);
  handle->builder = gtk_builder_new_from_file("resources/ui/chat_entry.ui");

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
ui_chat_entry_activate(UI_CHAT_ENTRY_Handle *handle)
{
  ui_chat_activate(handle->chat);
}

void
ui_chat_entry_delete(UI_CHAT_ENTRY_Handle *handle)
{
  ui_chat_delete(handle->chat);

  g_object_unref(handle->builder);

  g_free(handle);
}
