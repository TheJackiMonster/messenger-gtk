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
 * @file ui/chat.c
 */

#include "chat.h"

#include <gdk/gdkkeysyms.h>

#include "messenger.h"
#include "picker.h"
#include "profile_entry.h"

#include "../application.h"
#include "../contact.h"

static void
handle_flap_via_button_click(UNUSED GtkButton* button,
			     gpointer user_data)
{
  HdyFlap* flap = HDY_FLAP(user_data);

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }
}

static void
handle_chat_contacts_listbox_row_activated(UNUSED GtkListBox* listbox,
					   GtkListBoxRow* row,
					   gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
  {
    //g_idle_add(G_SOURCE_FUNC(_open_new_contact_dialog), app);
    return;
  }

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
      g_hash_table_lookup(app->ui.bindings, row)
  );

  if ((!contact) || (!GNUNET_CHAT_contact_get_key(contact)) ||
      (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact)))
    return;

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
      contact
  );

  if (!context)
    return;

  if (GNUNET_SYSERR == GNUNET_CHAT_context_get_status(context))
    GNUNET_CHAT_context_request(context);
}

static void
handle_back_button_click(UNUSED GtkButton* button,
			 gpointer user_data)
{
  HdyLeaflet* leaflet = HDY_LEAFLET(user_data);

  GList* children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if (children) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->data));
  }
}

static void
handle_send_text_buffer_changed(GtkTextBuffer *buffer,
				gpointer user_data)
{
  GtkImage *symbol = GTK_IMAGE(user_data);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  const gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  gtk_image_set_from_icon_name(
      symbol,
      0 < strlen(text)?
      "mail-send-symbolic" :
      "audio-input-microphone-symbolic",
      GTK_ICON_SIZE_BUTTON
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

  struct GNUNET_CHAT_Context *context = g_hash_table_lookup(
    app->ui.bindings, text_view
  );

  if (context)
    GNUNET_CHAT_context_send_text(context, text);

  gtk_text_buffer_delete(buffer, &start, &end);
  return TRUE;
}

static void
handle_send_record_button_click(GtkButton *button,
				gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
      g_hash_table_lookup(app->ui.bindings, button)
  );

  if (!_send_text_from_view(app, text_view))
  {
    // TODO: record audio and attach as file?
  }
}

static gboolean
handle_send_text_key_press (GtkWidget *widget,
                            GdkEventKey *event,
			    gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((app->ui.mobile) ||
      (event->state & GDK_SHIFT_MASK) ||
      ((event->keyval != GDK_KEY_Return) &&
       (event->keyval != GDK_KEY_KP_Enter)))
    return FALSE;

  return _send_text_from_view(app, GTK_TEXT_VIEW(widget));
}

static void
handle_picker_button_click(UNUSED GtkButton *button,
			   gpointer user_data)
{
  GtkRevealer *revealer = GTK_REVEALER(user_data);
  gboolean reveal = !gtk_revealer_get_child_revealed(revealer);

  gtk_revealer_set_reveal_child(revealer, reveal);
}

UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app)
{
  UI_CHAT_Handle *handle = g_malloc(sizeof(UI_CHAT_Handle));
  UI_MESSENGER_Handle *messenger = &(app->ui.messenger);

  handle->builder = gtk_builder_new_from_file(
      "resources/ui/chat.ui"
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

  handle->chat_title = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_title")
  );

  handle->chat_subtitle = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "chat_subtitle")
  );

  handle->chat_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "chat_details_button")
  );

  g_signal_connect(
      handle->chat_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
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
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_chat_details
  );

  handle->chat_details_contacts_box = GTK_BOX(
      gtk_builder_get_object(handle->builder, "chat_details_contacts_box")
  );

  handle->chat_contacts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "chat_contacts_listbox")
  );

  g_signal_connect(
      handle->chat_contacts_listbox,
      "row-activated",
      G_CALLBACK(handle_chat_contacts_listbox_row_activated),
      app
  );

  handle->messages_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "messages_listbox")
  );

  handle->attach_file_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "attach_file_button")
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
      handle->send_record_symbol
  );

  g_signal_connect(
      handle->send_record_button,
      "clicked",
      G_CALLBACK(handle_send_record_button_click),
      app
  );

  g_signal_connect(
      handle->send_text_view,
      "key-press-event",
      G_CALLBACK(handle_send_text_key_press),
      app
  );

  g_hash_table_insert(
      app->ui.bindings,
      handle->send_record_button,
      handle->send_text_view
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
      handle->picker_revealer
  );

  return handle;
}

struct IterateChatGroupClosure {
  GHashTable *bindings;
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
  UI_PROFILE_ENTRY_Handle* entry = ui_profile_entry_new();

  const char *name = GNUNET_CHAT_contact_get_name(contact);

  gtk_label_set_text(entry->entry_label, name? name : "");
  hdy_avatar_set_text(entry->entry_avatar, name? name : "");

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  g_hash_table_insert(closure->bindings, row, contact);

  ui_profile_entry_delete(entry);
  return GNUNET_YES;
}

void
ui_chat_update(UI_CHAT_Handle *handle,
	       MESSENGER_Application *app,
	       const struct GNUNET_CHAT_Context* context)
{
  const struct GNUNET_CHAT_Contact* contact;
  const struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(context);
  group = GNUNET_CHAT_context_get_group(context);

  const char *title = NULL;
  GString *subtitle = g_string_new("");

  if (contact)
    title = GNUNET_CHAT_contact_get_name(contact);
  else if (group)
  {
    title = GNUNET_CHAT_group_get_name(group);

    g_string_append_printf(
	subtitle,
	"%d members",
	GNUNET_CHAT_group_iterate_contacts(group, NULL, NULL)
    );
  }

  if (title)
  {
    gtk_label_set_text(handle->chat_title, title);
    gtk_label_set_text(handle->chat_details_label, title);
  }

  if (subtitle->len > 0)
    gtk_label_set_text(handle->chat_subtitle, subtitle->str);

  g_string_free(subtitle, TRUE);

  GList* children = gtk_container_get_children(
      GTK_CONTAINER(handle->chat_contacts_listbox)
  );

  while ((children) && (children->next)) {
    GtkWidget *widget = GTK_WIDGET(children->data);
    children = children->next;

    if (g_hash_table_contains(app->ui.bindings, widget))
      g_hash_table_remove(app->ui.bindings, widget);

    gtk_container_remove(
	GTK_CONTAINER(handle->chat_contacts_listbox),
	widget
    );
  }

  if (group)
  {
    struct IterateChatGroupClosure closure;
    closure.bindings = app->ui.bindings;
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

  const int status = GNUNET_CHAT_context_get_status(context);
  const gboolean activated = (GNUNET_OK == status? TRUE : FALSE);

  gtk_text_view_set_editable(handle->send_text_view, activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_text_view), activated);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->attach_file_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->emoji_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_record_button), activated);
}

void
ui_chat_delete(UI_CHAT_Handle *handle)
{
  ui_picker_delete(handle->picker);

  g_object_unref(handle->builder);

  g_free(handle);
}
