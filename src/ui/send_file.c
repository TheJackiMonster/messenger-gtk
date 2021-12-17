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
 * @file ui/send_file.c
 */

#include "send_file.h"

#include "../application.h"

static void
handle_cancel_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_send_button_click(GtkButton *button,
			 gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_hash_table_lookup(app->ui.bindings, button)
  );

  if (!text_view)
    return;

  gchar *filename = gtk_file_chooser_get_filename(
      GTK_FILE_CHOOSER(app->ui.send_file.file_chooser_button)
  );

  if (!filename)
    return;

  struct GNUNET_CHAT_Context *context = g_hash_table_lookup(
      app->ui.bindings, text_view
  );

  if (context)
    GNUNET_CHAT_context_send_file(context, filename, NULL, NULL); // TODO: callbacks!

  g_free(filename);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_send_file_dialog_cleanup((UI_SEND_FILE_Handle*) user_data);
}

void
ui_send_file_dialog_init(MESSENGER_Application *app,
			 UI_SEND_FILE_Handle *handle)
{
  handle->builder = gtk_builder_new_from_file("resources/ui/send_file.ui");

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "send_file_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      _("Send File")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->file_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(handle->builder, "file_drawing_area")
  );

  handle->file_chooser_button = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(handle->builder, "file_chooser_button")
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

  handle->send_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "send_button")
  );

  g_signal_connect(
      handle->send_button,
      "clicked",
      G_CALLBACK(handle_send_button_click),
      app
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );
}

void
ui_send_file_dialog_update(UI_SEND_FILE_Handle *handle,
			   const gchar *filename)
{
  if ((!filename) || (!gtk_file_chooser_set_filename(
      GTK_FILE_CHOOSER(handle->file_chooser_button),
      filename)))
    return;

  // TODO: update preview
}

void
ui_send_file_dialog_cleanup(UI_SEND_FILE_Handle *handle)
{
  g_object_unref(handle->builder);
}
