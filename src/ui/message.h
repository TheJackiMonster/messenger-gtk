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

typedef struct UI_MESSAGE_Handle
{
  GtkBuilder *builder;
  GtkWidget *message_box;

  HdyAvatar *sender_avatar;
  GtkLabel *sender_label;

  GtkLabel *text_label;

  GtkLabel *timestamp_label;
  GtkImage *read_receipt_image;
} UI_MESSAGE_Handle;

UI_MESSAGE_Handle*
ui_message_new(MESSENGER_Application *app,
	       bool sent);

void
ui_message_delete(UI_MESSAGE_Handle *handle);

#endif /* UI_MESSAGE_H_ */
