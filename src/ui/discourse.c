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

#include "account_entry.h"

#include "../application.h"
#include "../ui.h"
#include "../util.h"

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
