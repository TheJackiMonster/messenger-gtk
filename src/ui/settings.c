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
 * @file ui/settings.c
 */

#include "settings.h"

#include "../application.h"

static gboolean
handle_general_switch_state(UNUSED GtkSwitch *widget,
			    gboolean state,
			    gpointer user_data)
{
  gboolean *setting = (gboolean*) user_data;
  *setting = state;
  return FALSE;
}

static gboolean
handle_inverted_switch_state(GtkSwitch *widget,
			    gboolean state,
			    gpointer user_data)
{
  return handle_general_switch_state(widget, !state, user_data);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_settings_dialog_cleanup((UI_SETTINGS_Handle*) user_data);
}

void
ui_settings_dialog_init(MESSENGER_Application *app,
			UI_SETTINGS_Handle *handle)
{
  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/settings.ui")
  );

  handle->dialog = HDY_PREFERENCES_WINDOW(
      gtk_builder_get_object(handle->builder, "settings_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->enable_notifications_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "enable_notifications_switch")
  );

  gtk_switch_set_active(
      handle->enable_notifications_switch,
      !(app->settings.disable_notifications)
  );

  g_signal_connect(
      handle->enable_notifications_switch,
      "state-set",
      G_CALLBACK(handle_inverted_switch_state),
      &(app->settings.disable_notifications)
  );

  handle->read_receipts_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "read_receipts_switch")
  );

  gtk_switch_set_active(
      handle->read_receipts_switch,
      app->settings.send_read_receipts
  );

  g_signal_connect(
      handle->read_receipts_switch,
      "state-set",
      G_CALLBACK(handle_general_switch_state),
      &(app->settings.send_read_receipts)
  );

  handle->whispering_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "whispering_switch")
  );

  gtk_switch_set_active(
      handle->whispering_switch,
      app->settings.show_whispering
  );

  g_signal_connect(
      handle->whispering_switch,
      "state-set",
      G_CALLBACK(handle_general_switch_state),
      &(app->settings.show_whispering)
  );

  handle->auto_delete_combo_box = GTK_COMBO_BOX(
      gtk_builder_get_object(handle->builder, "auto_delete_combo_box")
  );

  handle->auto_accept_invitations_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "auto_accept_invitations_switch")
  );

  gtk_switch_set_active(
      handle->auto_accept_invitations_switch,
      app->settings.accept_all_invitations
  );

  g_signal_connect(
      handle->auto_accept_invitations_switch,
      "state-set",
      G_CALLBACK(handle_general_switch_state),
      &(app->settings.accept_all_invitations)
  );

  handle->delete_invitations_combo_box = GTK_COMBO_BOX(
      gtk_builder_get_object(handle->builder, "delete_invitations_combo_box")
  );

  handle->delete_invitations_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "delete_invitations_button")
  );

  handle->auto_accept_files_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "auto_accept_files_switch")
  );

  gtk_switch_set_active(
      handle->auto_accept_files_switch,
      app->settings.accept_all_files
  );

  g_signal_connect(
      handle->auto_accept_files_switch,
      "state-set",
      G_CALLBACK(handle_general_switch_state),
      &(app->settings.accept_all_files)
  );

  handle->download_folder_button = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(handle->builder, "download_folder_button")
  );

  handle->delete_files_combo_box = GTK_COMBO_BOX(
      gtk_builder_get_object(handle->builder, "delete_files_combo_box")
  );

  handle->delete_files_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "delete_files_button")
  );

  handle->leave_chats_combo_box = GTK_COMBO_BOX(
      gtk_builder_get_object(handle->builder, "leave_chats_combo_box")
  );

  handle->leave_chats_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "leave_chats_button")
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );
}

void
ui_settings_dialog_cleanup(UI_SETTINGS_Handle *handle)
{
  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
