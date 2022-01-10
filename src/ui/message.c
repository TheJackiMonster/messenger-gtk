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
 * @file ui/message.c
 */

#include "message.h"

#include <gnunet/gnunet_chat_lib.h>

#include "../application.h"

static int
handle_message_redraw_animation(gpointer user_data)
{
  UI_MESSAGE_Handle *handle = (UI_MESSAGE_Handle*) user_data;

  handle->redraw_animation = 0;

  if ((handle->preview_drawing_area) &&
      ((handle->preview_image) ||
       (handle->preview_animation) ||
       (handle->preview_animation_iter)))
    gtk_widget_queue_draw(GTK_WIDGET(handle->preview_drawing_area));

  return FALSE;
}

static gboolean
handle_preview_drawing_area_draw(GtkWidget* drawing_area,
			      cairo_t* cairo,
			      gpointer user_data)
{
  UI_MESSAGE_Handle *handle = (UI_MESSAGE_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  GdkPixbuf *image = handle->preview_image;

  if (!(handle->preview_animation))
    goto render_image;

  if (handle->preview_animation_iter)
    gdk_pixbuf_animation_iter_advance(handle->preview_animation_iter, NULL);
  else
    handle->preview_animation_iter = gdk_pixbuf_animation_get_iter(
	handle->preview_animation, NULL
    );

  image = gdk_pixbuf_animation_iter_get_pixbuf(handle->preview_animation_iter);

  const int delay = gdk_pixbuf_animation_iter_get_delay_time(
      handle->preview_animation_iter
  );

  handle->redraw_animation = g_timeout_add(
      delay, handle_message_redraw_animation, handle
  );

render_image:
  if (!image)
    return FALSE;

  int dwidth = gdk_pixbuf_get_width(image);
  int dheight = gdk_pixbuf_get_height(image);

  gint optimal_height = width * dheight / dwidth;

  gtk_widget_set_size_request(
      GTK_WIDGET(drawing_area),
      width,
      optimal_height
  );

  double ratio_width = 1.0 * width / dwidth;
  double ratio_height = 1.0 * height / dheight;

  const double ratio = ratio_width < ratio_height? ratio_width : ratio_height;

  dwidth = (int) (dwidth * ratio);
  dheight = (int) (dheight * ratio);

  double dx = (width - dwidth) * 0.5;
  double dy = (height - dheight) * 0.5;

  const int interp_type = (ratio >= 1.0?
      GDK_INTERP_NEAREST :
      GDK_INTERP_BILINEAR
  );

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
      image,
      dwidth,
      dheight,
      interp_type
  );

  gtk_render_icon(context, cairo, scaled, dx, dy);

  cairo_fill(cairo);

  g_object_unref(scaled);
  return FALSE;
}

static void
_clear_message_preview_data(UI_MESSAGE_Handle *handle)
{
  if (handle->preview_image)
  {
    g_object_unref(handle->preview_image);
    handle->preview_image = NULL;
  }

  if (handle->redraw_animation)
  {
    g_source_remove(handle->redraw_animation);
    handle->redraw_animation = 0;
  }

  if (handle->preview_animation_iter)
  {
    g_object_unref(handle->preview_animation_iter);
    handle->preview_animation_iter = NULL;
  }

  if (handle->preview_animation)
  {
    g_object_unref(handle->preview_animation);
    handle->preview_animation = NULL;
  }

  if (handle->preview_drawing_area)
    gtk_widget_set_size_request(
	GTK_WIDGET(handle->preview_drawing_area),
	-1,
	-1
    );
}

UI_MESSAGE_Handle*
ui_message_new(MESSENGER_Application *app,
	       UI_MESSAGE_Type type)
{
  UI_MESSAGE_Handle* handle = g_malloc(sizeof(UI_MESSAGE_Handle));

  handle->type = type;

  const char *ui_builder_file;

  switch (handle->type)
  {
    case UI_MESSAGE_SENT:
      ui_builder_file = "ui/message-sent.ui";
      break;
    case UI_MESSAGE_STATUS:
      ui_builder_file = "ui/message-status.ui";
      break;
    default:
      ui_builder_file = "ui/message.ui";
      break;
  }

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, ui_builder_file)
  );

  handle->message_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "message_box")
  );

  handle->sender_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "sender_avatar")
  );

  handle->sender_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "sender_label")
  );

  if (UI_MESSAGE_STATUS == handle->type)
  {
    handle->deny_revealer = GTK_REVEALER(
	gtk_builder_get_object(handle->builder, "deny_revealer")
    );

    handle->accept_revealer = GTK_REVEALER(
    	gtk_builder_get_object(handle->builder, "accept_revealer")
    );

    handle->deny_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "deny_button")
    );

    handle->accept_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder, "accept_button")
    );
  }
  else
  {
    handle->deny_revealer = NULL;
    handle->accept_revealer = NULL;

    handle->deny_button = NULL;
    handle->accept_button = NULL;
  }

  GtkContainer *content_box = GTK_CONTAINER(
      gtk_builder_get_object(handle->builder, "content_box")
  );

  GtkBuilder *builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/message_content.ui")
  );

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(builder, "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(builder, "read_receipt_image")
  );

  handle->content_stack = GTK_STACK(
      gtk_builder_get_object(builder, "content_stack")
  );

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(builder, "text_label")
  );

  handle->file_revealer = GTK_REVEALER(
      gtk_builder_get_object(builder, "file_revealer")
  );

  handle->filename_label = GTK_LABEL(
      gtk_builder_get_object(builder, "filename_label")
  );

  handle->file_progress_bar = GTK_PROGRESS_BAR(
      gtk_builder_get_object(builder, "file_progress_bar")
  );

  handle->file_button = GTK_BUTTON(
      gtk_builder_get_object(builder, "file_button")
  );

  handle->file_status_image = GTK_IMAGE(
      gtk_builder_get_object(builder, "file_status_image")
  );

  handle->preview_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(builder, "preview_drawing_area")
  );

  handle->preview_draw_signal = g_signal_connect(
      handle->preview_drawing_area,
      "draw",
      G_CALLBACK(handle_preview_drawing_area_draw),
      handle
  );

  switch (handle->type)
  {
    case UI_MESSAGE_STATUS:
      gtk_widget_set_visible(GTK_WIDGET(handle->timestamp_label), FALSE);
      break;
    default:
      break;
  }

  gtk_container_add(content_box, GTK_WIDGET(
      gtk_builder_get_object(builder, "message_content_box")
  ));

  g_object_unref(builder);

  handle->preview_image = NULL;
  handle->preview_animation = NULL;
  handle->preview_animation_iter = NULL;

  handle->redraw_animation = 0;

  return handle;
}

void
ui_message_update(UI_MESSAGE_Handle *handle,
		  const struct GNUNET_CHAT_Message *msg)
{
  struct GNUNET_CHAT_File *file = GNUNET_CHAT_message_get_file(msg);

  if (!file)
    return;

  if (GNUNET_YES != GNUNET_CHAT_file_is_local(file))
    goto file_content;

  if (!(handle->preview_drawing_area))
    goto file_progress;

  const char *preview = GNUNET_CHAT_file_open_preview(file);

  if (!preview)
    goto file_progress;

  handle->preview_animation = gdk_pixbuf_animation_new_from_file(
      preview, NULL
  );

  if (!(handle->preview_animation))
    handle->preview_image = gdk_pixbuf_new_from_file(preview, NULL);

  if ((handle->preview_animation) || (handle->preview_animation))
  {
    gtk_widget_set_size_request(
	GTK_WIDGET(handle->preview_drawing_area),
	250,
	-1
    );

    gtk_stack_set_visible_child(
    	handle->content_stack,
    	GTK_WIDGET(handle->preview_drawing_area)
    );

    gtk_widget_queue_draw(GTK_WIDGET(handle->preview_drawing_area));
    return;
  }

  GNUNET_CHAT_file_close_preview(file);

file_progress:
  gtk_progress_bar_set_fraction(handle->file_progress_bar, 1.0);

file_content:
  gtk_label_set_text(handle->filename_label, GNUNET_CHAT_file_get_name(file));

  gtk_stack_set_visible_child(
      handle->content_stack,
      GTK_WIDGET(handle->file_revealer)
  );

  gtk_revealer_set_reveal_child(handle->file_revealer, TRUE);
}

void
ui_message_delete(UI_MESSAGE_Handle *handle)
{
  _clear_message_preview_data(handle);

  g_signal_handler_disconnect(
      handle->preview_drawing_area,
      handle->preview_draw_signal
  );

  g_object_unref(handle->builder);

  g_free(handle);
}
