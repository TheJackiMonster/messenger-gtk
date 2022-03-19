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
 * @file ui/settings.h
 */

#ifndef UI_SETTINGS_H_
#define UI_SETTINGS_H_

#include "messenger.h"

typedef struct UI_SETTINGS_Handle
{
  GtkBuilder *builder;
  HdyPreferencesWindow *dialog;

  GtkSwitch *enable_notifications_switch;

  GtkSwitch *read_receipts_switch;
  GtkSwitch *whispering_switch;

  GtkComboBox *auto_delete_combo_box;

  GtkSwitch *auto_accept_invitations_switch;
  GtkComboBox *delete_invitations_combo_box;
  GtkButton *delete_invitations_button;

  GtkSwitch *auto_accept_files_switch;
  GtkFileChooserButton *download_folder_button;
  GtkComboBox *delete_files_combo_box;
  GtkButton *delete_files_button;

  GtkComboBox *leave_chats_combo_box;
  GtkButton *leave_chats_button;
} UI_SETTINGS_Handle;

void
ui_settings_dialog_init(MESSENGER_Application *app,
			UI_SETTINGS_Handle *handle);

void
ui_settings_dialog_cleanup(UI_SETTINGS_Handle *handle);

#endif /* UI_SETTINGS_H_ */
