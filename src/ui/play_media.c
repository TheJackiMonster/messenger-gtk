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
 * @file ui/play_media.c
 */

#include "play_media.h"

#include "../application.h"
#include "../ui.h"

static void
handle_back_button_click(GtkButton *button,
			 gpointer user_data)
{
  GtkWindow *window = GTK_WINDOW(user_data);
  gtk_window_close(window);
}

static void
_pause_playing_media(UI_PLAY_MEDIA_Handle *handle)
{
  //

  handle->playing = FALSE;
}

static void
_continue_playing_media(UI_PLAY_MEDIA_Handle *handle)
{
  //

  handle->playing = TRUE;
}

static void
handle_play_pause_button_click(GtkButton *button,
			       gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (handle->playing)
    _pause_playing_media(handle);
  else
    _continue_playing_media(handle);

  gtk_stack_set_visible_child_name(
      handle->play_symbol_stack,
      handle->playing? "pause_page" : "play_page"
  );
}

static void
handle_fullscreen_button_click(GtkButton *button,
			       gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  gtk_revealer_set_reveal_child(handle->header_revealer, handle->fullscreen);
  hdy_flap_set_reveal_flap(handle->controls_flap, handle->fullscreen);

  handle->fullscreen = !(handle->fullscreen);

  if (!(handle->fullscreen))
    gtk_window_unfullscreen(GTK_WINDOW(handle->window));

  gtk_widget_hide(GTK_WIDGET(handle->window));

  gtk_window_set_type_hint(
      GTK_WINDOW(handle->window),
      handle->fullscreen?
	  GDK_WINDOW_TYPE_HINT_NORMAL :
	  GDK_WINDOW_TYPE_HINT_DIALOG
  );

  gtk_window_set_modal(GTK_WINDOW(handle->window), !(handle->fullscreen));

  gtk_window_set_position(
      GTK_WINDOW(handle->window),
      handle->fullscreen? GTK_WIN_POS_NONE : GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->window),
      handle->fullscreen? NULL : handle->parent
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));

  if (handle->fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(handle->window));

  gtk_stack_set_visible_child_name(
      handle->fullscreen_symbol_stack,
      handle->fullscreen? "scale_down_page" : "scale_up_page"
  );
}

static gboolean
handle_media_motion_lost(gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (!(hdy_flap_get_reveal_flap(handle->controls_flap)))
    return FALSE;

  hdy_flap_set_reveal_flap(handle->controls_flap, FALSE);
  handle->motion_lost = 0;
  return FALSE;
}

static gboolean
handle_media_motion_notify(GtkWidget *widget,
			   GdkEvent *event,
			   gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (hdy_flap_get_reveal_flap(handle->controls_flap))
    return FALSE;

  if (handle->motion_lost)
    g_source_remove(handle->motion_lost);

  hdy_flap_set_reveal_flap(handle->controls_flap, TRUE);
  handle->motion_lost = g_timeout_add_seconds(
      3,
      G_SOURCE_FUNC(handle_media_motion_lost),
      handle
  );

  return FALSE;
}

static void
handle_window_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_play_media_window_cleanup((UI_PLAY_MEDIA_Handle*) user_data);
}

void
ui_play_media_window_init(MESSENGER_Application *app,
			  UI_PLAY_MEDIA_Handle *handle)
{
  GNUNET_assert((app) && (handle));

  handle->parent = GTK_WINDOW(app->ui.messenger.main_window);

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/play_media.ui")
  );

  handle->window = HDY_WINDOW(
      gtk_builder_get_object(handle->builder, "play_media_window")
  );

  gtk_window_set_position(
      GTK_WINDOW(handle->window),
      GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->window),
      handle->parent
  );

  handle->header_revealer = GTK_REVEALER(
      gtk_builder_get_object(handle->builder, "header_revealer")
  );

  handle->title_bar = HDY_HEADER_BAR(
      gtk_builder_get_object(handle->builder, "title_bar")
  );

  hdy_header_bar_set_title(handle->title_bar, _("Play Media"));

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
      handle->back_button,
      "clicked",
      G_CALLBACK(handle_back_button_click),
      handle->window
  );

  handle->controls_flap = HDY_FLAP(
      gtk_builder_get_object(handle->builder, "controls_flap")
  );

  handle->play_pause_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "play_pause_button")
  );

  handle->play_symbol_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "play_symbol_stack")
  );

  g_signal_connect(
      handle->play_pause_button,
      "clicked",
      G_CALLBACK(handle_play_pause_button_click),
      handle
  );

  handle->volume_button = GTK_VOLUME_BUTTON(
      gtk_builder_get_object(handle->builder, "volume_button")
  );

  handle->timeline_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "timeline_label")
  );

  handle->settings_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "settings_button")
  );

  handle->fullscreen_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "fullscreen_button")
  );

  handle->fullscreen_symbol_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "fullscreen_symbol_stack")
  );

  g_signal_connect(
      handle->fullscreen_button,
      "clicked",
      G_CALLBACK(handle_fullscreen_button_click),
      handle
  );

  g_signal_connect(
      handle->window,
      "motion-notify-event",
      G_CALLBACK(handle_media_motion_notify),
      handle
  );

  gtk_widget_add_events(
      GTK_WIDGET(handle->window),
      GDK_POINTER_MOTION_HINT_MASK |
      GDK_POINTER_MOTION_MASK
  );

  g_signal_connect(
      handle->window,
      "destroy",
      G_CALLBACK(handle_window_destroy),
      handle
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));
}

void
ui_play_media_window_cleanup(UI_PLAY_MEDIA_Handle *handle)
{
  GNUNET_assert(handle);

  g_object_unref(handle->builder);

  if (handle->motion_lost)
    g_source_remove(handle->motion_lost);

  memset(handle, 0, sizeof(*handle));
}
