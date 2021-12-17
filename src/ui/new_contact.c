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

#include "new_contact.h"

#include "../application.h"

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

  // TODO: Add new contact

  gtk_window_close(GTK_WINDOW(app->ui.new_contact.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_new_contact_dialog_cleanup((UI_NEW_CONTACT_Handle*) user_data);
}

static gboolean
handle_id_drawing_area_draw(GtkWidget* drawing_area,
			    cairo_t* cairo,
			    gpointer user_data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  GdkPixbuf *image = NULL;

  if (!handle->image)
    goto render_image;

  uint w, h;
  zbar_image_get_size(handle->image, &w, &h);

  uint x, y, min_size;
  min_size = (w < h? w : h);
  x = (w - min_size) / 2;
  y = (h - min_size) / 2;

  const void* data = (const void*) (
      (const char*) zbar_image_get_data(handle->image) +
      (x + y * w) * 3
  );

  image = gdk_pixbuf_new_from_data(
    data,
    GDK_COLORSPACE_RGB,
    FALSE,
    8,
    min_size,
    min_size,
    w * 3,
    NULL,
    NULL
  );

  GString *scan_result = (GString*) zbar_image_get_userdata(handle->image);

  if (!scan_result)
    goto render_image;

  gtk_entry_set_text(handle->id_entry, scan_result->str);
  g_string_free(scan_result, TRUE);

render_image:
  if (!image)
    return FALSE;

  int dwidth = gdk_pixbuf_get_width(image);
  int dheight = gdk_pixbuf_get_height(image);

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
  g_object_unref(image);

  zbar_image_destroy(handle->image);
  handle->image = NULL;

  return FALSE;
}

static gboolean
idle_video_processing(gpointer user_data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) user_data;

  if (0 == handle->idle_processing)
    return FALSE;

  zbar_image_t *image = zbar_video_next_image(handle->video);

  if (!image)
    return TRUE;

  GString *scan_result = NULL;

  zbar_image_t *y8 = zbar_image_convert(
      image,
      zbar_fourcc('Y', '8', '0', '0')
  );

  if (zbar_scan_image(handle->scanner, y8) <= 0)
    goto cleanup_scan;

  const zbar_symbol_set_t* set = zbar_image_scanner_get_results(
      handle->scanner
  );

  const zbar_symbol_t* symbol = zbar_symbol_set_first_symbol(set);

  if (!symbol)
    goto cleanup_scan;

  uint data_len = 0;
  const char *data = NULL;

  for (; symbol; symbol = zbar_symbol_next(symbol))
  {
    if (zbar_symbol_get_count(symbol))
      continue;

    data_len = zbar_symbol_get_data_length(symbol);
    data = zbar_symbol_get_data(symbol);
  }

  if ((data_len > 0) && (data))
    scan_result = g_string_new_len(data, data_len);

cleanup_scan:
  zbar_image_destroy(y8);

  zbar_image_t *rgb = zbar_image_convert(
      image,
      zbar_fourcc('R', 'G', 'B', '3')
  );

  if (!rgb)
    goto cleanup_image;

  zbar_image_set_userdata(rgb, scan_result);

  if (handle->image)
    zbar_image_destroy(handle->image);

  handle->image = rgb;

  if (handle->id_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(handle->id_drawing_area));

cleanup_image:
  zbar_image_destroy(image);
  return TRUE;
}

static void*
_ui_new_contact_video_thread(void *args)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) args;

  if (0 != zbar_video_open(handle->video, ""))
    return NULL;

  if (0 != zbar_video_enable(handle->video, 1))
    return NULL;

  zbar_image_scanner_set_config(
      handle->scanner,
      ZBAR_QRCODE,
      ZBAR_CFG_ENABLE,
      TRUE
  );

  handle->idle_processing = g_idle_add(idle_video_processing, handle);
  return NULL;
}

void
ui_new_contact_dialog_init(MESSENGER_Application *app,
			   UI_NEW_CONTACT_Handle *handle)
{
  handle->video = zbar_video_create();
  handle->scanner = zbar_image_scanner_create();

  pthread_create(&(handle->video_tid), NULL, _ui_new_contact_video_thread, handle);

  handle->builder = gtk_builder_new_from_file("resources/ui/new_contact.ui");

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "new_contact_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      _("New Contact")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->id_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(handle->builder, "id_drawing_area")
  );

  g_signal_connect(
      handle->id_drawing_area,
      "draw",
      G_CALLBACK(handle_id_drawing_area_draw),
      handle
  );

  handle->id_entry = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "id_entry")
  );

  handle->cancel_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
      handle->cancel_button,
      "clicked",
      G_CALLBACK(handle_cancel_button_click),
      handle->dialog
  );

  handle->confirm_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "confirm_button")
  );

  g_signal_connect(
      handle->confirm_button,
      "clicked",
      G_CALLBACK(handle_confirm_button_click),
      app
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );

  handle->idle_processing = 0;
}

void
ui_new_contact_dialog_cleanup(UI_NEW_CONTACT_Handle *handle)
{
  pthread_join(handle->video_tid, NULL);

  if (0 != handle->idle_processing)
    g_source_remove(handle->idle_processing);

  handle->idle_processing = 0;

  g_object_unref(handle->builder);

  if (handle->image)
    zbar_image_destroy(handle->image);

  handle->image = NULL;

  zbar_image_scanner_destroy(handle->scanner);
  zbar_video_destroy(handle->video);
}
