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
 * @file ui/new_platform.h
 */

#include "new_platform.h"

#include "../application.h"

static void
handle_platform_entry_changed(GtkEditable *editable,
			      gpointer user_data)
{
  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  hdy_avatar_set_text(avatar, gtk_entry_get_text(entry));
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

  const gchar *topic = gtk_entry_get_text(app->ui.new_platform.platform_entry);

  GString *topic_string = g_string_new(topic);

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_group_create(
      app->chat.messenger.handle,
      topic_string->str
  );

  g_string_prepend_c(topic_string, '#');

  GNUNET_CHAT_group_set_name(group, topic_string->str);

  g_string_free(topic_string, TRUE);

  gtk_window_close(GTK_WINDOW(app->ui.new_platform.platform_dialog));
}

void
ui_new_platform_dialog_init(MESSENGER_Application *app,
			    UI_NEW_PLATFORM_Handle *handle)
{
  GtkBuilder* builder = gtk_builder_new_from_file("resources/ui/new_platform.ui");

  handle->platform_dialog = GTK_DIALOG(
      gtk_builder_get_object(builder, "platform_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->platform_dialog),
      "New Platform"
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->platform_dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  gtk_window_set_modal(GTK_WINDOW(handle->platform_dialog), TRUE);

  handle->platform_avatar = HDY_AVATAR(
      gtk_builder_get_object(builder, "platform_avatar")
  );

  handle->platform_avatar_file = GTK_FILE_CHOOSER_BUTTON(
      gtk_builder_get_object(builder, "platform_avatar_file")
  );

  handle->platform_entry = GTK_ENTRY(
      gtk_builder_get_object(builder, "platform_entry")
  );

  g_signal_connect(
      handle->platform_entry,
      "changed",
      G_CALLBACK(handle_platform_entry_changed),
      handle->platform_avatar
  );

  handle->cancel_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "cancel_button")
  );

  g_signal_connect(
      handle->cancel_button,
      "clicked",
      G_CALLBACK(handle_cancel_button_click),
      handle->platform_dialog
  );

  handle->confirm_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "confirm_button")
  );

  g_signal_connect(
      handle->confirm_button,
      "clicked",
      G_CALLBACK(handle_confirm_button_click),
      app
  );
}
