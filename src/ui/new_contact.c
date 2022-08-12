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
 * @file ui/new_platform.h
 */

#include "new_contact.h"

#include <gstreamer-1.0/gst/app/gstappsink.h>

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

  const gint id_length = gtk_entry_get_text_length(app->ui.new_contact.id_entry);
  const gchar *id_text = gtk_entry_get_text(app->ui.new_contact.id_entry);

  if (id_length <= 0)
    goto close_dialog;

  gchar *emsg = NULL;
  struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_uri_parse(id_text, &emsg);

  if (emsg)
  {
    g_printerr("ERROR: %s\n", emsg);
    GNUNET_free(emsg);
  }

  if (!uri)
    goto close_dialog;

  GNUNET_CHAT_lobby_join(app->chat.messenger.handle, uri);
  GNUNET_CHAT_uri_destroy(uri);

close_dialog:
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

  if (!context)
    return FALSE;

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  if (!(handle->image))
    return FALSE;

  uint w, h;
  w = gdk_pixbuf_get_width(handle->image);
  h = gdk_pixbuf_get_height(handle->image);

  uint min_size = (w < h? w : h);

  double ratio_width = 1.0 * width / min_size;
  double ratio_height = 1.0 * height / min_size;

  const double ratio = ratio_width < ratio_height? ratio_width : ratio_height;

  w = (uint) (min_size * ratio);
  h = (uint) (min_size * ratio);

  double dx = (width - w) * 0.5;
  double dy = (height - h) * 0.5;

  const int interp_type = (ratio >= 1.0?
      GDK_INTERP_NEAREST :
      GDK_INTERP_BILINEAR
  );

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
      handle->image,
      w,
      h,
      interp_type
  );

  gtk_render_icon(context, cairo, scaled, dx, dy);

  cairo_fill(cairo);

  g_object_unref(scaled);
  return FALSE;
}

static void
_disable_video_processing(UI_NEW_CONTACT_Handle *handle,
			  gboolean drop_pipeline)
{
  gtk_stack_set_visible_child(handle->preview_stack, handle->fail_box);

  if (0 != handle->idle_processing)
    g_source_remove(handle->idle_processing);

  handle->idle_processing = 0;

  if ((!(handle->pipeline)) || (!drop_pipeline))
    return;

  gst_element_set_state(handle->pipeline, GST_STATE_NULL);
}

static void
_handle_video_sample(UI_NEW_CONTACT_Handle *handle,
		     GstAppSink *appsink)
{
  GstSample *sample = gst_app_sink_try_pull_sample(appsink, 10);

  if (!sample)
    return;

  GstCaps *caps = gst_sample_get_caps(sample);

  GstStructure *s = gst_caps_get_structure(caps, 0);

  gint width, height;
  gst_structure_get_int(s, "width", &width);
  gst_structure_get_int(s, "height", &height);

  uint x, y, min_size;
  min_size = (width < height? width : height);
  x = (width - min_size) / 2;
  y = (height - min_size) / 2;

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;

  gst_buffer_map(buffer, &map, GST_MAP_READ);

  if (handle->image)
    g_object_unref(handle->image);

  const void* data = (const void*) (
      (const char*) (map.data) + (x + y * width) * 3
  );

  handle->image = gdk_pixbuf_new_from_data(
      data,
      GDK_COLORSPACE_RGB,
      FALSE,
      8,
      min_size,
      min_size,
      width * 3,
      NULL,
      NULL
  );

  gst_buffer_unmap(buffer, &map);

  if (handle->id_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(handle->id_drawing_area));

  gst_sample_unref(sample);
}

static gboolean
idle_video_processing(gpointer user_data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) user_data;

  if (0 == handle->idle_processing)
    return FALSE;

  GstAppSink *appsink = GST_APP_SINK(handle->sink);

  if (!appsink)
  {
    _disable_video_processing(handle, TRUE);
    return FALSE;
  }

  _handle_video_sample(handle, appsink);
  return TRUE;
}

static void
msg_error_cb(UNUSED GstBus *bus,
	     GstMessage *msg,
	     gpointer *data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  GError* error;
  gst_message_parse_error(msg, &error, NULL);

  if (!error)
    fprintf(stderr, "ERROR: Unknown error\n");
  else if (error->message)
    fprintf(stderr, "ERROR: %s (%d)\n", error->message, error->code);
  else
    fprintf(stderr, "ERROR: Unknown error (%d)\n", error->code);

  _disable_video_processing(handle, TRUE);
}

static void
msg_eos_cb(UNUSED GstBus *bus,
	   UNUSED GstMessage *msg,
	   gpointer *data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  if (GST_MESSAGE_SRC(msg) == GST_OBJECT(handle->pipeline))
    _disable_video_processing(handle, TRUE);
}

static void
msg_state_changed_cb(UNUSED GstBus *bus,
		     GstMessage *msg,
		     gpointer *data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

  if ((GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->pipeline)) ||
      (new_state == old_state) || (!(handle->preview_stack)))
    return;

  if (GST_STATE_PLAYING == new_state)
  {
    gtk_stack_set_visible_child(
	handle->preview_stack,
	GTK_WIDGET(handle->id_drawing_area)
    );

    if (0 == handle->idle_processing)
      handle->idle_processing = g_idle_add(idle_video_processing, handle);
  }
  else if (GST_STATE_PAUSED == new_state)
    _disable_video_processing(handle, FALSE);
}

static void
msg_barcode_cb(UNUSED GstBus *bus,
	       GstMessage *msg,
	       gpointer *data)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;
  GstMessageType msg_type = GST_MESSAGE_TYPE(msg);

  if ((GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->scanner)) ||
      (GST_MESSAGE_ELEMENT != msg_type))
    return;

  const GstStructure *s = gst_message_get_structure(msg);

  if (!s)
    return;

  const gchar *type = gst_structure_get_string(s, "type");
  const gchar *symbol = gst_structure_get_string(s, "symbol");

  if ((!type) || (!symbol) || (0 != g_strcmp0(type, "QR-Code")))
    return;

  if (handle->id_entry)
    gtk_entry_set_text(handle->id_entry, symbol);
}

static void
_setup_gst_pipeline(UI_NEW_CONTACT_Handle *handle)
{
  handle->pipeline = gst_parse_launch(
      "v4l2src name=source ! videoconvert ! zbar name=scanner"
      " ! videoconvert ! video/x-raw,format=RGB ! videoconvert ! appsink name=sink",
      NULL
  );

  handle->source = gst_bin_get_by_name(
      GST_BIN(handle->pipeline), "source"
  );

  handle->scanner = gst_bin_get_by_name(
      GST_BIN(handle->pipeline), "scanner"
  );

  handle->sink = gst_bin_get_by_name(
      GST_BIN(handle->pipeline), "sink"
  );

  gst_app_sink_set_drop(GST_APP_SINK(handle->sink), TRUE);

  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->pipeline));

  gst_bus_add_signal_watch(bus);

  g_signal_connect(
      G_OBJECT(bus),
      "message::error",
      (GCallback) msg_error_cb,
      handle
  );

  g_signal_connect(
      G_OBJECT(bus),
      "message::eos",
      (GCallback) msg_eos_cb,
      handle
  );

  g_signal_connect(
      G_OBJECT(bus),
      "message::state-changed",
      (GCallback) msg_state_changed_cb,
      handle
  );

  g_signal_connect(
      G_OBJECT(bus),
      "message",
      (GCallback) msg_barcode_cb,
      handle
  );

  gst_object_unref(bus);
}

static void*
_ui_new_contact_video_thread(void *args)
{
  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) args;

  if (!(handle->pipeline))
    return NULL;

  GstStateChangeReturn ret = gst_element_set_state(
      handle->pipeline,
      GST_STATE_PLAYING
  );

  if (GST_STATE_CHANGE_FAILURE == ret)
    _disable_video_processing(handle, TRUE);

  return NULL;
}

void
ui_new_contact_dialog_init(MESSENGER_Application *app,
			   UI_NEW_CONTACT_Handle *handle)
{
  _setup_gst_pipeline(handle);

  handle->image = NULL;

  pthread_create(
      &(handle->video_tid),
      NULL,
      _ui_new_contact_video_thread,
      handle
  );

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/new_contact.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "new_contact_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->preview_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "preview_stack")
  );

  handle->fail_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "fail_box")
  );

  handle->id_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(handle->builder, "id_drawing_area")
  );

  handle->id_draw_signal = g_signal_connect(
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

  g_signal_handler_disconnect(
      handle->id_drawing_area,
      handle->id_draw_signal
  );

  if (handle->image)
    g_object_unref(handle->image);

  g_object_unref(handle->builder);

  if (handle->pipeline)
  {
    gst_element_set_state(handle->pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->pipeline));
  }

  memset(handle, 0, sizeof(*handle));
}
