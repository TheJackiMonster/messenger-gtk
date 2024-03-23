/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file ui/media_preview.c
 */

#include "media_preview.h"

#include "../application.h"

static int
handle_media_preview_redraw_animation(gpointer user_data)
{
  g_assert(user_data);

  UI_MEDIA_PREVIEW_Handle *handle = (UI_MEDIA_PREVIEW_Handle*) user_data;

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
  g_assert((drawing_area) && (cairo) && (user_data));

  UI_MEDIA_PREVIEW_Handle *handle = (UI_MEDIA_PREVIEW_Handle*) user_data;

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
    delay, handle_media_preview_redraw_animation, handle
  );

render_image:
  if (!image)
    return FALSE;

  int swidth = gdk_pixbuf_get_width(image);
  int sheight = gdk_pixbuf_get_height(image);

  int sx = 0;
  int sy = 0;

  if (swidth > sheight)
  {
    sx = swidth - sheight;
    swidth -= sx;
    sx /= 2;
  }
  else
  {
    sy = sheight - swidth;
    sheight -= sy;
    sy /= 2;
  }

  double ratio_width = 1.0 * width / swidth;
  double ratio_height = 1.0 * height / sheight;

  const double ratio = ratio_width < ratio_height? ratio_width : ratio_height;

  const int interp_type = (ratio >= 1.0?
    GDK_INTERP_NEAREST :
    GDK_INTERP_BILINEAR
  );

  GdkPixbuf* subimage = gdk_pixbuf_new_subpixbuf(
    image,
    sx,
    sy,
    swidth,
    sheight
  );

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
    subimage,
    width,
    height,
    interp_type
  );

  g_object_unref(subimage);
  gtk_render_icon(context, cairo, scaled, 0, 0);

  cairo_fill(cairo);
  g_object_unref(scaled);

  if (handle->preview_image)
  {
    g_object_unref(handle->preview_image);
    handle->preview_image = NULL;
  }

  return FALSE;
}

static void
_clear_message_preview_data(UI_MEDIA_PREVIEW_Handle *handle)
{
  g_assert(handle);

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
}

UI_MEDIA_PREVIEW_Handle*
ui_media_preview_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_MEDIA_PREVIEW_Handle* handle = g_malloc(sizeof(UI_MEDIA_PREVIEW_Handle));

  handle->builder = gtk_builder_new_from_resource(
    application_get_resource_path(app, "ui/media_preview.ui")
  );

  handle->media_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "media_box")
  );

  handle->preview_drawing_area = GTK_DRAWING_AREA(
    gtk_builder_get_object(handle->builder, "preview_drawing_area")
  );

  g_signal_connect(
    handle->preview_drawing_area,
    "draw",
    G_CALLBACK(handle_preview_drawing_area_draw),
    handle
  );

  handle->preview_image = NULL;
  handle->preview_animation = NULL;
  handle->preview_animation_iter = NULL;

  handle->redraw_animation = 0;

  return handle;
}

void
ui_media_preview_update(UI_MEDIA_PREVIEW_Handle *handle,
                        struct GNUNET_CHAT_File *file)
{
  g_assert((handle) && (file));

  const char *preview = GNUNET_CHAT_file_open_preview(file);

  if (!preview)
    return;

  handle->preview_animation = gdk_pixbuf_animation_new_from_file(
    preview, NULL
  );

  if (!(handle->preview_animation))
    handle->preview_image = gdk_pixbuf_new_from_file(preview, NULL);

  GNUNET_CHAT_file_close_preview(file);

  if ((handle->preview_animation) || (handle->preview_image))
    gtk_widget_queue_draw(GTK_WIDGET(handle->preview_drawing_area));
}

void
ui_media_preview_delete(UI_MEDIA_PREVIEW_Handle *handle)
{
  g_assert(handle);

  _clear_message_preview_data(handle);

  g_object_unref(handle->builder);

  g_free(handle);
}
