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
 * @file ui/message.h
 */

#ifndef UI_MESSAGE_H_
#define UI_MESSAGE_H_

#include <stdbool.h>

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef enum UI_MESSAGE_Type
{
  UI_MESSAGE_DEFAULT 	= 0,
  UI_MESSAGE_SENT 	= 1,
  UI_MESSAGE_STATUS 	= 2
} UI_MESSAGE_Type;

typedef enum UI_MESSAGE_ContentType
{
  UI_MESSAGE_CONTENT_TEXT 	= 0,
  UI_MESSAGE_CONTENT_FILE 	= 1,
  UI_MESSAGE_CONTENT_PREVIEW 	= 2
} UI_MESSAGE_ContentType;

typedef struct UI_MESSAGE_Handle
{
  UI_MESSAGE_Type type;

  GtkBuilder *builder;
  GtkWidget *message_box;

  HdyAvatar *sender_avatar;
  GtkLabel *sender_label;

  GtkRevealer *deny_revealer;
  GtkRevealer *accept_revealer;

  GtkButton *deny_button;
  GtkButton *accept_button;

  GtkLabel *timestamp_label;
  GtkImage *read_receipt_image;

  GtkLabel *text_label;
  GtkRevealer *file_revealer;
  GtkDrawingArea *preview_drawing_area;
} UI_MESSAGE_Handle;

UI_MESSAGE_Handle*
ui_message_new(UI_MESSAGE_Type type,
	       UI_MESSAGE_ContentType content_type);

void
ui_message_delete(UI_MESSAGE_Handle *handle);

#endif /* UI_MESSAGE_H_ */
