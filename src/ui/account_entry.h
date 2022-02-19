/*
   This file is part of GNUnet.
   Copyright (C) 2021--2022 GNUnet e.V.

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
 * @file ui/account_entry.h
 */

#ifndef UI_ACCOUNT_ENTRY_H_
#define UI_ACCOUNT_ENTRY_H_

#include "messenger.h"

typedef struct UI_ACCOUNT_ENTRY_Handle
{
  GtkBuilder *builder;

  GtkWidget *entry_box;

  HdyAvatar *entry_avatar;
  GtkLabel *entry_label;
} UI_ACCOUNT_ENTRY_Handle;

UI_ACCOUNT_ENTRY_Handle*
ui_account_entry_new(MESSENGER_Application *app);

void
ui_account_entry_set_account(UI_ACCOUNT_ENTRY_Handle* handle,
			     const struct GNUNET_CHAT_Account *account);

void
ui_account_entry_set_contact(UI_ACCOUNT_ENTRY_Handle* handle,
			     const struct GNUNET_CHAT_Contact *contact);

void
ui_account_entry_delete(UI_ACCOUNT_ENTRY_Handle *handle);

#endif /* UI_ACCOUNT_ENTRY_H_ */
