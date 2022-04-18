/*
   This file is part of GNUnet.
   Copyright (C) 2022 GNUnet e.V.

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
 * @file file.h
 */

#ifndef FILE_H_
#define FILE_H_

#include "application.h"
#include "ui/message.h"

typedef struct MESSENGER_FileInfo
{
  GList *file_messages;
} MESSENGER_FileInfo;

/**
 * Creates a file information struct to potentially update
 * all GUI appearances of a specific file at once.
 *
 * @param file Chat file
 */
void
file_create_info(struct GNUNET_CHAT_File *file);

/**
 * Destroys and frees resources allocated for a given
 * file information struct.
 *
 * @param file Chat file
 */
void
file_destroy_info(struct GNUNET_CHAT_File *file);

/**
 * Adds a UI message handle to the list of handles
 * which get updated by state changes.
 *
 * @param file Chat file
 * @param message UI message handle
 */
void
file_add_ui_message_to_info(const struct GNUNET_CHAT_File *file,
			    UI_MESSAGE_Handle *message);

/**
 * Updates the connected UI elements for a given
 * file depending on the current state of its upload
 * process.
 *
 * @param file Chat file
 * @param completed Amount of uploaded bytes
 * @param size Size of the file in bytes
 */
void
file_update_upload_info(const struct GNUNET_CHAT_File *file,
			uint64_t completed,
			uint64_t size);

/**
 * Updates the connected UI elements for a given
 * file depending on the current state of its download
 * process.
 *
 * @param file Chat file
 * @param app Messenger application
 * @param completed Amount of downloaded bytes
 * @param size Size of the file in bytes
 */
void
file_update_download_info(const struct GNUNET_CHAT_File *file,
			  MESSENGER_Application *app,
			  uint64_t completed,
			  uint64_t size);

#endif /* FILE_H_ */
