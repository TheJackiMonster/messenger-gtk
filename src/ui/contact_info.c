/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file ui/contact_info.c
 */

#include "contact_info.h"

#include "chat_entry.h"

#include "../application.h"
#include "../ui.h"
#include <gnunet/gnunet_chat_lib.h>

static void
handle_contact_edit_button_click(UNUSED GtkButton *button,
				 gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  gboolean editable = gtk_widget_is_sensitive(
      GTK_WIDGET(handle->contact_name_entry)
  );

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(
	  G_OBJECT(handle->contact_name_entry),
	  handle->app->quarks.data
      )
  );

  if ((!editable) || (!contact))
    goto skip_change_name;

  const gchar *name = gtk_entry_get_text(handle->contact_name_entry);

  if ((name) && (0 == g_utf8_strlen(name, 1)))
    name = NULL;

  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
  {
    if (GNUNET_YES != GNUNET_CHAT_set_name(handle->app->chat.messenger.handle,
					   name))
      gtk_entry_set_text(
	      handle->contact_name_entry,
	      GNUNET_CHAT_contact_get_name(contact)
      );
  }
  else
    GNUNET_CHAT_contact_set_name(contact, name);

skip_change_name:
  gtk_image_set_from_icon_name(
      handle->contact_edit_symbol,
      editable?
	    "document-edit-symbolic" :
	    "emblem-ok-symbolic",
      GTK_ICON_SIZE_BUTTON
  );

  gtk_widget_set_sensitive(
      GTK_WIDGET(handle->contact_name_entry),
      !editable
  );
}

static void
handle_contact_name_entry_activate(UNUSED GtkEntry *entry,
				                           gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  handle_contact_edit_button_click(handle->contact_edit_button, handle);
}

static void
_contact_info_reveal_identity(UI_CONTACT_INFO_Handle *handle)
{
  gtk_widget_set_visible(GTK_WIDGET(handle->back_button), TRUE);

  gtk_stack_set_visible_child(
      handle->contact_info_stack,
      handle->identity_box
  );
}

static void
handle_reveal_identity_button_click(UNUSED GtkButton *button,
                                    gpointer user_data)
{
  _contact_info_reveal_identity((UI_CONTACT_INFO_Handle*) user_data);
}

static void
handle_block_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(
	    G_OBJECT(handle->block_stack),
	    handle->app->quarks.data
    )
  );

  if (!contact)
    return;

  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_YES);

  gtk_stack_set_visible_child(
      handle->block_stack,
      GTK_WIDGET(handle->unblock_button)
  );
}

static void
handle_unblock_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(
	    G_OBJECT(handle->block_stack),
	    handle->app->quarks.data
    )
  );

  if (!contact)
    return;

  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_NO);

  gtk_stack_set_visible_child(
      handle->block_stack, 
      GTK_WIDGET(handle->block_button)
  );
}

static void
handle_open_chat_button_click(UNUSED GtkButton *button,
			                        gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(
	    G_OBJECT(handle->contact_name_entry),
	    handle->app->quarks.data
    )
  );

  if (!contact)
    return;

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (!context)
    return;

  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
  {
    GNUNET_CHAT_context_request(context);
    goto close_dialog;
  }

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!entry) || (!(entry->entry_box)))
    return;

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  if (!row)
    return;

  gtk_list_box_select_row(handle->app->ui.messenger.chats_listbox, row);
  gtk_list_box_invalidate_filter(handle->app->ui.messenger.chats_listbox);

  gtk_widget_activate(GTK_WIDGET(row));

close_dialog:
  gtk_window_close(GTK_WINDOW(handle->dialog));
}

static void
handle_back_button_click(UNUSED GtkButton *button,
			                   gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  gtk_widget_set_visible(GTK_WIDGET(handle->back_button), FALSE);

  gtk_stack_set_visible_child(
      handle->contact_info_stack,
      handle->details_box
  );
}

static void
handle_close_button_click(UNUSED GtkButton *button,
			  gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_contact_info_dialog_cleanup((UI_CONTACT_INFO_Handle*) user_data);
}

static gboolean
handle_id_drawing_area_draw(GtkWidget* drawing_area,
			    cairo_t* cairo,
			    gpointer user_data)
{
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  if (!context)
    return FALSE;

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  if ((!(handle->qr)) || (handle->qr->width <= 0))
    return FALSE;

  const guint m = 3;
  const guint w = handle->qr->width;
  const guint w2 = w + m * 2;

  guchar *pixels = (guchar*) g_malloc(sizeof(guchar) * w2 * w2 * 3);

  guint x, y, z;
  for (y = 0; y < w2; y++)
    for (x = 0; x < w2; x++)
    {
      guchar value;

      if ((x >= m) && (y >= m) && (x - m < w) && (y - m < w))
	value  = ((handle->qr->data[(y - m) * w + x - m]) & 1);
      else
	value = 0;

      for (z = 0; z < 3; z++)
	pixels[(y * w2 + x) * 3 + z] = value? 0x00 : 0xff;
    }

  GdkPixbuf *image = gdk_pixbuf_new_from_data(
      pixels,
      GDK_COLORSPACE_RGB,
      FALSE,
      8,
      w2,
      w2,
      w2 * 3,
      NULL,
      NULL
  );

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

  g_free(pixels);

  return FALSE;
}

void
ui_contact_info_dialog_init(MESSENGER_Application *app,
			    UI_CONTACT_INFO_Handle *handle)
{
  handle->app = app;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/contact_info.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "contact_info_dialog")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->contact_info_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "contact_info_stack")
  );

  handle->details_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "details_box")
  );

  handle->contact_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "contact_avatar")
  );

  handle->contact_name_entry = GTK_ENTRY(
      gtk_builder_get_object(handle->builder, "contact_name")
  );

  handle->contact_edit_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "contact_edit_button")
  );

  handle->contact_edit_symbol = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "contact_edit_symbol")
  );

  g_signal_connect(
      handle->contact_name_entry,
      "activate",
      G_CALLBACK(handle_contact_name_entry_activate),
      handle
  );

  g_signal_connect(
      handle->contact_edit_button,
      "clicked",
      G_CALLBACK(handle_contact_edit_button_click),
      handle
  );

  handle->reveal_identity_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "reveal_identity_button")
  );

  g_signal_connect(
      handle->reveal_identity_button,
      "clicked",
      G_CALLBACK(handle_reveal_identity_button_click),
      handle
  );

  handle->block_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "block_stack")
  );

  handle->block_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "block_button")
  );

  g_signal_connect(
      handle->block_button,
      "clicked",
      G_CALLBACK(handle_block_button_click),
      handle
  );

  handle->unblock_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "unblock_button")
  );

  g_signal_connect(
      handle->unblock_button,
      "clicked",
      G_CALLBACK(handle_unblock_button_click),
      handle
  );

  handle->open_chat_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "open_chat_button")
  );

  g_signal_connect(
      handle->open_chat_button,
      "clicked",
      G_CALLBACK(handle_open_chat_button_click),
      handle
  );

  handle->identity_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "identity_box")
  );

  handle->name_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "name_label")
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

  handle->back_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
      handle->back_button,
      "clicked",
      G_CALLBACK(handle_back_button_click),
      handle
  );

  handle->close_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
      handle->close_button,
      "clicked",
      G_CALLBACK(handle_close_button_click),
      handle->dialog
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );
}

void
ui_contact_info_dialog_update(UI_CONTACT_INFO_Handle *handle,
                              struct GNUNET_CHAT_Contact *contact,
                              gboolean reveal)
{
  const char *name = GNUNET_CHAT_contact_get_name(contact);

  ui_avatar_set_text(handle->contact_avatar, name);
  ui_entry_set_text(handle->contact_name_entry, name);

  g_object_set_qdata(
      G_OBJECT(handle->contact_name_entry),
      handle->app->quarks.data,
      contact
  );

  const char *key = GNUNET_CHAT_contact_get_key(contact);

  if (handle->qr)
    QRcode_free(handle->qr);

  if (key)
    handle->qr = QRcode_encodeString(
	key,
	0,
	QR_ECLEVEL_L,
	QR_MODE_8,
	0
    );
  else
    handle->qr = NULL;

  ui_label_set_text(handle->name_label, name);

  if (handle->id_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(handle->id_drawing_area));

  ui_entry_set_text(handle->id_entry, key);

  gtk_widget_set_sensitive(
      GTK_WIDGET(handle->reveal_identity_button),
      key? TRUE : FALSE
  );

  gtk_stack_set_visible_child(
      handle->block_stack,
      GNUNET_YES == GNUNET_CHAT_contact_is_blocked(contact)?
      GTK_WIDGET(handle->unblock_button) :
      GTK_WIDGET(handle->block_button)
  );

  g_object_set_qdata(
      G_OBJECT(handle->block_stack),
      handle->app->quarks.data,
      contact
  );

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  gtk_widget_set_sensitive(
      GTK_WIDGET(handle->open_chat_button),
      context? TRUE : FALSE
  );

  gtk_widget_set_visible(
      GTK_WIDGET(handle->open_chat_button),
      GNUNET_YES != GNUNET_CHAT_contact_is_owned(contact)
  );

  if (reveal)
    _contact_info_reveal_identity(handle);
}

void
ui_contact_info_dialog_cleanup(UI_CONTACT_INFO_Handle *handle)
{
  g_signal_handler_disconnect(
      handle->id_drawing_area,
      handle->id_draw_signal
  );

  g_object_unref(handle->builder);

  if (handle->qr)
    QRcode_free(handle->qr);

  memset(handle, 0, sizeof(*handle));
}
