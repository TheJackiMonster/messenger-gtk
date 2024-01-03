/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

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

#include <gdk/gdkkeysyms.h>
#include <gnunet/gnunet_chat_lib.h>
#include <stdlib.h>

#include "chat_entry.h"
#include "file_load_entry.h"
#include "message.h"
#include "messenger.h"
#include "picker.h"
#include "account_entry.h"
#include "delete_messages.h"

#include "../application.h"
#include "../contact.h"
#include "../file.h"
#include "../ui.h"

static gboolean
_flap_chat_details_reveal_switch(gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  HdyFlap* flap = handle->flap_chat_details;

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }

  gtk_widget_set_sensitive(GTK_WIDGET(handle->messages_listbox), TRUE);
  return FALSE;
}

static void
handle_chat_details_via_button_click(UNUSED GtkButton* button,
				     gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  gtk_widget_set_sensitive(GTK_WIDGET(handle->messages_listbox), FALSE);
  g_idle_add(
      G_SOURCE_FUNC(_flap_chat_details_reveal_switch),
      handle
  );
}

static void
handle_popover_via_button_click(UNUSED GtkButton *button,
				gpointer user_data)
{
  GtkPopover *popover = GTK_POPOVER(user_data);

  if (gtk_widget_is_visible(GTK_WIDGET(popover)))
    gtk_popover_popdown(popover);
  else
    gtk_popover_popup(popover);
}

static void
handle_chat_contacts_listbox_row_activated(GtkListBox *listbox,
					   GtkListBoxRow *row,
					   gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_object_get_qdata(G_OBJECT(listbox), app->quarks.widget)
  );

  if (!text_view)
    return;

  if (!gtk_list_box_row_get_selectable(row))
  {
    ui_invite_contact_dialog_init(app, &(app->ui.invite_contact));

    g_object_set_qdata(
	G_OBJECT(app->ui.invite_contact.contacts_listbox),
	app->quarks.widget,
	text_view
    );

    gtk_widget_show(GTK_WIDGET(app->ui.invite_contact.dialog));
    return;
  }

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  if (!contact)
    return;

  hdy_flap_set_reveal_flap(handle->flap_chat_details, FALSE);

  ui_contact_info_dialog_init(app, &(app->ui.contact_info));
  ui_contact_info_dialog_update(&(app->ui.contact_info), contact, FALSE);

  gtk_widget_show(GTK_WIDGET(app->ui.contact_info.dialog));
}

static void
handle_chat_messages_listbox_size_allocate(UNUSED GtkWidget *widget,
					   UNUSED GdkRectangle *allocation,
				           gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(
      handle->chat_scrolled_window
  );

  const gdouble value = gtk_adjustment_get_value(adjustment);
  const gdouble upper = gtk_adjustment_get_upper(adjustment);
  const gdouble page_size = gtk_adjustment_get_page_size(adjustment);

  const gdouble edge_value = upper - page_size;

  if (value >= handle->edge_value)
    gtk_adjustment_set_value(adjustment, edge_value);

  handle->edge_value = upper - page_size;
}

static void
handle_back_button_click(UNUSED GtkButton *button,
			 gpointer user_data)
{
  HdyLeaflet *leaflet = HDY_LEAFLET(user_data);

  GList *children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if (children) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->data));
  }
}

static void
handle_reveal_identity_button_click(GtkButton *button,
				    gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.data)
  );

  if (!contact)
    return;

  hdy_flap_set_reveal_flap(handle->flap_chat_details, FALSE);

  ui_contact_info_dialog_init(app, &(app->ui.contact_info));
  ui_contact_info_dialog_update(&(app->ui.contact_info), contact, TRUE);

  gtk_widget_show(GTK_WIDGET(app->ui.contact_info.dialog));
}

static void
handle_block_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(G_OBJECT(handle->block_stack), handle->app->quarks.data)
  );

  if (!contact)
    return;

  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_YES);

  gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->unblock_button));
}

static void
handle_unblock_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_object_get_qdata(G_OBJECT(handle->block_stack), handle->app->quarks.data)
  );

  if (!contact)
    return;

  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_NO);

  gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->block_button));
}

static void
handle_leave_chat_button_click(UNUSED GtkButton *button,
			       gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  if ((!handle) || (!(handle->send_text_view)))
    return;

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) (
      g_object_get_qdata(
	        G_OBJECT(handle->send_text_view),
	        handle->app->quarks.data
      )
  );

  if (!context)
    return;

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_context_get_contact(
      context
  );

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_context_get_group(
      context
  );

  if (contact)
    GNUNET_CHAT_contact_delete(contact);
  else if (group)
    GNUNET_CHAT_group_leave(group);

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!entry) || (!(handle->app)))
    return;

  ui_chat_entry_dispose(entry, handle->app);
}

static gint
handle_chat_messages_sort(GtkListBoxRow* row0,
			  GtkListBoxRow* row1,
			  gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle *message0 = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(row0), app->quarks.ui)
  );

  UI_MESSAGE_Handle *message1 = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(row1), app->quarks.ui)
  );

  if ((!message0) || (!message1))
    return 0;

  struct GNUNET_TIME_Absolute timestamp0 = message0->timestamp;
  struct GNUNET_TIME_Absolute timestamp1 = message1->timestamp;

  if (GNUNET_TIME_absolute_cmp(timestamp0, <, timestamp1))
    return -1;
  else if (GNUNET_TIME_absolute_cmp(timestamp0, >, timestamp1))
    return +1;
  else
    return 0;
}

static void
handle_chat_messages_selected_rows_changed(GtkListBox *listbox,
					   gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  GList *selected = gtk_list_box_get_selected_rows(listbox);
  uint32_t count = 0;

  while (selected)
  {
    count++;
    selected = selected->next;
  }

  GString *counter = g_string_new("");
  g_string_append_printf(counter, "%u", count);
  gtk_label_set_text(handle->selection_count_label, counter->str);
  g_string_free(counter, TRUE);

  if (count > 0)
    gtk_stack_set_visible_child(handle->chat_title_stack, handle->selection_box);
  else
    gtk_stack_set_visible_child(handle->chat_title_stack, handle->title_box);
}

static void
handle_chat_selection_close_button_click(UNUSED GtkButton *button,
					 gpointer user_data)
{
  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_unselect_all(listbox);
}

void
_delete_messages_callback(MESSENGER_Application *app,
			  GList *selected,
			  gulong delay)
{
  UI_MESSAGE_Handle *message;

  while (selected)
  {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(selected->data);

    if (!row)
      goto skip_row;

    message = (UI_MESSAGE_Handle*) g_object_get_qdata(
	G_OBJECT(row),
	app->quarks.ui
    );

    if ((!message) || (!(message->msg)))
      goto skip_row;

    GNUNET_CHAT_message_delete(
    	message->msg,
    	GNUNET_TIME_relative_multiply(
    	    GNUNET_TIME_relative_get_second_(),
	    delay
	)
    );

  skip_row:
    selected = selected->next;
  }
}

static void
handle_chat_selection_delete_button_click(UNUSED GtkButton *button,
					  gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  MESSENGER_Application *app = handle->app;

  GList *selected = gtk_list_box_get_selected_rows(handle->messages_listbox);

  if (app->settings.hide_delete_dialog)
    _delete_messages_callback(app, selected, 0);
  else
  {
    ui_delete_messages_dialog_init(app, &(app->ui.delete_messages));

    ui_delete_messages_dialog_link(
	&(app->ui.delete_messages),
	_delete_messages_callback,
	selected
    );

    gtk_widget_show(GTK_WIDGET(app->ui.delete_messages.dialog));
  }
}

static void
handle_attach_file_button_click(GtkButton *button,
				gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_object_get_qdata(G_OBJECT(button), app->quarks.widget)
  );

  if (!text_view)
    return;

  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      _("Select file"),
      GTK_WINDOW(app->ui.messenger.main_window),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      _("Cancel"),
      GTK_RESPONSE_CANCEL,
      _("Confirm"),
      GTK_RESPONSE_ACCEPT,
      NULL
  );

  if (GTK_RESPONSE_ACCEPT != gtk_dialog_run(GTK_DIALOG(dialog)))
    goto close_dialog;

  gchar *filename = gtk_file_chooser_get_filename(
      GTK_FILE_CHOOSER(dialog)
  );

  if (!filename)
    return;

  ui_send_file_dialog_init(app, &(app->ui.send_file));
  ui_send_file_dialog_update(&(app->ui.send_file), filename);

  g_free(filename);

  g_object_set_qdata(
      G_OBJECT(app->ui.send_file.send_button),
      app->quarks.widget,
      text_view
  );

  gtk_widget_show(GTK_WIDGET(app->ui.send_file.dialog));

close_dialog:
  gtk_widget_destroy(dialog);
}

static void
_update_send_record_symbol(GtkTextBuffer *buffer,
			   GtkImage *symbol,
			   gboolean picker_revealed)
{
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  gtk_image_set_from_icon_name(
      symbol,
      (0 < strlen(text)) || (picker_revealed)?
      "mail-send-symbolic" :
      "audio-input-microphone-symbolic",
      GTK_ICON_SIZE_BUTTON
  );
}

static void
handle_send_text_buffer_changed(GtkTextBuffer *buffer,
				gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  _update_send_record_symbol(
      buffer,
      handle->send_record_symbol,
      gtk_revealer_get_child_revealed(handle->picker_revealer)
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

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) (
      g_object_get_qdata(G_OBJECT(text_view), app->quarks.data)
  );

  if (context)
    GNUNET_CHAT_context_send_text(context, text);

  gtk_text_buffer_delete(buffer, &start, &end);
  return TRUE;
}

static void
_drop_any_recording(UI_CHAT_Handle *handle)
{
  if ((handle->play_pipeline) && (handle->playing))
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    handle->playing = FALSE;
  }

  _update_send_record_symbol(
      gtk_text_view_get_buffer(handle->send_text_view),
      handle->send_record_symbol,
      FALSE
  );

  gtk_stack_set_visible_child(handle->send_stack, handle->send_text_box);

  if (handle->recording_filename[0])
    remove(handle->recording_filename);

  handle->recording_filename[0] = 0;
  handle->recorded = FALSE;
}

static void
handle_sending_recording_upload_file(UNUSED void *cls,
				     const struct GNUNET_CHAT_File *file,
				     uint64_t completed,
				     uint64_t size)
{
  UI_FILE_LOAD_ENTRY_Handle *file_load = cls;

  gtk_progress_bar_set_fraction(
      file_load->load_progress_bar,
      1.0 * completed / size
  );

  file_update_upload_info(file, completed, size);

  if ((completed >= size) && (file_load->chat))
    ui_chat_remove_file_load(file_load->chat, file_load);
}

static void
handle_send_record_button_click(GtkButton *button,
				gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) (
      g_object_get_qdata(G_OBJECT(handle->send_text_view), app->quarks.data)
  );

  if ((handle->recorded) && (context) &&
      (handle->recording_filename[0]) &&
      (!gtk_revealer_get_child_revealed(handle->picker_revealer)))
  {
    UI_FILE_LOAD_ENTRY_Handle *file_load = ui_file_load_entry_new(app);

    ui_label_set_text(file_load->file_label, handle->recording_filename);
    gtk_progress_bar_set_fraction(file_load->load_progress_bar, 0.0);

    struct GNUNET_CHAT_File *file = GNUNET_CHAT_context_send_file(
	context,
	handle->recording_filename,
	handle_sending_recording_upload_file,
	file_load
    );

    if (file)
    {
      file_create_info(file);

      ui_chat_add_file_load(handle, file_load);
    }
    else if (file_load)
      ui_file_load_entry_delete(file_load);

    _drop_any_recording(handle);
    return;
  }

  if (gtk_stack_get_visible_child(handle->send_stack) != handle->send_text_box)
    return;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_object_get_qdata(G_OBJECT(button), app->quarks.widget)
  );

  _send_text_from_view(app, text_view);
}

static gboolean
handle_send_record_button_pressed(GtkWidget *widget,
				  UNUSED GdkEvent *event,
				  gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_object_get_qdata(G_OBJECT(widget), app->quarks.widget)
  );

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 < strlen(text))
    return FALSE;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
      g_object_get_qdata(G_OBJECT(widget), app->quarks.ui)
  );

  if ((handle->recorded) || (!(handle->record_pipeline)) ||
      (handle->recording_filename[0]) ||
      (gtk_revealer_get_child_revealed(handle->picker_revealer)) ||
      (handle->send_text_box != gtk_stack_get_visible_child(handle->send_stack)))
    return FALSE;

  strcpy(handle->recording_filename, "/tmp/rec_XXXXXX.ogg");

  int fd = mkstemps(handle->recording_filename, 4);

  if (-1 == fd)
    return FALSE;
  else
    close(fd);

  if ((handle->play_pipeline) && (handle->playing))
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    handle->playing = FALSE;
  }

  gtk_image_set_from_icon_name(
      handle->play_pause_symbol,
      "media-playback-start-symbolic",
      GTK_ICON_SIZE_BUTTON
  );

  gtk_image_set_from_icon_name(
      handle->send_record_symbol,
      "media-record-symbolic",
      GTK_ICON_SIZE_BUTTON
  );

  gtk_label_set_text(handle->recording_label, "00:00:00");
  gtk_progress_bar_set_fraction(handle->recording_progress_bar, 0.0);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->recording_play_button), FALSE);
  gtk_stack_set_visible_child(handle->send_stack, handle->send_recording_box);

  g_object_set(
      G_OBJECT(handle->record_sink),
      "location",
      handle->recording_filename,
      NULL
  );

  gst_element_set_state(handle->record_pipeline, GST_STATE_PLAYING);

  return TRUE;
}

static gboolean
handle_send_record_button_released(GtkWidget *widget,
				   UNUSED GdkEvent *event,
				   gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_object_get_qdata(G_OBJECT(widget), app->quarks.widget)
  );

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 < strlen(text))
    return FALSE;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
      g_object_get_qdata(G_OBJECT(widget), app->quarks.ui)
  );

  if ((handle->recorded) || (!(handle->record_pipeline)) ||
      (!(handle->recording_filename[0])) ||
      (gtk_revealer_get_child_revealed(handle->picker_revealer)) ||
      (handle->send_recording_box != gtk_stack_get_visible_child(
	  handle->send_stack)))
    return FALSE;

  gtk_widget_set_sensitive(GTK_WIDGET(handle->recording_play_button), TRUE);

  gst_element_set_state(handle->record_pipeline, GST_STATE_NULL);
  handle->recorded = TRUE;

  gtk_image_set_from_icon_name(
      handle->send_record_symbol,
      "mail-send-symbolic",
      GTK_ICON_SIZE_BUTTON
  );

  return TRUE;
}

static gboolean
handle_send_text_key_press (GtkWidget *widget,
                            GdkEventKey *event,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((app->settings.mobile_design) ||
      (event->state & GDK_SHIFT_MASK) ||
      ((event->keyval != GDK_KEY_Return) &&
       (event->keyval != GDK_KEY_KP_Enter)))
    return FALSE;

  return _send_text_from_view(app, GTK_TEXT_VIEW(widget));
}

static void
handle_recording_close_button_click(UNUSED GtkButton *button,
				    gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  _drop_any_recording(handle);
}

static void
_stop_playing_recording(UI_CHAT_Handle *handle,
			gboolean reset_bar)
{
  gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
  handle->playing = FALSE;

  gtk_image_set_from_icon_name(
    handle->play_pause_symbol,
    "media-playback-start-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  gtk_progress_bar_set_fraction(
      handle->recording_progress_bar,
      reset_bar? 0.0 : 1.0
  );

  if (handle->play_timer)
  {
    g_source_remove(handle->play_timer);
    handle->play_timer = 0;
  }
}

static void
handle_recording_play_button_click(UNUSED GtkButton *button,
				   gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  if ((!(handle->recorded)) || (!(handle->play_pipeline)))
    return;

  if (handle->playing)
    _stop_playing_recording(handle, TRUE);
  else if (handle->recording_filename[0])
  {
    g_object_set(
	G_OBJECT(handle->play_source),
	"location",
	handle->recording_filename,
	NULL
    );

    gst_element_set_state(handle->play_pipeline, GST_STATE_PLAYING);
    handle->playing = TRUE;

    gtk_image_set_from_icon_name(
	handle->play_pause_symbol,
    	"media-playback-stop-symbolic",
    	GTK_ICON_SIZE_BUTTON
    );
  }
}

static void
handle_picker_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  gboolean reveal = !gtk_revealer_get_child_revealed(handle->picker_revealer);

  gtk_revealer_set_reveal_child(handle->picker_revealer, reveal);

  _update_send_record_symbol(
      gtk_text_view_get_buffer(handle->send_text_view),
      handle->send_record_symbol,
      reveal
  );
}

static gboolean
_record_timer_func(gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  GString *time_string = g_string_new(NULL);

  g_string_printf(
      time_string,
      "%02u:%02u:%02u",
      (handle->record_time / 3600),
      (handle->record_time / 60) % 60,
      (handle->record_time % 60)
  );

  gtk_label_set_text(handle->recording_label, time_string->str);
  g_string_free(time_string, TRUE);

  if (!(handle->recorded))
  {
    handle->record_time++;
    handle->record_timer = g_timeout_add_seconds(
	1,
	_record_timer_func,
	handle
    );
  }
  else
    handle->record_timer = 0;

  return FALSE;
}

static gboolean
_play_timer_func(gpointer user_data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  if (handle->play_time < handle->record_time * 100)
    gtk_progress_bar_set_fraction(
	handle->recording_progress_bar,
	0.01 * handle->play_time / handle->record_time
    );
  else
    gtk_progress_bar_set_fraction(
	handle->recording_progress_bar,
	1.0
    );

  if (handle->playing)
  {
    handle->play_time++;
    handle->play_timer = g_timeout_add(
	10,
	_play_timer_func,
	handle
    );
  }
  else
    handle->play_timer = 0;

  return FALSE;
}

static gboolean
handle_record_bus_watch(UNUSED GstBus *bus,
		        GstMessage *msg,
		        gpointer data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) data;
  GstMessageType type = GST_MESSAGE_TYPE(msg);

  switch (type)
  {
    case GST_MESSAGE_STREAM_START:
      handle->record_time = 0;
      handle->record_timer = g_timeout_add_seconds(
	  0,
	  _record_timer_func,
	  handle
      );

      break;
    default:
      break;
  }

  return TRUE;
}

static gboolean
handle_play_bus_watch(UNUSED GstBus *bus,
		      GstMessage *msg,
		      gpointer data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) data;
  GstMessageType type = GST_MESSAGE_TYPE(msg);

  switch (type)
  {
    case GST_MESSAGE_STREAM_START:
      handle->play_time = 0;
      handle->play_timer = g_timeout_add_seconds(
	0,
	_play_timer_func,
	handle
      );

      break;
    case GST_MESSAGE_EOS:
      if (handle->playing)
	_stop_playing_recording(handle, FALSE);
      break;
    default:
      break;
  }

  return TRUE;
}

static void
_setup_gst_pipelines(UI_CHAT_Handle *handle)
{
  handle->record_pipeline = gst_parse_launch(
      "autoaudiosrc ! audioconvert ! vorbisenc ! oggmux ! filesink name=sink",
      NULL
  );

  handle->record_sink = gst_bin_get_by_name(
      GST_BIN(handle->record_pipeline), "sink"
  );

  {
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->record_pipeline));

    handle->record_watch = gst_bus_add_watch(
	bus,
	handle_record_bus_watch,
	handle
    );

    gst_object_unref(bus);
  }

  handle->play_pipeline = gst_parse_launch(
      "filesrc name=source ! oggdemux ! vorbisdec ! audioconvert ! autoaudiosink",
      NULL
  );

  handle->play_source = gst_bin_get_by_name(
      GST_BIN(handle->play_pipeline), "source"
  );

  {
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->play_pipeline));

    handle->play_watch = gst_bus_add_watch(
      bus,
      handle_play_bus_watch,
      handle
    );

    gst_object_unref(bus);
  }
}

UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app)
{
  GNUNET_assert(app);

  UI_CHAT_Handle *handle = g_malloc(sizeof(UI_CHAT_Handle));
  UI_MESSENGER_Handle *messenger = &(app->ui.messenger);

  memset(handle, 0, sizeof(*handle));

  _setup_gst_pipelines(handle);

  handle->app = app;

  handle->loads = NULL;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/chat.ui")
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

  handle->flap_chat_details = HDY_FLAP(
      gtk_builder_get_object(handle->builder, "flap_chat_details")
  );

  handle->chat_title_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "chat_title_stack")
  );

  handle->title_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "title_box")
  );

  handle->selection_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "selection_box")
  );

  handle->chat_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "chat_avatar")
  );

  handle->chat_title = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_title")
  );

  handle->chat_subtitle = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_subtitle")
  );

  handle->chat_load_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "chat_load_button")
  );

  handle->chat_load_popover = GTK_POPOVER(
      gtk_builder_get_object(handle->builder, "chat_load_popover")
  );

  handle->chat_load_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "chat_load_listbox")
  );

  g_signal_connect(
      handle->chat_load_button,
      "clicked",
      G_CALLBACK(handle_popover_via_button_click),
      handle->chat_load_popover
  );

  handle->chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "chat_details_button")
  );

  g_signal_connect(
      handle->chat_details_button,
      "clicked",
      G_CALLBACK(handle_chat_details_via_button_click),
      handle
  );

  handle->chat_details_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_details_label")
  );

  handle->hide_chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "hide_chat_details_button")
  );

  g_signal_connect(
      handle->hide_chat_details_button,
      "clicked",
      G_CALLBACK(handle_chat_details_via_button_click),
      handle
  );

  handle->chat_details_contacts_box = GTK_BOX(
      gtk_builder_get_object(handle->builder, "chat_details_contacts_box")
  );

  handle->chat_details_files_box = GTK_BOX(
      gtk_builder_get_object(handle->builder, "chat_details_files_box")
  );

  handle->chat_details_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "chat_details_avatar")
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

  handle->leave_chat_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "leave_chat_button")
  );

  g_signal_connect(
      handle->leave_chat_button,
      "clicked",
      G_CALLBACK(handle_leave_chat_button_click),
      handle
  );

  handle->chat_notifications_switch = GTK_SWITCH(
      gtk_builder_get_object(handle->builder, "chat_notifications_switch")
  );

  handle->selection_close_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "selection_close_button")
  );

  handle->selection_count_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "selection_count_label")
  );

  handle->selection_delete_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "selection_delete_button")
  );

  handle->chat_scrolled_window = GTK_SCROLLED_WINDOW(
      gtk_builder_get_object(handle->builder, "chat_scrolled_window")
  );

  handle->chat_contacts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "chat_contacts_listbox")
  );

  g_signal_connect(
      handle->chat_contacts_listbox,
      "row-activated",
      G_CALLBACK(handle_chat_contacts_listbox_row_activated),
      handle
  );

  handle->chat_files_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "chat_files_listbox")
  );

  handle->messages_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "messages_listbox")
  );

  gtk_list_box_set_sort_func(
      handle->messages_listbox,
      handle_chat_messages_sort,
      app,
      NULL
  );

  g_signal_connect(
      handle->messages_listbox,
      "selected-rows-changed",
      G_CALLBACK(handle_chat_messages_selected_rows_changed),
      handle
  );

  g_signal_connect(
      handle->selection_close_button,
      "clicked",
      G_CALLBACK(handle_chat_selection_close_button_click),
      handle->messages_listbox
  );

  g_signal_connect(
      handle->selection_delete_button,
      "clicked",
      G_CALLBACK(handle_chat_selection_delete_button_click),
      handle
  );

  g_signal_connect(
      handle->messages_listbox,
      "size-allocate",
      G_CALLBACK(handle_chat_messages_listbox_size_allocate),
      handle
  );

  handle->send_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "send_stack")
  );

  handle->send_text_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "send_text_box")
  );

  handle->send_recording_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "send_recording_box")
  );

  handle->attach_file_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "attach_file_button")
  );

  g_signal_connect(
      handle->attach_file_button,
      "clicked",
      G_CALLBACK(handle_attach_file_button_click),
      app
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
      handle
  );

  g_signal_connect(
      handle->send_record_button,
      "clicked",
      G_CALLBACK(handle_send_record_button_click),
      app
  );

  g_signal_connect(
      handle->send_record_button,
      "button-press-event",
      G_CALLBACK(handle_send_record_button_pressed),
      app
  );

  g_signal_connect(
      handle->send_record_button,
      "button-release-event",
      G_CALLBACK(handle_send_record_button_released),
      app
  );

  g_signal_connect(
      handle->send_text_view,
      "key-press-event",
      G_CALLBACK(handle_send_text_key_press),
      app
  );

  g_object_set_qdata(
      G_OBJECT(handle->chat_contacts_listbox),
      app->quarks.widget,
      handle->send_text_view
  );

  g_object_set_qdata(
      G_OBJECT(handle->attach_file_button),
      app->quarks.widget,
      handle->send_text_view
  );

  g_object_set_qdata(
      G_OBJECT(handle->send_record_button),
      app->quarks.widget,
      handle->send_text_view
  );

  g_object_set_qdata(
      G_OBJECT(handle->send_record_button),
      app->quarks.ui,
      handle
  );

  handle->recording_close_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "recording_close_button")
  );

  g_signal_connect(
      handle->recording_close_button,
      "clicked",
      G_CALLBACK(handle_recording_close_button_click),
      handle
  );

  handle->recording_play_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "recording_play_button")
  );

  g_signal_connect(
      handle->recording_play_button,
      "clicked",
      G_CALLBACK(handle_recording_play_button_click),
      handle
  );

  handle->play_pause_symbol = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "play_pause_symbol")
  );

  handle->recording_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "recording_label")
  );

  handle->recording_progress_bar = GTK_PROGRESS_BAR(
      gtk_builder_get_object(handle->builder, "recording_progress_bar")
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
      handle
  );

  return handle;
}

struct IterateChatGroupClosure {
  MESSENGER_Application *app;
  GtkListBox *listbox;
};

static int
iterate_ui_chat_update_group_contacts(void *cls,
				      UNUSED const struct GNUNET_CHAT_Group *group,
				      struct GNUNET_CHAT_Contact *contact)
{
  struct IterateChatGroupClosure *closure = (
      (struct IterateChatGroupClosure*) cls
  );

  GtkListBox *listbox = closure->listbox;
  UI_ACCOUNT_ENTRY_Handle* entry = ui_account_entry_new(closure->app);

  ui_account_entry_set_contact(entry, contact);

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), closure->app->quarks.data, contact);
  g_object_set_qdata_full(
      G_OBJECT(row),
      closure->app->quarks.ui,
      entry,
      (GDestroyNotify) ui_account_entry_delete
  );

  return GNUNET_YES;
}

void
ui_chat_update(UI_CHAT_Handle *handle,
	       MESSENGER_Application *app,
	       struct GNUNET_CHAT_Context* context)
{
  GNUNET_assert((handle) && (app) && (context));

  struct GNUNET_CHAT_Contact* contact;
  struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(context);
  group = GNUNET_CHAT_context_get_group(context);

  const char *title = NULL;
  const char *icon = "action-unavailable-symbolic";

  GString *subtitle = g_string_new("");

  if (contact)
  {
    title = GNUNET_CHAT_contact_get_name(contact);
    icon = "avatar-default-symbolic";
  }
  else if (group)
  {
    title = GNUNET_CHAT_group_get_name(group);

    if ((title) && ('#' == *title))
      icon = "network-wired-symbolic";
    else
      icon = "system-users-symbolic";

    g_string_append_printf(
      subtitle,
      _("%d members"),
      GNUNET_CHAT_group_iterate_contacts(group, NULL, NULL)
    );
  }

  ui_avatar_set_text(handle->chat_avatar, title);
  hdy_avatar_set_icon_name(handle->chat_avatar, icon);

  ui_avatar_set_text(handle->chat_details_avatar, title);
  hdy_avatar_set_icon_name(handle->chat_details_avatar, icon);

  ui_label_set_text(handle->chat_title, title);
  ui_label_set_text(handle->chat_details_label, title);

  if (subtitle->len > 0)
    gtk_label_set_text(handle->chat_subtitle, subtitle->str);

  g_string_free(subtitle, TRUE);

  GList* children = gtk_container_get_children(
      GTK_CONTAINER(handle->chat_contacts_listbox)
  );

  while ((children) && (children->next)) {
    GtkWidget *widget = GTK_WIDGET(children->data);
    children = children->next;

    gtk_container_remove(
      GTK_CONTAINER(handle->chat_contacts_listbox),
      widget
    );
  }

  if (group)
  {
    struct IterateChatGroupClosure closure;
    closure.app = app;
    closure.listbox = handle->chat_contacts_listbox;

    GNUNET_CHAT_group_iterate_contacts(
	    group,
      iterate_ui_chat_update_group_contacts,
      &closure
    );
  }

  gtk_widget_set_visible(
      GTK_WIDGET(handle->chat_details_contacts_box),
      group? TRUE : FALSE
  );

  g_object_set_qdata(
      G_OBJECT(handle->reveal_identity_button),
      app->quarks.data,
      contact
  );

  gtk_widget_set_visible(
      GTK_WIDGET(handle->reveal_identity_button),
      contact? TRUE : FALSE
  );

  g_object_set_qdata(
      G_OBJECT(handle->block_stack),
      app->quarks.data,
      contact
  );

  if (GNUNET_YES == GNUNET_CHAT_contact_is_blocked(contact))
    gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->unblock_button));
  else
    gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->block_button));

  gtk_widget_set_visible(
      GTK_WIDGET(handle->block_stack),
      contact? TRUE : FALSE
  );

  gtk_widget_set_sensitive(
      GTK_WIDGET(handle->leave_chat_button),
      (contact) || (group)? TRUE : FALSE
  );

  const int status = GNUNET_CHAT_context_get_status(context);
  const gboolean activated = (GNUNET_OK == status? TRUE : FALSE);

  gtk_text_view_set_editable(handle->send_text_view, activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_text_view), activated);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->attach_file_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->emoji_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_record_button), activated);

  if (!(handle->messages))
    return;

  GList *list = handle->messages;

  while (list)
  {
    ui_message_refresh((UI_MESSAGE_Handle*) list->data);
    list = list->next;
  }

  UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) (handle->messages->data);

  if (!(message->timestamp_label))
    return;

  const gchar *time = gtk_label_get_text(message->timestamp_label);

  if (!group)
    gtk_label_set_text(handle->chat_subtitle, time);
}

void
ui_chat_delete(UI_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  ui_picker_delete(handle->picker);

  g_object_unref(handle->builder);

  if (handle->messages)
    g_list_free_full(handle->messages, (GDestroyNotify) ui_message_delete);

  if (handle->loads)
    g_list_free_full(handle->loads, (GDestroyNotify) ui_file_load_entry_delete);

  if (handle->record_pipeline)
  {
    gst_element_set_state(handle->record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->record_pipeline));
  }

  if (handle->play_pipeline)
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->play_pipeline));
  }

  if (handle->recording_filename[0])
    remove(handle->recording_filename);

  if (handle->record_timer)
    g_source_remove(handle->record_timer);

  if (handle->play_timer)
    g_source_remove(handle->play_timer);

  g_free(handle);
}

void
ui_chat_add_message(UI_CHAT_Handle *handle,
		    MESSENGER_Application *app,
		    UI_MESSAGE_Handle *message)
{
  GNUNET_assert((handle) && (message) && (message->message_box));

  gtk_container_add(
      GTK_CONTAINER(handle->messages_listbox),
      message->message_box
  );

  GtkWidget *row = gtk_widget_get_parent(message->message_box);

  g_object_set_qdata(G_OBJECT(row), app->quarks.ui, message);

  handle->messages = g_list_prepend(handle->messages, message);

  gtk_list_box_invalidate_sort(handle->messages_listbox);
}

void
ui_chat_remove_message(UI_CHAT_Handle *handle,
		       UNUSED MESSENGER_Application *app,
		       UI_MESSAGE_Handle *message)
{
  GNUNET_assert((handle) && (message) && (message->message_box));

  handle->messages = g_list_remove(handle->messages, message);

  GtkWidget *row = gtk_widget_get_parent(message->message_box);

  gtk_container_remove(GTK_CONTAINER(handle->messages_listbox), row);

  handle->messages = g_list_append(handle->messages, message);
}

void
ui_chat_add_file_load(UI_CHAT_Handle *handle,
		      UI_FILE_LOAD_ENTRY_Handle *file_load)
{
  GNUNET_assert((handle) && (file_load));

  gtk_container_add(
      GTK_CONTAINER(handle->chat_load_listbox),
      file_load->entry_box
  );

  handle->loads = g_list_append(handle->loads, file_load);

  gtk_widget_show(GTK_WIDGET(handle->chat_load_button));

  file_load->chat = handle;
}

void
ui_chat_remove_file_load(UI_CHAT_Handle *handle,
			 UI_FILE_LOAD_ENTRY_Handle *file_load)
{
  GNUNET_assert((handle) && (file_load) && (handle == file_load->chat) &&
		(file_load->entry_box));

  handle->loads = g_list_remove(handle->loads, file_load);

  gtk_container_remove(
      GTK_CONTAINER(handle->chat_load_listbox),
      gtk_widget_get_parent(file_load->entry_box)
  );

  if (handle->loads)
    return;

  if (gtk_widget_is_visible(GTK_WIDGET(handle->chat_load_popover)))
    gtk_popover_popdown(handle->chat_load_popover);

  gtk_widget_hide(GTK_WIDGET(handle->chat_load_button));

  file_load->chat = NULL;
}
