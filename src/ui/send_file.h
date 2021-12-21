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
 * @file ui/send_file.h
 */

#ifndef UI_SEND_FILE_H_
#define UI_SEND_FILE_H_

#include "messenger.h"

typedef struct UI_SEND_FILE_Handle
{
  GList *contact_entries;

  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkDrawingArea *file_drawing_area;
  GtkFileChooserButton *file_chooser_button;

  GtkButton *cancel_button;
  GtkButton *send_button;

  GdkPixbuf *image;
  GdkPixbufAnimation *animation;
  GdkPixbufAnimationIter *animation_iter;

  guint redraw_animation;
} UI_SEND_FILE_Handle;

void
ui_send_file_dialog_init(MESSENGER_Application *app,
			 UI_SEND_FILE_Handle *handle);

void
ui_send_file_dialog_update(UI_SEND_FILE_Handle *handle,
			   const gchar *filename);

void
ui_send_file_dialog_cleanup(UI_SEND_FILE_Handle *handle);

#endif /* UI_SEND_FILE_H_ */
