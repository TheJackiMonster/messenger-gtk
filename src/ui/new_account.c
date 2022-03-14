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
 * @file ui/new_account.c
 */

#include "new_account.h"

#include "../application.h"

static gboolean
_show_messenger_main_window(gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  gtk_widget_show(GTK_WIDGET(app->ui.messenger.main_window));
  return FALSE;
}

static void
_open_new_account(GtkEntry *entry, MESSENGER_Application *app)
{
  const gchar *name = gtk_entry_get_text(entry);

  if (GNUNET_OK != GNUNET_CHAT_account_create(app->chat.messenger.handle, name))
    return;

  gtk_list_box_unselect_all(app->ui.messenger.accounts_listbox);

  if (app->chat.identity)
    GNUNET_free(app->chat.identity);

  app->chat.identity = GNUNET_strdup(name);

  if (!gtk_widget_is_visible(GTK_WIDGET(app->ui.messenger.main_window)))
    app->ui.new_account.show_queued = g_idle_add(
      G_SOURCE_FUNC(_show_messenger_main_window), app
    );
}

static void
handle_account_entry_changed(GtkEditable *editable,
			     gpointer user_data)
{
  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  hdy_avatar_set_text(avatar, gtk_entry_get_text(entry));
}

static void
handle_account_entry_activate(UNUSED GtkEntry *entry,
			      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_account(app->ui.new_account.account_entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_account.dialog));
}

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

  _open_new_account(app->ui.new_account.account_entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_account.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_account_dialog_cleanup(&(app->ui.new_account));

  if ((!(app->ui.new_account.show_queued)) &&
      (!gtk_widget_is_visible(GTK_WIDGET(app->ui.messenger.main_window))))
    gtk_widget_destroy(GTK_WIDGET(app->ui.messenger.main_window));
}

void
ui_new_account_dialog_init(MESSENGER_Application *app,
			   UI_NEW_ACCOUNT_Handle *handle)
{
  handle->show_queued = 0;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/new_account.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "new_account_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->account_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "account_avatar")
  );

  handle->account_avatar_file = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(handle->builder, "account_avatar_file")
  );

  handle->account_entry = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "account_entry")
  );

  g_signal_connect(
      handle->account_entry,
      "changed",
      G_CALLBACK(handle_account_entry_changed),
      handle->account_avatar
  );

  g_signal_connect(
      handle->account_entry,
      "activate",
      G_CALLBACK(handle_account_entry_activate),
      app
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
ui_new_account_dialog_cleanup(UI_NEW_ACCOUNT_Handle *handle)
{
  g_object_unref(handle->builder);

  guint show = handle->show_queued;
  memset(handle, 0, sizeof(*handle));
  handle->show_queued = show;
}
