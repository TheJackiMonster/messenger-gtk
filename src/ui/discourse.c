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
#include "discourse_panel.h"

#include "../application.h"
#include "../discourse.h"
#include "../ui.h"
#include "../util.h"

#include <string.h>

static const struct GNUNET_ShortHashCode*
get_voice_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_ShortHashCode id;

  if (GNUNET_YES != init)
  {
    memset(&id, 0, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

static const struct GNUNET_ShortHashCode*
get_video_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_ShortHashCode id;

  if (GNUNET_YES != init)
  {
    memset(&id, 1, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

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
  if (handle->voice_discourse)
    discourse_set_mute(handle->voice_discourse, handle->muted);

  _update_microphone_icon(handle);
}

static void
handle_speakers_button_value_changed(UNUSED GtkScaleButton *button,
                                     gdouble value,
                                     gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (handle->voice_discourse)
    discourse_set_volume(handle->voice_discourse, value);
}

static void
_update_call_button(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  if ((handle->voice_discourse) && 
      (GNUNET_YES ==GNUNET_CHAT_discourse_is_open(handle->voice_discourse)))
    gtk_stack_set_visible_child(handle->call_stack, handle->call_stop_button);
  else
    gtk_stack_set_visible_child(handle->call_stack, handle->call_start_button);
}

static void
handle_call_start_button_click(UNUSED GtkButton *button,
			                         gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (!(handle->context))
    return;

  handle->voice_discourse = GNUNET_CHAT_context_open_discourse(
    handle->context, get_voice_discourse_id()
  );

  _update_call_button(handle);
}

static void
handle_call_stop_button_click(UNUSED GtkButton *button,
			                        gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if ((!(handle->context)) || (!(handle->voice_discourse)))
    return;

  GNUNET_CHAT_discourse_close(handle->voice_discourse);
  handle->voice_discourse = NULL;

  _update_call_button(handle);
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

  handle->discourse_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "discourse_stack")
  );

  handle->offline_page = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "offline_page")
  );

  handle->members_page = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "members_page")
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

  g_signal_connect(
    handle->speakers_button,
    "value-changed",
    G_CALLBACK(handle_speakers_button_value_changed),
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

  handle->call_start_button = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_start_button")
  );

  g_signal_connect(
    handle->call_start_button,
    "clicked",
    G_CALLBACK(handle_call_start_button_click),
    handle
  );

  handle->call_stop_button = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_stop_button")
  );

  g_signal_connect(
    handle->call_stop_button,
    "clicked",
    G_CALLBACK(handle_call_stop_button_click),
    handle
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
append_discourse_members_to_list(void *cls,
                                 UNUSED const struct GNUNET_CHAT_Discourse *discourse,
                                 struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  GList **list = (GList**) cls;
  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
append_discourses_members(void *cls,
                          UNUSED struct GNUNET_CHAT_Context *context,
                          struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (discourse));

  GNUNET_CHAT_discourse_iterate_contacts(
    discourse,
    append_discourse_members_to_list,
    cls
  );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
append_group_contacts(void *cls,
                      UNUSED const struct GNUNET_CHAT_Group *group,
                      struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  GList **list = (GList**) cls;
  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

struct IterateDiscourseClosure {
  MESSENGER_Application *app;
  GtkContainer *container;
};

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_discourse_members(void *cls,
                                              const struct GNUNET_CHAT_Discourse *discourse,
                                              struct GNUNET_CHAT_Contact *contact)
{
  struct IterateDiscourseClosure *closure = (
    (struct IterateDiscourseClosure*) cls
  );

  if (ui_find_qdata_in_container(closure->container, closure->app->quarks.data, contact))
    return GNUNET_YES;

  GtkFlowBox *flowbox = GTK_FLOW_BOX(closure->container);

  UI_DISCOURSE_PANEL_Handle* panel = ui_discourse_panel_new(closure->app);
  ui_discourse_panel_set_contact(panel, contact);

  gtk_flow_box_insert(flowbox, panel->panel_box, -1);

  GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(
    gtk_widget_get_parent(panel->panel_box)
  );

  g_object_set_qdata(G_OBJECT(child), closure->app->quarks.data, contact);
  g_object_set_qdata_full(
    G_OBJECT(child),
    closure->app->quarks.ui,
    panel,
    (GDestroyNotify) ui_discourse_panel_delete
  );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_context_discourses(void *cls,
                                               struct GNUNET_CHAT_Context *context,
                                               struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (context) && (discourse));

  GNUNET_CHAT_discourse_iterate_contacts(
    discourse,
    iterate_ui_discourse_update_discourse_members,
    cls
  );

  return GNUNET_YES;
}

static void
_discourse_update_members(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  GList *list = NULL;
  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    append_discourses_members,
    &list
  );
  
  ui_clear_container_of_missing_qdata(
    GTK_CONTAINER(handle->members_flowbox),
    handle->app->quarks.data,
    list
  );

  if (list)
  {
    gtk_stack_set_visible_child(handle->discourse_stack, handle->members_page);
    g_list_free(list);
  }
  else
    gtk_stack_set_visible_child(handle->discourse_stack, handle->offline_page);

  if (!(handle->context))
    return;

  struct IterateDiscourseClosure closure;
  closure.app = handle->app;
  closure.container = GTK_CONTAINER(handle->members_flowbox);

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    iterate_ui_discourse_update_context_discourses,
    &closure
  );
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_group_contacts(void *cls,
                                           UNUSED const struct GNUNET_CHAT_Group *group,
                                           struct GNUNET_CHAT_Contact *contact)
{
  struct IterateDiscourseClosure *closure = (
    (struct IterateDiscourseClosure*) cls
  );

  if (ui_find_qdata_in_container(closure->container, closure->app->quarks.data, contact))
    return GNUNET_YES;

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

  GList *list = NULL;
  if (group)
    GNUNET_CHAT_group_iterate_contacts(
	    group,
      append_group_contacts,
      &list
    );
  
  ui_clear_container_of_missing_qdata(
    GTK_CONTAINER(handle->contacts_listbox),
    handle->app->quarks.data,
    list
  );

  if (list)
    g_list_free(list);

  if (group)
  {
    struct IterateDiscourseClosure closure;
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

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_search_context_discourses(void *cls,
                                               struct GNUNET_CHAT_Context *context,
                                               struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (context) && (discourse));

  struct GNUNET_CHAT_Discourse **discourses = (struct GNUNET_CHAT_Discourse**) cls;

  const struct GNUNET_ShortHashCode *id = GNUNET_CHAT_discourse_get_id(discourse);

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
    discourses[0] = discourse;
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    discourses[1] = discourse;

  return GNUNET_YES;
}

static void
_update_discourse_via_context(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  handle->voice_discourse = NULL;
  handle->video_discourse = NULL;

  if (!(handle->context))
    return;

  struct GNUNET_CHAT_Discourse *discourses [2];
  memset(discourses, 0, sizeof(struct GNUNET_CHAT_Discourse*) * 2);

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    iterate_ui_discourse_search_context_discourses,
    discourses
  );

  const gboolean has_voice_controls = discourse_has_controls(
    discourses[0]
  );

  const gboolean has_video_controls = discourse_has_controls(
    discourses[1]
  );

  gtk_widget_set_sensitive(GTK_WIDGET(handle->microphone_button), has_voice_controls);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->camera_button), has_video_controls);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->screen_button), has_video_controls);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->speakers_button), has_voice_controls);

  if (discourses[0])
  {
    handle->muted = discourse_is_mute(discourses[0]);

    gtk_scale_button_set_value(
      GTK_SCALE_BUTTON(handle->speakers_button),
      discourse_get_volume(discourses[0])
    );
  }

  handle->voice_discourse = discourses[0];
  handle->video_discourse = discourses[1];
}

void
ui_discourse_window_update(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Context *context)
{
  g_assert(handle);

  handle->context = context;

  _update_discourse_via_context(handle);
  _update_call_button(handle);
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
