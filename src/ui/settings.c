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
