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
 * @file ui/chat.c
 */

#include "chat.h"

#include "messenger.h"
#include "picker.h"
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
      0 < strlen(text)?
      "mail-send-symbolic" :
      "audio-input-microphone-symbolic",
      GTK_ICON_SIZE_BUTTON
  );
}

static gboolean
_send_text_from_view(MESSENGER_Application *app,
		     GtkTextView *text_view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 == strlen(text))
    return FALSE;

  struct GNUNET_CHAT_Context *context = g_hash_table_lookup(
    app->ui.bindings, text_view
  );

  if (context)
    GNUNET_CHAT_context_send_text(context, text);

  gtk_text_buffer_delete(buffer, &start, &end);
  return TRUE;
}

static void
handle_send_record_button_click(GtkButton *button,
				gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_hash_table_lookup(app->ui.bindings, button)
  );

  if (!_send_text_from_view(app, text_view))
  {
    // TODO: record audio and attach as file?
  }
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

  return _send_text_from_view(app, GTK_TEXT_VIEW(widget));
}

static void
handle_picker_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  GtkRevealer *revealer = GTK_REVEALER(user_data);
  gboolean reveal = !gtk_revealer_get_child_revealed(revealer);

  gtk_revealer_set_reveal_child(revealer, reveal);
}

UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app)
{
  UI_CHAT_Handle *handle = g_malloc(sizeof(UI_CHAT_Handle));
  UI_MESSENGER_Handle *messenger = &(app->ui.messenger);

  handle->builder = gtk_builder_new_from_file(
      "resources/ui/chat.ui"
  );

  handle->chat_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "chat_box")
  );

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "back_button")
  );

  g_object_bind_property(
      messenger->leaflet_chat,
      "folded",
      handle->back_button,
      "visible",
      G_BINDING_SYNC_CREATE
  );

  g_signal_connect(
      handle->back_button,
      "clicked",
      G_CALLBACK(handle_back_button_click),
      messenger->leaflet_chat
  );

  handle->chat_title = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_title")
  );

  handle->chat_subtitle = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_subtitle")
  );

  handle->chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "chat_details_button")
  );

  g_signal_connect(
      handle->chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      messenger->flap_chat_details
  );

  handle->messages_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "messages_listbox")
  );

  handle->attach_file_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "attach_file_button")
  );

  handle->send_text_view = GTK_TEXT_VIEW(
      gtk_builder_get_object(handle->builder, "send_text_view")
  );

  handle->emoji_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "emoji_button")
  );

  handle->send_record_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "send_record_button")
  );

  handle->send_record_symbol = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "send_record_symbol")
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

  g_hash_table_insert(
      app->ui.bindings,
      handle->send_record_button,
      handle->send_text_view
  );

  handle->picker_revealer = GTK_REVEALER(
      gtk_builder_get_object(handle->builder, "picker_revealer")
  );

  handle->picker = ui_picker_new(app, handle);

  gtk_container_add(
      GTK_CONTAINER(handle->picker_revealer),
      handle->picker->picker_box
  );

  g_signal_connect(
      handle->emoji_button,
      "clicked",
      G_CALLBACK(handle_picker_button_click),
      handle->picker_revealer
  );

  return handle;
}

void
ui_chat_delete(UI_CHAT_Handle *handle)
{
  ui_picker_delete(handle->picker);

  g_object_unref(handle->builder);

  g_free(handle);
}
