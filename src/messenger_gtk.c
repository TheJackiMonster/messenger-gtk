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
 * @file messenger_gtk.c
 */

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

#include <gnunet/gnunet_chat_lib.h>

G_MODULE_EXPORT void handle_user_details_button_click(GtkButton* button,
						      gpointer user_data)
{
  HdyFlap* flap = HDY_FLAP(user_data);

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }
}

G_MODULE_EXPORT void handle_account_details_button_click(GtkButton* button,
							 gpointer user_data)
{
  GtkRevealer* revealer = GTK_REVEALER(user_data);

  if (TRUE == gtk_revealer_get_reveal_child(revealer)) {
    gtk_revealer_set_reveal_child(revealer, FALSE);
  } else {
    gtk_revealer_set_reveal_child(revealer, TRUE);
  }
}

int main(int argc, char** argv) {
  gtk_init(&argc, &argv);

  GtkBuilder* builder = gtk_builder_new();
  gtk_builder_add_from_file(
      builder,
      "resources/ui/messenger.ui",
      NULL
  );

  GtkApplicationWindow* window = GTK_APPLICATION_WINDOW(
      gtk_builder_get_object(builder, "main_window")
  );

  GdkScreen* screen = gdk_screen_get_default();

  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(
      provider,
      "resources/css/style.css",
      NULL
  );

  gtk_style_context_add_provider_for_screen(
      screen,
      GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );

  HdyHeaderBar* title_bar = HDY_HEADER_BAR(
      gtk_builder_get_object(builder, "title_bar")
  );

  GtkButton* user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "user_details_button")
  );

  GtkButton* hide_user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "hide_user_details_button")
  );

  HdyFlap* flap_user_details = HDY_FLAP(
      gtk_builder_get_object(builder, "flap_user_details")
  );

  g_signal_connect(
      user_details_button,
      "clicked",
      G_CALLBACK(handle_user_details_button_click),
      flap_user_details
  );

  g_signal_connect(
      hide_user_details_button,
      "clicked",
      G_CALLBACK(handle_user_details_button_click),
      flap_user_details
  );

  GtkButton* account_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "account_details_button")
  );

  GtkRevealer* account_details_revealer = GTK_REVEALER(
      gtk_builder_get_object(builder, "account_details_revealer")
  );

  g_signal_connect(
      account_details_button,
      "clicked",
      G_CALLBACK(handle_account_details_button_click),
      account_details_revealer
  );

  gtk_widget_show(GTK_WIDGET(window));

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_main();
  return 0;
}
