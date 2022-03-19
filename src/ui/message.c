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
#include "../file.h"

static void
handle_downloading_file(void *cls,
			const struct GNUNET_CHAT_File *file,
			uint64_t completed,
			uint64_t size)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (!app)
    return;

  file_update_download_info(file, app, completed, size);
}

static void
handle_file_button_click(GtkButton *button,
			 gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle* handle = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if (!handle)
    return;

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File*) (
      g_object_get_qdata(G_OBJECT(handle->file_progress_bar), app->quarks.data)
  );

  if (!file)
    return;

  uint64_t size = GNUNET_CHAT_file_get_size(file);

  if (size <= 0)
    return;

  uint64_t local_size = GNUNET_CHAT_file_get_local_size(file);

  if (GNUNET_YES == GNUNET_CHAT_file_is_downloading(file))
  {
    GNUNET_CHAT_file_stop_download(file);

    gtk_image_set_from_icon_name(
	handle->file_status_image,
	"folder-download-symbolic",
	GTK_ICON_SIZE_BUTTON
    );
  }
  else if (local_size < size)
  {
    GNUNET_CHAT_file_start_download(file, handle_downloading_file, app);

    gtk_image_set_from_icon_name(
    	handle->file_status_image,
    	"process-stop-symbolic",
    	GTK_ICON_SIZE_BUTTON
    );
  }
  else if (size > 0)
  {
    const gchar *preview = GNUNET_CHAT_file_open_preview(file);

    if (!preview)
      return;

    GString* uri = g_string_new("file://");
    g_string_append(uri, preview);

    if (!g_app_info_launch_default_for_uri(uri->str, NULL, NULL))
      GNUNET_CHAT_file_close_preview(file);

    g_string_free(uri, TRUE);
  }
}

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

  handle->timestamp = GNUNET_TIME_absolute_get_zero_();
  handle->msg = NULL;

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

  handle->builder[0] = gtk_builder_new_from_resource(
      application_get_resource_path(app, ui_builder_file)
  );

  handle->message_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder[0], "message_box")
  );

  handle->sender_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder[0], "sender_avatar")
  );

  handle->sender_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder[0], "sender_label")
  );

  if (UI_MESSAGE_STATUS == handle->type)
  {
    handle->deny_revealer = GTK_REVEALER(
	gtk_builder_get_object(handle->builder[0], "deny_revealer")
    );

    handle->accept_revealer = GTK_REVEALER(
    	gtk_builder_get_object(handle->builder[0], "accept_revealer")
    );

    handle->deny_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder[0], "deny_button")
    );

    handle->accept_button = GTK_BUTTON(
	gtk_builder_get_object(handle->builder[0], "accept_button")
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
      gtk_builder_get_object(handle->builder[0], "content_box")
  );

  handle->builder[1] = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/message_content.ui")
  );

  handle->timestamp_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder[1], "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
      gtk_builder_get_object(handle->builder[1], "read_receipt_image")
  );

  handle->content_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder[1], "content_stack")
  );

  handle->text_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder[1], "text_label")
  );

  handle->file_revealer = GTK_REVEALER(
      gtk_builder_get_object(handle->builder[1], "file_revealer")
  );

  handle->filename_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder[1], "filename_label")
  );

  handle->file_progress_bar = GTK_PROGRESS_BAR(
      gtk_builder_get_object(handle->builder[1], "file_progress_bar")
  );

  handle->file_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder[1], "file_button")
  );

  g_signal_connect(
      handle->file_button,
      "clicked",
      G_CALLBACK(handle_file_button_click),
      app
  );

  handle->file_status_image = GTK_IMAGE(
      gtk_builder_get_object(handle->builder[1], "file_status_image")
  );

  g_object_set_qdata(G_OBJECT(handle->file_button), app->quarks.ui, handle);

  handle->preview_drawing_area = GTK_DRAWING_AREA(
      gtk_builder_get_object(handle->builder[1], "preview_drawing_area")
  );

  g_signal_connect(
      handle->preview_drawing_area,
      "draw",
      G_CALLBACK(handle_preview_drawing_area_draw),
      handle
  );

  handle->whisper_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder[1], "whisper_box")
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
      gtk_builder_get_object(handle->builder[1], "message_content_box")
  ));

  handle->preview_image = NULL;
  handle->preview_animation = NULL;
  handle->preview_animation_iter = NULL;

  handle->redraw_animation = 0;

  return handle;
}

static int
_iterate_read_receipts(void *cls,
		       UNUSED const struct GNUNET_CHAT_Message *message,
		       const struct GNUNET_CHAT_Contact *contact,
		       int read_receipt)
{
  int *count_read_receipts = (int*) cls;

  if ((GNUNET_YES == read_receipt) &&
      (GNUNET_NO == GNUNET_CHAT_contact_is_owned(contact)))
    (*count_read_receipts)++;

  return GNUNET_YES;
}

void
ui_message_refresh(UI_MESSAGE_Handle *handle)
{
  if ((!(handle->msg)) ||
      (GNUNET_YES != GNUNET_CHAT_message_is_sent(handle->msg)))
    return;

  int count = 0;
  if ((0 < GNUNET_CHAT_message_get_read_receipt(handle->msg, _iterate_read_receipts, &count)) &&
      (0 < count))
    gtk_widget_show(GTK_WIDGET(handle->read_receipt_image));
  else
    gtk_widget_hide(GTK_WIDGET(handle->read_receipt_image));
}

static void
_update_file_message(UI_MESSAGE_Handle *handle,
		     MESSENGER_Application *app,
		     struct GNUNET_CHAT_File *file)
{
  uint64_t size = GNUNET_CHAT_file_get_size(file);
  uint64_t local_size = GNUNET_CHAT_file_get_local_size(file);

  gboolean autostart_download = FALSE;

  if ((size <= 0) || (size > local_size))
  {
    gtk_image_set_from_icon_name(
      handle->file_status_image,
      "folder-download-symbolic",
      GTK_ICON_SIZE_BUTTON
    );

    if ((app->settings.accept_all_files) &&
      (!GNUNET_CHAT_file_is_downloading(file)))
      autostart_download = TRUE;

    goto file_content;
  }

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

  gtk_image_set_from_icon_name(
      handle->file_status_image,
      "document-open-symbolic",
      GTK_ICON_SIZE_BUTTON
  );

file_content:
  gtk_label_set_text(handle->filename_label, GNUNET_CHAT_file_get_name(file));

  gtk_stack_set_visible_child(
      handle->content_stack,
      GTK_WIDGET(handle->file_revealer)
  );

  gtk_revealer_set_reveal_child(handle->file_revealer, TRUE);

  g_object_set_qdata(
      G_OBJECT(handle->file_progress_bar),
      app->quarks.data,
      file
  );

  if (autostart_download)
    gtk_button_clicked(handle->file_button);
}

void
ui_message_update(UI_MESSAGE_Handle *handle,
		  MESSENGER_Application *app,
		  const struct GNUNET_CHAT_Message *msg)
{
  struct GNUNET_CHAT_File *file = NULL;

  handle->msg = msg;

  ui_message_refresh(handle);

  if (msg)
  {
    if (GNUNET_CHAT_KIND_WHISPER == GNUNET_CHAT_message_get_kind(msg))
      gtk_stack_set_visible_child(
	  handle->content_stack,
	  GTK_WIDGET(handle->whisper_box)
      );

    file = GNUNET_CHAT_message_get_file(msg);

    handle->timestamp = GNUNET_CHAT_message_get_timestamp(msg);

    g_object_set_qdata(G_OBJECT(handle->message_box), app->quarks.data, file);
  }
  else
    file = (struct GNUNET_CHAT_File*) (
	g_object_get_qdata(G_OBJECT(handle->message_box), app->quarks.data)
    );

  if (!file)
    return;

  _update_file_message(handle, app, file);
}

void
ui_message_delete(UI_MESSAGE_Handle *handle)
{
  _clear_message_preview_data(handle);

  g_object_unref(handle->builder[1]);
  g_object_unref(handle->builder[0]);

  g_free(handle);
}
