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
 * @file ui/new_profile.c
 */

#include "new_profile.h"

#include "../application.h"

static void
_open_new_profile(GtkEntry *entry, MESSENGER_Application *app)
{
  const gchar *name = gtk_entry_get_text(entry);

  if (GNUNET_OK != GNUNET_CHAT_account_create(app->chat.messenger.handle, name))
    return;

  gtk_list_box_unselect_all(app->ui.messenger.accounts_listbox);

  if (app->chat.identity)
    GNUNET_free(app->chat.identity);

  app->chat.identity = GNUNET_strdup(name);
}

static void
handle_profile_entry_changed(GtkEditable *editable,
			      gpointer user_data)
{
  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  hdy_avatar_set_text(avatar, gtk_entry_get_text(entry));
}

static void
handle_profile_entry_activate(UNUSED GtkEntry *entry,
			       gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_profile(app->ui.new_profile.profile_entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_profile.dialog));
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

  _open_new_profile(app->ui.new_profile.profile_entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_profile.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_new_profile_dialog_cleanup((UI_NEW_PROFILE_Handle*) user_data);
}

void
ui_new_profile_dialog_init(MESSENGER_Application *app,
			   UI_NEW_PROFILE_Handle *handle)
{
  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/new_profile.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "new_profile_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      _("New Profile")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->profile_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "profile_avatar")
  );

  handle->profile_avatar_file = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(handle->builder, "profile_avatar_file")
  );

  handle->profile_entry = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "profile_entry")
  );

  g_signal_connect(
      handle->profile_entry,
      "changed",
      G_CALLBACK(handle_profile_entry_changed),
      handle->profile_avatar
  );

  g_signal_connect(
      handle->profile_entry,
      "activate",
      G_CALLBACK(handle_profile_entry_activate),
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
      handle
  );
}

void
ui_new_profile_dialog_cleanup(UI_NEW_PROFILE_Handle *handle)
{
  g_object_unref(handle->builder);
}
