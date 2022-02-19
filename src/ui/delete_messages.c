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
 * @file ui/delete_messages.c
 */

#include "delete_messages.h"

#include "../application.h"

static void
handle_cancel_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_confirm_button_click(UNUSED GtkButton *button,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->settings.hide_delete_dialog = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(app->ui.delete_messages.hide_checkbox)
  );

  GtkTreeModel *model = gtk_combo_box_get_model(
      app->ui.delete_messages.delay_combobox
  );

  gulong delay = 0;

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(app->ui.delete_messages.delay_combobox,
				    &iter))
    gtk_tree_model_get(model, &iter, 1, &delay, -1);

  if (app->ui.delete_messages.callback)
    app->ui.delete_messages.callback(
	app,
	app->ui.delete_messages.selected,
	delay
    );

  gtk_window_close(GTK_WINDOW(app->ui.delete_messages.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_delete_messages_dialog_cleanup(&(app->ui.delete_messages));
}

void
ui_delete_messages_dialog_init(MESSENGER_Application *app,
			       UI_DELETE_MESSAGES_Handle *handle)
{
  handle->selected = NULL;
  handle->callback = NULL;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/delete_messages.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "delete_messages_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->delay_store = GTK_LIST_STORE(
      gtk_builder_get_object(handle->builder, "delay_store")
  );

  handle->delay_combobox = GTK_COMBO_BOX(
      gtk_builder_get_object(handle->builder, "delay_combobox")
  );

  handle->hide_checkbox = GTK_CHECK_BUTTON(
      gtk_builder_get_object(handle->builder, "hide_checkbox")
  );

  handle->cancel_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
      handle->cancel_button,
      "clicked",
      G_CALLBACK(handle_cancel_button_click),
      handle->dialog
  );

  handle->confirm_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "confirm_button")
  );

  g_signal_connect(
      handle->confirm_button,
      "clicked",
      G_CALLBACK(handle_confirm_button_click),
      app
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      app
  );
}

void
ui_delete_messages_dialog_link(UI_DELETE_MESSAGES_Handle *handle,
			       UI_DELETE_MESSAGES_Callback callback,
			       GList *selected)
{
  handle->selected = selected;
  handle->callback = callback;
}

void
ui_delete_messages_dialog_cleanup(UI_DELETE_MESSAGES_Handle *handle)
{
  g_object_unref(handle->builder);
}
