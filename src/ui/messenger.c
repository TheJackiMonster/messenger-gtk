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

#include <gtk-3.0/gdk/gdkkeys.h>

#include "message.h"
#include "new_platform.h"
#include "../application.h"

static void
handle_flap_via_button_click(UNUSED GtkButton* button,
			     gpointer user_data)
{
  HdyFlap* flap = HDY_FLAP(user_data);

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }
}

static void
handle_account_details_button_click(UNUSED GtkButton* button,
				    gpointer user_data)
{
  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  GtkRevealer *revealer = handle->account_details_revealer;
  GtkImage *symbol = handle->account_details_symbol;

  gboolean old_state = gtk_revealer_get_reveal_child(revealer);

  gtk_revealer_set_reveal_child(revealer, !old_state);

  gtk_image_set_from_icon_name(
      symbol,
      old_state?
      "go-down-symbolic" :
      "go-up-symbolic",
      GTK_ICON_SIZE_BUTTON
  );
}

static void
handle_new_platform_button_click(UNUSED GtkButton* button,
				 gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_new_platform_dialog_init(app, &(app->ui.new_platform));

  gtk_widget_show(GTK_WIDGET(app->ui.new_platform.platform_dialog));
}

static void
handle_chats_listbox_row_activated(UNUSED GtkListBox* listbox,
				   UNUSED GtkListBoxRow* row,
				   gpointer user_data)
{
  HdyLeaflet* leaflet = HDY_LEAFLET(user_data);

  GList* children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if ((children) && (children->next)) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->next->data));
  }
}

static void
handle_back_button_click(UNUSED GtkButton* button,
			 gpointer user_data)
{
  HdyLeaflet* leaflet = HDY_LEAFLET(user_data);

  GList* children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if (children) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->data));
  }
}

static void
handle_send_text_buffer_changed(GtkTextBuffer *buffer,
				gpointer user_data)
{
  GtkImage *symbol = GTK_IMAGE(user_data);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  gtk_image_set_from_icon_name(
      symbol,
      0 < g_utf8_strlen(text, 1)?
      "mail-send-symbolic" :
      "audio-input-microphone-symbolic",
      GTK_ICON_SIZE_BUTTON
  );
}

static void
handle_send_record_button_click(GtkButton *button,
				gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(
      app->ui.messenger.send_text_view
  );

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 < g_utf8_strlen(text, 1))
  {
    struct GNUNET_CHAT_Context *context = g_hash_table_lookup(
	app->ui.bindings, button
    );

    if (context)
      GNUNET_CHAT_context_send_text(context, text);
  }
  else
  {
    // TODO: record audio and attach as file?
  }

  gtk_text_buffer_delete(buffer, &start, &end);
}

static gboolean
handle_send_text_key_press (GtkWidget *widget,
                            GdkEventKey *event,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((app->ui.mobile) ||
      (event->state & GDK_SHIFT_MASK) ||
      ((event->keyval != GDK_KEY_Return) &&
       (event->keyval != GDK_KEY_KP_Enter)))
    return FALSE;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (widget));

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 == g_utf8_strlen(text, 1))
    return FALSE;

  struct GNUNET_CHAT_Context *context = g_hash_table_lookup(
    app->ui.bindings, widget
  );

  if (context)
    GNUNET_CHAT_context_send_text(context, text);

  gtk_text_buffer_delete(buffer, &start, &end);
  return TRUE;
}

static void
handle_main_window_destroy(UNUSED GtkWidget *window,
			   gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  application_exit(app, MESSENGER_QUIT);
}

void
ui_messenger_init(MESSENGER_Application *app,
		  UI_MESSENGER_Handle *handle)
{
  GtkBuilder* builder = gtk_builder_new_from_file("resources/ui/messenger.ui");

  handle->main_window = GTK_APPLICATION_WINDOW(
      gtk_builder_get_object(builder, "main_window")
  );

  gtk_application_add_window(
      app->application,
      GTK_WINDOW(handle->main_window)
  );

  gtk_window_set_default_size(
      GTK_WINDOW(handle->main_window),
      1100, 700
  );

  handle->leaflet_chat = HDY_LEAFLET(
      gtk_builder_get_object(builder, "leaflet_chat")
  );

  handle->flap_user_details = HDY_FLAP(
      gtk_builder_get_object(builder, "flap_user_details")
  );

  handle->flap_chat_details = HDY_FLAP(
      gtk_builder_get_object(builder, "flap_chat_details")
  );

  handle->title_bar = HDY_HEADER_BAR(
      gtk_builder_get_object(builder, "title_bar")
  );

  g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->title_bar,
      "show-close-button",
      G_BINDING_INVERT_BOOLEAN
  );

  if (app->ui.mobile)
    g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->title_bar,
      "visible",
      G_BINDING_SYNC_CREATE |
      G_BINDING_INVERT_BOOLEAN
    );

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "back_button")
  );

  g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->back_button,
      "visible",
      G_BINDING_SYNC_CREATE
  );

  g_signal_connect(
      handle->back_button,
      "clicked",
      G_CALLBACK(handle_back_button_click),
      handle->leaflet_chat
  );

  handle->profile_avatar = HDY_AVATAR(
      gtk_builder_get_object(builder, "profile_avatar")
  );

  handle->profile_label = GTK_LABEL(
      gtk_builder_get_object(builder, "profile_label")
  );

  handle->hide_user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "hide_user_details_button")
  );

  g_signal_connect(
      handle->hide_user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  handle->favourites_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "favourites_button")
  );

  handle->account_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "account_details_button")
  );

  handle->account_details_symbol = GTK_IMAGE(
      gtk_builder_get_object(builder, "account_details_symbol")
  );

  handle->account_details_revealer = GTK_REVEALER(
      gtk_builder_get_object(builder, "account_details_revealer")
  );

  g_signal_connect(
      handle->account_details_button,
      "clicked",
      G_CALLBACK(handle_account_details_button_click),
      handle
  );

  handle->accounts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(builder, "accounts_listbox")
  );

  handle->new_contact_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "new_contact_button")
  );

  handle->new_group_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "new_group_button")
  );

  handle->new_platform_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "new_platform_button")
  );

  g_signal_connect(
      handle->new_platform_button,
      "clicked",
      G_CALLBACK(handle_new_platform_button_click),
      app
  );

  handle->contacts_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "contacts_button")
  );

  handle->settings_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "settings_button")
  );

  handle->user_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "user_details_button")
  );

  g_signal_connect(
      handle->user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  handle->chats_search = GTK_SEARCH_ENTRY(
      gtk_builder_get_object(builder, "chats_search")
  );

  handle->chats_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(builder, "chats_listbox")
  );

  g_signal_connect(
      handle->chats_listbox,
      "row-activated",
      G_CALLBACK(handle_chats_listbox_row_activated),
      handle->leaflet_chat
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

  g_signal_connect(
      handle->chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
  );

  handle->hide_chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "hide_chat_details_button")
  );

  g_signal_connect(
      handle->hide_chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
  );

  handle->messages_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(builder, "messages_listbox")
  );

  handle->attach_file_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "attach_file_button")
  );

  handle->send_text_view = GTK_TEXT_VIEW(
      gtk_builder_get_object(builder, "send_text_view")
  );

  handle->emoji_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "emoji_button")
  );

  handle->send_record_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "send_record_button")
  );

  handle->send_record_symbol = GTK_IMAGE(
      gtk_builder_get_object(builder, "send_record_symbol")
  );

  GtkTextBuffer *send_text_buffer = gtk_text_view_get_buffer(
      handle->send_text_view
  );

  g_signal_connect(
      send_text_buffer,
      "changed",
      G_CALLBACK(handle_send_text_buffer_changed),
      handle->send_record_symbol
  );

  g_signal_connect(
      handle->send_record_button,
      "clicked",
      G_CALLBACK(handle_send_record_button_click),
      app
  );

  g_signal_connect(
      handle->send_text_view,
      "key-press-event",
      G_CALLBACK(handle_send_text_key_press),
      app
  );

  gtk_widget_show(GTK_WIDGET(handle->main_window));

  g_signal_connect(
      handle->main_window,
      "destroy",
      G_CALLBACK(handle_main_window_destroy),
      app
  );
}

void
ui_messenger_run(MESSENGER_Application *app)
{
  ui_messenger_init(app, &(app->ui.messenger));
}
