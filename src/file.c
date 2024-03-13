/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file file.c
 */

#include "file.h"

void
file_create_info(struct GNUNET_CHAT_File *file)
{
  if ((!file) || (GNUNET_CHAT_file_get_user_pointer(file)))
    return;

  MESSENGER_FileInfo* info = g_malloc(sizeof(MESSENGER_FileInfo));

  info->app = NULL;

  info->update_task = 0;
  info->file_messages = NULL;

  GNUNET_CHAT_file_set_user_pointer(file, info);
}

void
file_destroy_info(struct GNUNET_CHAT_File *file)
{
  g_assert(file);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  if (info->update_task)
    g_source_remove(info->update_task);

  if (info->file_messages)
    g_list_free(info->file_messages);

  g_free(info);

  GNUNET_CHAT_file_set_user_pointer(file, NULL);
}

void
file_add_ui_message_to_info(const struct GNUNET_CHAT_File *file,
                            UI_MESSAGE_Handle *message)
{
  g_assert(message);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
   return;

  info->file_messages = g_list_append(info->file_messages, message);
}

void
file_update_upload_info(const struct GNUNET_CHAT_File *file,
                        uint64_t completed,
                        uint64_t size)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    gtk_progress_bar_set_fraction(
      message->file_progress_bar,
      1.0 * completed / size
    );

    list = list->next;
  }
}

static gboolean
file_update_messages(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_FileInfo* info = (MESSENGER_FileInfo*) user_data;

  info->update_task = 0;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    ui_message_update(message, info->app, message->msg);

    list = list->next;
  }

  return FALSE;
}

void
file_update_download_info(const struct GNUNET_CHAT_File *file,
                          MESSENGER_Application *app,
                          uint64_t completed,
                          uint64_t size)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    gtk_progress_bar_set_fraction(
      message->file_progress_bar,
      1.0 * completed / size
    );

    list = list->next;
  }

  if ((completed < size) || (info->update_task))
    return;

  info->app = app;
  info->update_task = g_idle_add(file_update_messages, info);
}
