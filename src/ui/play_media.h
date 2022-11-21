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
 * @file ui/play_media.h
 */

#ifndef UI_PLAY_MEDIA_H_
#define UI_PLAY_MEDIA_H_

#include "messenger.h"

typedef struct UI_PLAY_MEDIA_Handle
{
  gboolean playing;
  gboolean fullscreen;

  GtkWindow *parent;

  GtkBuilder *builder;
  HdyWindow *window;

  GtkRevealer *header_revealer;
  HdyHeaderBar *title_bar;
  GtkButton *back_button;

  HdyFlap *controls_flap;
  GtkButton *play_pause_button;
  GtkStack *play_symbol_stack;

  GtkVolumeButton *volume_button;
  GtkLabel *timeline_label;

  GtkButton *settings_button;

  GtkButton *fullscreen_button;
  GtkStack *fullscreen_symbol_stack;

  guint motion_lost;
} UI_PLAY_MEDIA_Handle;

/**
 * Initializes a handle for the play media window
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Play media window handle
 */
void
ui_play_media_window_init(MESSENGER_Application *app,
			  UI_PLAY_MEDIA_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given play media window handle.
 *
 * @param handle Play media window handle
 */
void
ui_play_media_window_cleanup(UI_PLAY_MEDIA_Handle *handle);

#endif /* UI_PLAY_MEDIA_H_ */
