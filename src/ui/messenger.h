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
  GtkApplicationWindow *main_window;

  HdyLeaflet *leaflet_chat;
  HdyFlap *flap_user_details;
  HdyFlap *flap_chat_details;

  HdyHeaderBar *title_bar;
  GtkButton *back_button;

  HdyAvatar *profile_avatar;
  GtkLabel *profile_label;

  GtkButton *hide_user_details_button;
  GtkButton *favourites_button;
  GtkButton *account_details_button;

  GtkRevealer *account_details_revealer;
  GtkListBox *accounts_listbox;

  GtkButton *new_group_button;
  GtkButton *new_platform_button;
  GtkButton *contacts_button;
  GtkButton *calls_button;
  GtkButton *settings_button;
  GtkButton *night_mode_button;

  GtkButton *user_details_button;
  GtkSearchEntry *chats_search;
  GtkListBox *chats_listbox;

  GtkLabel *chat_title;
  GtkLabel *chat_subtitle;
  GtkButton *chat_details_button;

  GtkButton *hide_chat_details_button;
} UI_MESSENGER_Handle;

void
ui_messenger_init(MESSENGER_Application *app,
		  UI_MESSENGER_Handle *handle);

void
ui_messenger_run(MESSENGER_Application *app);

#endif /* UI_MESSENGER_H_ */
