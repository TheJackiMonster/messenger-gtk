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
 * @file ui/picker.c
 */

#include "picker.h"

#include "../application.h"

#include <emoji.h>

static void
handle_emoji_button_click(GtkButton *button,
			  gpointer user_data)
{
  GtkTextView *text_view = GTK_TEXT_VIEW(user_data);
  GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);

  const gchar *label = gtk_button_get_label(button);

  if (label)
    gtk_text_buffer_insert_at_cursor(text_buffer, label, strlen(label));
}

static void
_add_emoji_buttons(GtkFlowBox *flow_box,
		   GtkTextView *text_view,
		   size_t characters_count,
		   const uint32_t *characters)
{
  for (size_t i = 0; i < characters_count; i++) {
    GString *string = g_string_new("");
    g_string_append_unichar(string, (gunichar) characters[i]);

    gchar *_text = g_locale_to_utf8(string->str, string->len, NULL, NULL, NULL);

    GtkButton *emoji_button = GTK_BUTTON(
      gtk_button_new_with_label(string->str)
    );

    g_free(_text);

    gtk_button_set_relief(emoji_button, GTK_RELIEF_NONE);

    g_signal_connect(
	emoji_button,
	"clicked",
	G_CALLBACK(handle_emoji_button_click),
	text_view
    );

    gtk_flow_box_insert(flow_box, GTK_WIDGET(emoji_button), -1);
    gtk_widget_show(GTK_WIDGET(emoji_button));

    g_string_free(string, TRUE);
  }
}

static void
handle_search_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  UI_PICKER_Handle *handle = (UI_PICKER_Handle*) user_data;

  const gchar* picked = gtk_stack_get_visible_child_name(handle->picker_stack);

  HdySearchBar *search_bar = NULL;

  if (0 == g_strcmp0(picked, "emoji"))
    search_bar = handle->emoji_search_bar;

  if (search_bar)
    hdy_search_bar_set_search_mode(
	search_bar,
	!hdy_search_bar_get_search_mode(search_bar)
    );
}

UI_PICKER_Handle*
ui_picker_new(UNUSED MESSENGER_Application *app,
	      UI_CHAT_Handle *chat)
{
  UI_PICKER_Handle *handle = g_malloc(sizeof(UI_PICKER_Handle));

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/picker.ui")
  );

  handle->picker_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "picker_box")
  );

  handle->picker_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "picker_stack")
  );

  handle->emoji_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "emoji_stack")
  );

  handle->recent_emoji_page = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "recent_emoji_page")
  );

  handle->picker_switcher_bar = HDY_VIEW_SWITCHER_BAR(
      gtk_builder_get_object(handle->builder, "picker_switcher_bar")
  );

  handle->emoji_switcher_bar = HDY_VIEW_SWITCHER_BAR(
      gtk_builder_get_object(handle->builder, "emoji_switcher_bar")
  );

  handle->recent_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "recent_flow_box")
  );

  handle->people_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "people_flow_box")
  );

  _add_emoji_buttons(
      handle->people_flow_box,
      chat->send_text_view,
      EMOJI_SMILEYS_CHARACTER_COUNT,
      emoji_smileys_characters
  );

  handle->nature_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "nature_flow_box")
  );

  _add_emoji_buttons(
      handle->nature_flow_box,
      chat->send_text_view,
      EMOJI_ANIMALS_CHARACTER_COUNT,
      emoji_animals_characters
  );

  handle->food_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "food_flow_box")
  );

  _add_emoji_buttons(
      handle->food_flow_box,
      chat->send_text_view,
      EMOJI_FOOD_CHARACTER_COUNT,
      emoji_food_characters
  );

  handle->activities_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "activities_flow_box")
  );

  _add_emoji_buttons(
      handle->activities_flow_box,
      chat->send_text_view,
      EMOJI_ACTIVITIES_CHARACTER_COUNT,
      emoji_activities_characters
  );

  handle->travel_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "travel_flow_box")
  );

  _add_emoji_buttons(
      handle->travel_flow_box,
      chat->send_text_view,
      EMOJI_TRAVEL_CHARACTER_COUNT,
      emoji_travel_characters
  );

  handle->objects_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "objects_flow_box")
  );

  _add_emoji_buttons(
      handle->objects_flow_box,
      chat->send_text_view,
      EMOJI_OBJECTS_CHARACTER_COUNT,
      emoji_objects_characters
  );

  handle->symbols_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "symbols_flow_box")
  );

  _add_emoji_buttons(
      handle->symbols_flow_box,
      chat->send_text_view,
      EMOJI_SYMBOLS_CHARACTER_COUNT,
      emoji_symbols_characters
  );

  handle->flags_flow_box = GTK_FLOW_BOX(
      gtk_builder_get_object(handle->builder, "flags_flow_box")
  );

  _add_emoji_buttons(
      handle->flags_flow_box,
      chat->send_text_view,
      EMOJI_FLAGS_CHARACTER_COUNT,
      emoji_flags_characters
  );

  handle->emoji_search_bar = HDY_SEARCH_BAR(
      gtk_builder_get_object(handle->builder, "emoji_search_bar")
  );

  handle->emoji_search_entry = GTK_SEARCH_ENTRY(
      gtk_builder_get_object(handle->builder, "emoji_search_entry")
  );

  handle->search_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "search_button")
  );

  g_signal_connect(
      handle->search_button,
      "clicked",
      G_CALLBACK(handle_search_button_click),
      handle
  );

  handle->settings_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "settings_button")
  );

  return handle;
}

void
ui_picker_delete(UI_PICKER_Handle *handle)
{
  hdy_view_switcher_bar_set_stack(handle->picker_switcher_bar, NULL);
  hdy_view_switcher_bar_set_stack(handle->emoji_switcher_bar, NULL);

  g_object_unref(handle->builder);

  g_free(handle);
}
