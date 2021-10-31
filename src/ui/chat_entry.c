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

UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(void)
{
  UI_CHAT_ENTRY_Handle* handle = g_malloc(sizeof(UI_CHAT_ENTRY_Handle));

  GtkBuilder* builder = gtk_builder_new_from_file("resources/ui/chat_entry.ui");

  handle->entry_box = GTK_WIDGET(
      gtk_builder_get_object(builder, "entry_box")
  );

  handle->avatar = HDY_AVATAR(
      gtk_builder_get_object(builder, "avatar")
  );

  handle->title = GTK_LABEL(
      gtk_builder_get_object(builder, "title")
  );

  handle->timestamp = GTK_LABEL(
      gtk_builder_get_object(builder, "timestamp")
  );

  handle->text = GTK_LABEL(
      gtk_builder_get_object(builder, "text")
  );

  return handle;
}
