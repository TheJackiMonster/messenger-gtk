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
 * @file ui/messenger.h
 */

#ifndef UI_MESSENGER_H_
#define UI_MESSENGER_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef struct UI_MESSENGER_Handle
{
  GList *chat_entries;

  GtkBuilder *builder;
  GtkApplicationWindow *main_window;

  HdyLeaflet *leaflet_chat;
  HdyFlap *flap_user_details;
  HdyFlap *flap_chat_details;

  HdyHeaderBar *title_bar;

  HdyAvatar *profile_avatar;
  GtkLabel *profile_label;
  GtkLabel *profile_key_label;

  GtkButton *hide_user_details_button;
  GtkButton *favourites_button;
  GtkButton *account_details_button;
  GtkImage *account_details_symbol;

  GtkRevealer *account_details_revealer;
  GtkListBox *accounts_listbox;
  GtkListBoxRow *add_account_listbox_row;

  GtkButton *new_contact_button;
  GtkButton *new_group_button;
  GtkButton *new_platform_button;
  GtkButton *contacts_button;
  GtkButton *settings_button;

  GtkButton *user_details_button;
  GtkSearchEntry *chats_search;
  GtkListBox *chats_listbox;

  GtkStack *chats_stack;

  GtkButton *hide_chat_details_button;
} UI_MESSENGER_Handle;

void
ui_messenger_init(MESSENGER_Application *app,
		  UI_MESSENGER_Handle *handle);

void
ui_messenger_cleanup(UI_MESSENGER_Handle *handle);

#endif /* UI_MESSENGER_H_ */
