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
 * @file ui/chat.h
 */

#ifndef UI_CHAT_H_
#define UI_CHAT_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>

#include <gnunet/gnunet_chat_lib.h>

typedef struct MESSENGER_Application MESSENGER_Application;
typedef struct UI_PICKER_Handle UI_PICKER_Handle;

typedef struct UI_CHAT_Handle
{
  GtkBuilder *builder;
  GtkWidget *chat_box;

  GtkButton *back_button;

  HdyFlap *flap_chat_details;

  GtkLabel *chat_title;
  GtkLabel *chat_subtitle;
  GtkButton *chat_details_button;

  GtkLabel *chat_details_label;
  GtkButton *hide_chat_details_button;
  GtkBox *chat_details_contacts_box;

  GtkListBox *chat_contacts_listbox;
  GtkListBox *messages_listbox;

  GtkButton *attach_file_button;
  GtkTextView *send_text_view;
  GtkButton *emoji_button;
  GtkButton *send_record_button;
  GtkImage *send_record_symbol;

  GtkRevealer *picker_revealer;

  UI_PICKER_Handle *picker;
} UI_CHAT_Handle;

UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app);

void
ui_chat_update(UI_CHAT_Handle *handle,
	       const struct GNUNET_CHAT_Context* context);

void
ui_chat_delete(UI_CHAT_Handle *handle);

#endif /* UI_CHAT_H_ */
