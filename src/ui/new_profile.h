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
 * @file ui/new_profile.h
 */

#ifndef UI_NEW_PROFILE_H_
#define UI_NEW_PROFILE_H_

#include "messenger.h"

typedef struct UI_NEW_PROFILE_Handle
{
  GtkBuilder *builder;
  GtkDialog *dialog;

  HdyAvatar *profile_avatar;
  GtkFileChooserButton *profile_avatar_file;

  GtkEntry *profile_entry;

  GtkButton *cancel_button;
  GtkButton *confirm_button;
} UI_NEW_PROFILE_Handle;

void
ui_new_profile_dialog_init(MESSENGER_Application *app,
			   UI_NEW_PROFILE_Handle *handle);

void
ui_new_profile_dialog_cleanup(UI_NEW_PROFILE_Handle *handle);

#endif /* UI_NEW_PROFILE_H_ */
