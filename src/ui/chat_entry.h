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
 * @file ui/chat_entry.h
 */

#ifndef UI_CHAT_ENTRY_H_
#define UI_CHAT_ENTRY_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef struct UI_CHAT_ENTRY_Handle
{
  GtkWidget* entry_box;

  HdyAvatar* avatar;

  GtkLabel* title;
  GtkLabel* timestamp;

  GtkLabel* text;
} UI_CHAT_ENTRY_Handle;

UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(void);

#endif /* UI_CHAT_ENTRY_H_ */