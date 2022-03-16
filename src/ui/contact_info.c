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
 * @file ui/contact_info.c
 */

#include "contact_info.h"

#include "../application.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
			  gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_contact_info_dialog_cleanup((UI_CONTACT_INFO_Handle*) user_data);
}

void
ui_contact_info_dialog_init(MESSENGER_Application *app,
			    UI_CONTACT_INFO_Handle *handle)
{
  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/contact_info.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "contact_info_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->contact_info_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "contact_info_stack")
  );

  handle->details_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "details_box")
  );

  handle->contact_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "contact_avatar")
  );

  handle->contact_name = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "contact_name")
  );

  handle->reveal_identity_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "reveal_identity_button")
  );

  handle->open_chat_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "open_chat_button")
  );

  handle->id_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(handle->builder, "id_drawing_area")
  );

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "back_button")
  );

  handle->close_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
      handle->close_button,
      "clicked",
      G_CALLBACK(handle_close_button_click),
      handle->dialog
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );
}

void
ui_contact_info_dialog_update(UI_CONTACT_INFO_Handle *handle,
			      struct GNUNET_CHAT_Contact *contact)
{
  // TODO
}

void
ui_contact_info_dialog_cleanup(UI_CONTACT_INFO_Handle *handle)
{
  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
