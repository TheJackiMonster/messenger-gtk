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
 * @file ui/contact_info.h
 */

#ifndef UI_CONTACT_INFO_H_
#define UI_CONTACT_INFO_H_

#include "messenger.h"

#include <cairo/cairo.h>
#include <gdk/gdkpixbuf.h>
#include <qrencode.h>

typedef struct UI_CONTACT_INFO_Handle
{
  MESSENGER_Application *app;

  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkStack *contact_info_stack;

  GtkWidget *details_box;
  HdyAvatar *contact_avatar;
  GtkEntry *contact_name_entry;

  GtkButton *contact_edit_button;
  GtkImage *contact_edit_symbol;

  GtkButton *reveal_identity_button;
  GtkButton *open_chat_button;

  GtkDrawingArea *id_drawing_area;
  gulong id_draw_signal;

  GtkButton *back_button;
  GtkButton *close_button;

  QRcode *qr;
} UI_CONTACT_INFO_Handle;

void
ui_contact_info_dialog_init(MESSENGER_Application *app,
			    UI_CONTACT_INFO_Handle *handle);

void
ui_contact_info_dialog_update(UI_CONTACT_INFO_Handle *handle,
			      struct GNUNET_CHAT_Contact *contact,
			      gboolean reveal);

void
ui_contact_info_dialog_cleanup(UI_CONTACT_INFO_Handle *handle);

#endif /* UI_CONTACT_INFO_H_ */
