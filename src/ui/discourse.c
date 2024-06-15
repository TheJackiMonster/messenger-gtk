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
 * @file ui/discourse.c
 */

#include "discourse.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_time_lib.h>

#include "account_entry.h"

#include "../application.h"
#include "../ui.h"
#include "../util.h"
#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>
#include <string.h>

static void
handle_back_button_click(UNUSED GtkButton *button,
			                   gpointer user_data)
{
  g_assert(user_data);

  GtkWindow *window = GTK_WINDOW(user_data);
  gtk_window_close(window);
}

static void
handle_details_button_click(UNUSED GtkButton *button,
			                      gpointer user_data)
{
  g_assert(user_data);

  HdyFlap *flap = HDY_FLAP(user_data);

  hdy_flap_set_reveal_flap(flap, !hdy_flap_get_reveal_flap(flap));
}

static void
handle_details_folded(GObject* object,
                      GParamSpec* pspec,
                      gpointer user_data)
{
  g_assert((object) && (pspec) && (user_data));

  HdyFlap* flap = HDY_FLAP(object);
  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  const gboolean revealed = hdy_flap_get_reveal_flap(flap);

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->back_button),
    !revealed
  );
}

static void
_update_microphone_icon(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  if (handle->muted)
    gtk_stack_set_visible_child(handle->microphone_stack, handle->microphone_off_icon);
  else
    gtk_stack_set_visible_child(handle->microphone_stack, handle->microphone_on_icon);
}

static void
handle_microphone_button_click(UNUSED GtkButton *button,
			                         gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  handle->muted = !(handle->muted);
  _update_microphone_icon(handle);
}

static void
handle_call_button_click(UNUSED GtkButton *button,
			                   gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (!(handle->context))
    return;

  const gboolean calling = (
    (handle->voice_discourse) && 
    (GNUNET_YES == GNUNET_CHAT_discourse_is_open(handle->voice_discourse))
  );

  struct GNUNET_ShortHashCode voice_id;
  memset(&voice_id, 0, sizeof(voice_id));

  if (calling)
  {
    GNUNET_CHAT_discourse_close(handle->voice_discourse);
    handle->voice_discourse = NULL;
  }
  else
    handle->voice_discourse = GNUNET_CHAT_context_open_discourse(
      handle->context, &voice_id
    );

  if (handle->voice_discourse)
    gtk_stack_set_visible_child(handle->call_stack, handle->call_stop_icon);
  else
    gtk_stack_set_visible_child(handle->call_stack, handle->call_start_icon);
}

static void
handle_window_destroy(UNUSED GtkWidget *window,
		                  gpointer user_data)
{
  g_assert(user_data);

  ui_discourse_window_cleanup((UI_DISCOURSE_Handle*) user_data);
}

void
ui_discourse_window_init(MESSENGER_Application *app,
                         UI_DISCOURSE_Handle *handle)
{
  g_assert((app) && (handle));

  handle->app = app;
  handle->context = NULL;

  handle->voice_discourse = NULL;
  handle->muted = TRUE;

  handle->parent = GTK_WINDOW(app->ui.messenger.main_window);

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/discourse.ui")
  );

  handle->window = HDY_WINDOW(
    gtk_builder_get_object(handle->builder, "discourse_window")
  );

  gtk_window_set_position(
    GTK_WINDOW(handle->window),
    GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->window),
    handle->parent
  );

  handle->title_bar = HDY_HEADER_BAR(
    gtk_builder_get_object(handle->builder, "title_bar")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    handle->window
  );

  handle->details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "details_button")
  );

  handle->details_flap = HDY_FLAP(
    gtk_builder_get_object(handle->builder, "details_flap")
  );

  g_signal_connect(
    handle->details_button,
    "clicked",
    G_CALLBACK(handle_details_button_click),
    handle->details_flap
  );

  g_signal_connect(
    handle->details_flap,
    "notify::reveal-flap",
    G_CALLBACK(handle_details_folded),
    handle
  );

  handle->members_flowbox = GTK_FLOW_BOX(
    gtk_builder_get_object(handle->builder, "members_flowbox")
  );

  handle->microphone_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "microphone_button")
  );

  g_signal_connect(
    handle->microphone_button,
    "clicked",
    G_CALLBACK(handle_microphone_button_click),
    handle
  );

  handle->camera_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "camera_button")
  );

  handle->screen_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "screen_button")
  );

  handle->speakers_button = GTK_VOLUME_BUTTON(
    gtk_builder_get_object(handle->builder, "speakers_button")
  );

  handle->call_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "call_button")
  );

  g_signal_connect(
    handle->call_button,
    "clicked",
    G_CALLBACK(handle_call_button_click),
    handle
  );

  handle->microphone_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "microphone_stack")
  );

  handle->microphone_on_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "microphone_on_icon")
  );

  handle->microphone_off_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "microphone_off_icon")
  );

  handle->call_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "call_stack")
  );

  handle->call_start_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_start_icon")
  );

  handle->call_stop_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_stop_icon")
  );

  handle->close_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "close_details_button")
  );

  g_signal_connect(
    handle->close_details_button,
    "clicked",
    G_CALLBACK(handle_details_button_click),
    handle->details_flap
  );

  handle->contacts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "contacts_listbox")
  );

  g_signal_connect(
    handle->window,
    "destroy",
    G_CALLBACK(handle_window_destroy),
    handle
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_discourse_members(void *cls,
                                              const struct GNUNET_CHAT_Discourse *discourse,
                                              struct GNUNET_CHAT_Contact *contact)
{
  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) cls;

  printf("%s\n", GNUNET_CHAT_contact_get_name(contact));

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_context_discourses(void *cls,
                                               UNUSED struct GNUNET_CHAT_Context *context,
                                               struct GNUNET_CHAT_Discourse *discourse)
{
  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) cls;

  GNUNET_CHAT_discourse_iterate_contacts(
    discourse,
    iterate_ui_discourse_update_discourse_members,
    handle
  );

  return GNUNET_YES;
}

static void
_discourse_update_members(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  if (!(handle->context))
    return;

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    iterate_ui_discourse_update_context_discourses,
    handle
  );
}

struct IterateChatClosure {
  MESSENGER_Application *app;
  GtkContainer *container;
};

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_group_contacts(void *cls,
                                           UNUSED const struct GNUNET_CHAT_Group *group,
                                           struct GNUNET_CHAT_Contact *contact)
{
  struct IterateChatClosure *closure = (
    (struct IterateChatClosure*) cls
  );

  GtkListBox *listbox = GTK_LIST_BOX(closure->container);
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

static void
_discourse_update_contacts(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Group* group)
{
  g_assert((handle) && (handle->app));

  GList* children = gtk_container_get_children(
    GTK_CONTAINER(handle->contacts_listbox)
  );

  GList *item = children;
  while ((item) && (item->next)) {
    GtkWidget *widget = GTK_WIDGET(item->data);
    item = item->next;

    gtk_container_remove(
      GTK_CONTAINER(handle->contacts_listbox),
      widget
    );
  }

  if (children)
    g_list_free(children);

  if (group)
  {
    struct IterateChatClosure closure;
    closure.app = handle->app;
    closure.container = GTK_CONTAINER(handle->contacts_listbox);

    GNUNET_CHAT_group_iterate_contacts(
	    group,
      iterate_ui_discourse_update_group_contacts,
      &closure
    );
  }

  gtk_widget_set_visible(
    GTK_WIDGET(handle->details_button),
    group? TRUE : FALSE
  );
}

void
ui_discourse_window_update(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Context *context)
{
  g_assert(handle);

  if (handle->context)
  {
    // TODO


  }

  handle->context = context;

  _update_microphone_icon(handle);
  _discourse_update_members(handle);

  struct GNUNET_CHAT_Group* group = GNUNET_CHAT_context_get_group(
    handle->context
  );

  _discourse_update_contacts(handle, group);
}

void
ui_discourse_window_cleanup(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  ui_discourse_window_update(handle, NULL);

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
