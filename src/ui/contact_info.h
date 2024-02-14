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
  GtkStack *block_stack;
  GtkButton *block_button;
  GtkButton *unblock_button;
  GtkButton *open_chat_button;

  GtkWidget *identity_box;
  GtkLabel *name_label;

  GtkDrawingArea *id_drawing_area;
  gulong id_draw_signal;
  GtkEntry *id_entry;

  GtkButton *back_button;
  GtkButton *close_button;

  QRcode *qr;
} UI_CONTACT_INFO_Handle;

/**
 * Initializes a handle for the contact info dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Contact info dialog handle
 */
void
ui_contact_info_dialog_init(MESSENGER_Application *app,
                            UI_CONTACT_INFO_Handle *handle);

/**
 * Updates a given contact info dialog handle with
 * current data of a certain chat contact and whether
 * their identity should be visually revealed in the
 * dialog.
 *
 * @param handle Contact info dialog handle
 * @param contact Chat contact
 * @param reveal Flag to reveal QR code of identity key
 */
void
ui_contact_info_dialog_update(UI_CONTACT_INFO_Handle *handle,
                              struct GNUNET_CHAT_Contact *contact,
                              gboolean reveal);

/**
 * Cleans up the allocated resources and resets the
 * state of a given contact info dialog handle.
 *
 * @param handle Contact info dialog handle
 */
void
ui_contact_info_dialog_cleanup(UI_CONTACT_INFO_Handle *handle);

#endif /* UI_CONTACT_INFO_H_ */
