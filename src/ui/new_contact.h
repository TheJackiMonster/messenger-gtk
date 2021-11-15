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
 * @file ui/new_contact.h
 */

#ifndef UI_NEW_CONTACT_H_
#define UI_NEW_CONTACT_H_

#include "messenger.h"

#include <gdk/gdkpixbuf.h>
#include <pthread.h>
#include <zbar.h>

typedef struct UI_NEW_CONTACT_Handle
{
  zbar_video_t *video;
  zbar_image_t *image;
  zbar_image_scanner_t *scanner;

  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkDrawingArea *id_drawing_area;
  GtkEntry *id_entry;

  GtkButton *cancel_button;
  GtkButton *confirm_button;

  pthread_t video_tid;
  guint idle_processing;
} UI_NEW_CONTACT_Handle;

void
ui_new_contact_dialog_init(MESSENGER_Application *app,
			   UI_NEW_CONTACT_Handle *handle);

void
ui_new_contact_dialog_cleanup(UI_NEW_CONTACT_Handle *handle);

#endif /* UI_NEW_CONTACT_H_ */
