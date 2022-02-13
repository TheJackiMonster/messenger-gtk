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
 * @file ui/messenger.c
 */

#include "messenger.h"

#include <gtk-3.0/gdk/gdkkeys.h>

#include "chat_entry.h"
#include "contacts.h"
#include "message.h"
#include "new_contact.h"
#include "new_platform.h"
#include "settings.h"

#include "../application.h"

static gboolean
_flap_reveal_switch(gpointer user_data)
{
  HdyFlap* flap = HDY_FLAP(user_data);

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }

  return FALSE;
}

static void
handle_flap_via_button_click(UNUSED GtkButton* button,
			     gpointer user_data)
{
  g_idle_add(G_SOURCE_FUNC(_flap_reveal_switch), user_data);
}

static void
handle_account_details_button_click(UNUSED GtkButton* button,
				    gpointer user_data)
{
  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  GtkRevealer *revealer = handle->account_details_revealer;
  GtkImage *symbol = handle->account_details_symbol;

  gboolean old_state = gtk_revealer_get_reveal_child(revealer);

  gtk_revealer_set_reveal_child(revealer, !old_state);
  gtk_image_set_from_icon_name(
      symbol,
      old_state?
      "go-down-symbolic" :
      "go-up-symbolic",
      GTK_ICON_SIZE_BUTTON
  );
}

static void
handle_accounts_listbox_row_activated(UNUSED GtkListBox* listbox,
				      GtkListBoxRow* row,
				      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (row == app->ui.messenger.add_account_listbox_row)
  {
    hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

    ui_new_profile_dialog_init(app, &(app->ui.new_profile));

    gtk_widget_show(GTK_WIDGET(app->ui.new_profile.dialog));
    return;
  }

  struct GNUNET_CHAT_Account *account = (struct GNUNET_CHAT_Account*) (
      g_hash_table_lookup(app->ui.bindings, row)
  );

  if (!account)
    return;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  GNUNET_CHAT_connect(app->chat.messenger.handle, account);
}

static void
handle_new_contact_button_click(UNUSED GtkButton* button,
				 gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_new_contact_dialog_init(app, &(app->ui.new_contact));

  gtk_widget_show(GTK_WIDGET(app->ui.new_contact.dialog));
}

static void
handle_new_group_button_click(UNUSED GtkButton* button,
			      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_new_group_dialog_init(app, &(app->ui.new_group));

  gtk_widget_show(GTK_WIDGET(app->ui.new_group.dialog));
}

static void
handle_new_platform_button_click(UNUSED GtkButton* button,
				 gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_new_platform_dialog_init(app, &(app->ui.new_platform));

  gtk_widget_show(GTK_WIDGET(app->ui.new_platform.dialog));
}

static void
handle_contacts_button_click(UNUSED GtkButton* button,
			     gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_contacts_dialog_init(app, &(app->ui.contacts));

  gtk_widget_show(GTK_WIDGET(app->ui.contacts.dialog));
}

static void
handle_settings_button_click(UNUSED GtkButton* button,
			     gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_settings_dialog_init(app, &(app->ui.settings));

  gtk_widget_show(GTK_WIDGET(app->ui.settings.dialog));
}

static void
handle_chats_listbox_row_activated(UNUSED GtkListBox* listbox,
				   GtkListBoxRow* row,
				   gpointer user_data)
{
  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
    return;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) g_hash_table_lookup(
      handle->bindings, row
  );

  if ((!entry) || (!(entry->chat)) || (!(entry->chat->chat_box)))
    return;

  GtkStack *stack = handle->chats_stack;
  HdyLeaflet *leaflet = handle->leaflet_chat;

  GList *children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if ((children) && (children->next)) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->next->data));
  }

  gtk_stack_set_visible_child(stack, entry->chat->chat_box);
}

static gboolean
handle_chats_listbox_filter_func(GtkListBoxRow *row,
				 gpointer user_data)
{
  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  if ((!gtk_list_box_row_get_selectable(row)) ||
      (gtk_list_box_row_is_selected(row)))
    return TRUE;

  const gchar *filter = gtk_entry_get_text(
      GTK_ENTRY(handle->chats_search)
  );

  if (!filter)
    return TRUE;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) g_hash_table_lookup(
      handle->bindings, row
  );

  if (!entry)
    return FALSE;

  const gchar *title = gtk_label_get_text(entry->title_label);

  if (!title)
    return FALSE;

  return g_str_match_string(filter, title, TRUE);
}

static void
handle_chats_search_changed(UNUSED GtkSearchEntry *search,
			    gpointer user_data)
{
  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_invalidate_filter(listbox);
}

static void
handle_main_window_destroy(UNUSED GtkWidget *window,
			   gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_messenger_cleanup(&(app->ui.messenger));

  application_exit(app, MESSENGER_QUIT);
}

void
ui_messenger_init(MESSENGER_Application *app,
		  UI_MESSENGER_Handle *handle)
{
  handle->chat_entries = NULL;
  handle->bindings = app->ui.bindings;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/messenger.ui")
  );

  handle->main_window = GTK_APPLICATION_WINDOW(
      gtk_builder_get_object(handle->builder, "main_window")
  );

  gtk_application_add_window(
      app->application,
      GTK_WINDOW(handle->main_window)
  );

  gtk_window_set_default_size(
      GTK_WINDOW(handle->main_window),
      1100, 700
  );

  handle->leaflet_chat = HDY_LEAFLET(
      gtk_builder_get_object(handle->builder, "leaflet_chat")
  );

  handle->flap_user_details = HDY_FLAP(
      gtk_builder_get_object(handle->builder, "flap_user_details")
  );

  handle->title_bar = HDY_HEADER_BAR(
      gtk_builder_get_object(handle->builder, "title_bar")
  );

  g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->title_bar,
      "show-close-button",
      G_BINDING_INVERT_BOOLEAN
  );

  if (app->ui.mobile)
    g_object_bind_property(
      handle->leaflet_chat,
      "folded",
      handle->title_bar,
      "visible",
      G_BINDING_SYNC_CREATE |
      G_BINDING_INVERT_BOOLEAN
    );

  handle->profile_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "profile_avatar")
  );

  handle->profile_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "profile_label")
  );

  handle->profile_key_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "profile_key_label")
  );

  handle->hide_user_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "hide_user_details_button")
  );

  g_signal_connect(
      handle->hide_user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  handle->favourites_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "favourites_button")
  );

  handle->account_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "account_details_button")
  );

  handle->account_details_symbol = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "account_details_symbol")
  );

  handle->account_details_revealer = GTK_REVEALER(
      gtk_builder_get_object(handle->builder, "account_details_revealer")
  );

  g_signal_connect(
      handle->account_details_button,
      "clicked",
      G_CALLBACK(handle_account_details_button_click),
      handle
  );

  handle->accounts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "accounts_listbox")
  );

  handle->add_account_listbox_row = GTK_LIST_BOX_ROW(
      gtk_builder_get_object(handle->builder, "add_account_listbox_row")
  );

  g_signal_connect(
      handle->accounts_listbox,
      "row-activated",
      G_CALLBACK(handle_accounts_listbox_row_activated),
      app
  );

  handle->new_contact_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "new_contact_button")
  );

  handle->new_group_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "new_group_button")
  );

  handle->new_platform_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "new_platform_button")
  );

  g_signal_connect(
      handle->new_contact_button,
      "clicked",
      G_CALLBACK(handle_new_contact_button_click),
      app
  );

  g_signal_connect(
      handle->new_group_button,
      "clicked",
      G_CALLBACK(handle_new_group_button_click),
      app
  );

  g_signal_connect(
      handle->new_platform_button,
      "clicked",
      G_CALLBACK(handle_new_platform_button_click),
      app
  );

  handle->contacts_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "contacts_button")
  );

  handle->settings_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "settings_button")
  );

  g_signal_connect(
      handle->contacts_button,
      "clicked",
      G_CALLBACK(handle_contacts_button_click),
      app
  );

  g_signal_connect(
      handle->settings_button,
      "clicked",
      G_CALLBACK(handle_settings_button_click),
      app
  );

  handle->user_details_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "user_details_button")
  );

  g_signal_connect(
      handle->user_details_button,
      "clicked",
      G_CALLBACK(handle_flap_via_button_click),
      handle->flap_user_details
  );

  handle->chats_search = GTK_SEARCH_ENTRY(
      gtk_builder_get_object(handle->builder, "chats_search")
  );

  handle->chats_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "chats_listbox")
  );

  gtk_list_box_set_filter_func(
      handle->chats_listbox,
      handle_chats_listbox_filter_func,
      handle,
      NULL
  );

  g_signal_connect(
      handle->chats_search,
      "search-changed",
      G_CALLBACK(handle_chats_search_changed),
      handle->chats_listbox
  );

  g_signal_connect(
      handle->chats_listbox,
      "row-activated",
      G_CALLBACK(handle_chats_listbox_row_activated),
      handle
  );

  handle->chats_stack = GTK_STACK(
      gtk_builder_get_object(handle->builder, "chats_stack")
  );

  gtk_widget_show(GTK_WIDGET(handle->main_window));

  g_signal_connect(
      handle->main_window,
      "destroy",
      G_CALLBACK(handle_main_window_destroy),
      app
  );
}

gboolean
ui_messenger_is_context_active(UI_MESSENGER_Handle *handle,
			       struct GNUNET_CHAT_Context *context)
{
  if (!gtk_window_is_active(GTK_WINDOW(handle->main_window)))
    return FALSE;

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(context);

  if (!entry)
    return FALSE;

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
      gtk_widget_get_parent(entry->entry_box)
  );

  return gtk_list_box_row_is_selected(row);
}

void
ui_messenger_cleanup(UI_MESSENGER_Handle *handle)
{
  g_object_unref(handle->builder);

  for (GList *list = handle->chat_entries; list; list = list->next)
    ui_chat_entry_delete((UI_CHAT_ENTRY_Handle*) list->data);

  if (handle->chat_entries)
    g_list_free(handle->chat_entries);

  memset(handle, 0, sizeof(*handle));
}
