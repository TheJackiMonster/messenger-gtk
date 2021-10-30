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
 * @file ui/messenger.c
 */

#include "messenger.h"

void handle_flap_via_button_click(UI_UNUSED GtkButton* button,
				  gpointer user_data)
{
  HdyFlap* flap = HDY_FLAP(user_data);

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }
}

void handle_account_details_button_click(UI_UNUSED GtkButton* button,
					 gpointer user_data)
{
  GtkRevealer* revealer = GTK_REVEALER(user_data);

  if (TRUE == gtk_revealer_get_reveal_child(revealer)) {
    gtk_revealer_set_reveal_child(revealer, FALSE);
  } else {
    gtk_revealer_set_reveal_child(revealer, TRUE);
  }
}

void handle_chats_listbox_row_activated(UI_UNUSED GtkListBox* listbox,
					UI_UNUSED GtkListBoxRow* row,
					gpointer user_data)
{
  HdyLeaflet* leaflet = HDY_LEAFLET(user_data);

  GList* children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if ((children) && (children->next)) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->next->data));
  }
}

void handle_back_button_click(UI_UNUSED GtkButton* button,
			      gpointer user_data)
{
  HdyLeaflet* leaflet = HDY_LEAFLET(user_data);

  GList* children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if (children) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->data));
  }
}

void
ui_messenger_init(struct UI_MESSENGER_Handle *handle)
{
  GtkBuilder* builder = gtk_builder_new();
  gtk_builder_add_from_file(
      builder,
      "resources/ui/messenger.ui",
      NULL
  );

  handle->main_window = GTK_APPLICATION_WINDOW(
      gtk_builder_get_object(builder, "main_window")
  );

  handle->profile_avatar = HDY_AVATAR(
      gtk_builder_get_object(builder, "profile_avatar")
  );

  handle->profile_label = GTK_LABEL(
      gtk_builder_get_object(builder, "profile_label")
  );

  handle->title_bar = HDY_HEADER_BAR(
      gtk_builder_get_object(builder, "title_bar")
  );

  handle->leaflet_chat = HDY_LEAFLET(
      gtk_builder_get_object(builder, "leaflet_chat")
  );

  hdy_leaflet_set_homogeneous(handle->leaflet_chat, FALSE, GTK_ORIENTATION_HORIZONTAL, FALSE);

  handle->chats_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(builder, "chats_listbox")
  );

  g_signal_connect(
      handle->chats_listbox,
      "row-activated",
      G_CALLBACK(handle_chats_listbox_row_activated),
      handle->leaflet_chat
  );

  handle->user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "user_details_button")
  );

  handle->hide_user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "hide_user_details_button")
  );

  handle->flap_user_details = HDY_FLAP(
      gtk_builder_get_object(builder, "flap_user_details")
  );

  g_signal_connect(
      handle->user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  g_signal_connect(
      handle->hide_user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  handle->account_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "account_details_button")
  );

  handle->account_details_revealer = GTK_REVEALER(
      gtk_builder_get_object(builder, "account_details_revealer")
  );

  g_signal_connect(
      handle->account_details_button,
      "clicked",
      G_CALLBACK(handle_account_details_button_click),
      handle->account_details_revealer
  );

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "back_button")
  );

  g_signal_connect(
      handle->back_button,
      "clicked",
      G_CALLBACK(handle_back_button_click),
      handle->leaflet_chat
  );

  g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->back_button,
      "visible",
      G_BINDING_SYNC_CREATE
  );

  g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->title_bar,
      "show-close-button",
      G_BINDING_INVERT_BOOLEAN
  );

  handle->flap_chat_details = HDY_FLAP(
      gtk_builder_get_object(builder, "flap_chat_details")
  );

  handle->chat_title = GTK_LABEL(
      gtk_builder_get_object(builder, "chat_title")
  );

  handle->chat_subtitle = GTK_LABEL(
      gtk_builder_get_object(builder, "chat_subtitle")
  );

  handle->chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "chat_details_button")
  );

  handle->hide_chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "hide_chat_details_button")
  );

  g_signal_connect(
      handle->chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
  );

  g_signal_connect(
      handle->hide_chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
  );

  gtk_widget_show(GTK_WIDGET(handle->main_window));

  g_signal_connect(handle->main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}
