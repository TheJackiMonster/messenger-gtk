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
 * @file ui.h
 */

#ifndef UI_H_
#define UI_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

void
ui_label_set_text(GtkLabel *label, const char *text);

void
ui_entry_set_text(GtkEntry *entry, const char *text);

void
ui_avatar_set_text(HdyAvatar *avatar, const char *text);

#endif /* UI_H_ */
