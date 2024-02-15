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
 * @file application.c
 */

#include "application.h"
#include "request.h"
#include "resources.h"

#include <gstreamer-1.0/gst/gst.h>
#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>
#include <pipewire/impl.h>

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal-gtk3/portal-gtk3.h>
#endif

static void
_load_ui_stylesheets(MESSENGER_Application *app)
{
  g_assert(app);

  GdkScreen* screen = gdk_screen_get_default();
  GtkCssProvider* provider = gtk_css_provider_new();

  gtk_css_provider_load_from_resource(
    provider,
    application_get_resource_path(app, "css/style.css")
  );

  gtk_style_context_add_provider_for_screen(
    screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

static gboolean
_application_accounts(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->init = 0;

  ui_accounts_dialog_init(app, &(app->ui.accounts));
  ui_accounts_dialog_refresh(app, &(app->ui.accounts));

  gtk_widget_show(GTK_WIDGET(app->ui.accounts.dialog));
  return FALSE;
}

static void
_application_init(MESSENGER_Application *app)
{
  g_assert(app);

  ui_messenger_init(app, &(app->ui.messenger));

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if (app->portal)
    app->parent = xdp_parent_new_gtk(GTK_WINDOW(app->ui.messenger.main_window));
#endif

  if (app->chat.identity)
    application_show_window(app);
  else
    app->init = g_idle_add(G_SOURCE_FUNC(_application_accounts), app);
}

static void
_application_activate(GApplication* application,
		                  gpointer user_data)
{
  g_assert((application) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  g_application_hold(application);

  _application_init(app);

  g_application_release(application);
}

static void
_application_open(GApplication* application,
                  GFile **files,
                  gint n_files,
                  UNUSED gchar* hint,
                  gpointer user_data)
{
  g_assert((application) && (files) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  g_application_hold(application);

  _application_init(app);

  for (gint i = 0; i < n_files; i++) {
    if (!g_file_has_uri_scheme(files[i], "gnunet"))
      continue;

    gchar *uri_string = g_file_get_uri(files[i]);

    if (!uri_string)
      continue;

    char *emsg = NULL;
    struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_uri_parse(uri_string, &emsg);

    if (emsg)
    {
      g_printerr("ERROR: %s\n", emsg);
      GNUNET_free(emsg);
    }

    if (!uri)
      goto free_string;

    GNUNET_CHAT_lobby_join(app->chat.messenger.handle, uri);
    GNUNET_CHAT_uri_destroy(uri);

  free_string:
    g_free(uri_string);
  }

  g_application_release(application);
}

static void
on_core_done(void *data,
             UNUSED uint32_t id,
             int seq)
{
  g_assert(data);

	MESSENGER_Application *app = (MESSENGER_Application*) data;

	if (seq == app->pw.pending)
		pw_main_loop_quit(app->pw.main_loop);
}

static void
on_core_error(void *data,
              UNUSED uint32_t id,
              UNUSED int seq,
              int res,
              const char *message)
{
  g_assert((data) && (message));

	MESSENGER_Application *app = (MESSENGER_Application*) data;

  g_printerr("ERROR: %s\n", message);

	if ((id == PW_ID_CORE) && (res == -EPIPE))
		pw_main_loop_quit(app->pw.main_loop);
}

static const struct pw_core_events remote_core_events = {
	PW_VERSION_CORE_EVENTS,
	.done = on_core_done,
	.error = on_core_error,
};

static void
registry_event_global(void *data,
                      uint32_t id,
                      uint32_t permissions,
                      const char *type,
                      uint32_t version,
                      const struct spa_dict *props)
{
  g_assert(data);

  MESSENGER_Application *app = (MESSENGER_Application*) data;

	if (!props)
    return;

  struct pw_properties *properties = pw_properties_new_dict(props);
  if (!properties)
    return;

  size_t size = pw_map_get_size(&(app->pw.globals));
	while (id > size)
		pw_map_insert_at(&(app->pw.globals), size++, NULL);

	pw_map_insert_at(&(app->pw.globals), id, properties);

  app->pw.pending = pw_core_sync(app->pw.core, 0, 0);
}

static void
registry_event_global_remove(void *data,
                             uint32_t id)
{
  g_assert(data);

  MESSENGER_Application *app = (MESSENGER_Application*) data;

  struct pw_properties *properties = pw_map_lookup(&(app->pw.globals), id);
  if (!properties)
    return;

  pw_map_insert_at(&(app->pw.globals), id, NULL);
	pw_properties_free(properties);
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove,
};

void
application_init(MESSENGER_Application *app,
                 int argc,
                 char **argv)
{
  g_assert((app) && (argv));

  memset(app, 0, sizeof(*app));

  app->argc = argc;
  app->argv = argv;

  pw_init(&argc, &argv);
  gst_init(&argc, &argv);
  gtk_init(&argc, &argv);
  hdy_init();

  app->application = gtk_application_new(
    MESSENGER_APPLICATION_ID,
    G_APPLICATION_HANDLES_OPEN |
    G_APPLICATION_NON_UNIQUE
  );

  resources_register();

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  GError *error = NULL;
  app->portal = xdp_portal_initable_new(&error);

  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);
  }
#endif

  notify_init(MESSENGER_APPLICATION_NAME);
  app->notifications = NULL;
  app->requests = NULL;

  _load_ui_stylesheets(app);

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;
  pipe(app->chat.pipe);

  pthread_mutex_init(&(app->chat.mutex), NULL);

  app->quarks.widget = g_quark_from_string("messenger_widget");
  app->quarks.data = g_quark_from_string("messenger_data");
  app->quarks.ui = g_quark_from_string("messenger_ui");

  app->pw.main_loop = pw_main_loop_new(NULL);
  app->pw.loop = pw_main_loop_get_loop(app->pw.main_loop);

  app->pw.context = pw_context_new(
    app->pw.loop,
    pw_properties_new(
      PW_KEY_CORE_DAEMON,
      NULL,
      NULL
    ),
    0
  );

  pw_context_load_module(app->pw.context, "libpipewire-module-link-factory", NULL, NULL);

  app->pw.core = pw_context_connect(app->pw.context, NULL, 0);
  app->pw.registry = pw_core_get_registry(app->pw.core, PW_VERSION_REGISTRY, 0);

  pw_map_init(&(app->pw.globals), 64, 16);

  pw_core_add_listener(
    app->pw.core,
    &(app->pw.core_listener),
    &remote_core_events,
    app
  );

	pw_registry_add_listener(
    app->pw.registry,
		&(app->pw.registry_listener),
		&registry_events,
    app
  );

  app->pw.pending = pw_core_sync(app->pw.core, 0, 0);

  pw_main_loop_run(app->pw.main_loop);

  g_application_add_main_option(
    G_APPLICATION(app->application),
    "mobile",
    'm',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE,
    "Optimize UI spacing for mobile devices",
    NULL
  );

  g_application_add_main_option(
    G_APPLICATION(app->application),
    "ego",
    'e',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_STRING,
    "Identity to select for messaging",
    "IDENTITY"
  );

  g_signal_connect(
    app->application,
    "activate",
    G_CALLBACK(_application_activate),
    app
  );

  g_signal_connect(
    app->application,
    "open",
    G_CALLBACK(_application_open),
    app
  );
}

const gchar*
application_get_resource_path(MESSENGER_Application *app,
			                        const char *path)
{
  g_assert((app) && (path));

  static gchar resource_path [PATH_MAX];

  const gchar *base_path = g_application_get_resource_base_path(
    G_APPLICATION(app->application)
  );

  snprintf(resource_path, PATH_MAX, "%s/%s", base_path, path);
  return resource_path;
}

static void*
_application_chat_thread(void *args)
{
  g_assert(args);

  MESSENGER_Application *app = (MESSENGER_Application*) args;

  struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_option_flag (
  'm',
  "mobile",
  "Optimize UI spacing for mobile devices",
  &(app->settings.mobile_design)
    ),
    GNUNET_GETOPT_option_string (
'e',
      "ego",
"IDENTITY",
"Identity to select for messaging",
      &(app->chat.identity)
    ),
    GNUNET_GETOPT_OPTION_END
  };

  app->chat.status = (GNUNET_PROGRAM_run(
    app->argc,
    app->argv,
    MESSENGER_APPLICATION_BINARY,
    gettext_noop(MESSENGER_APPLICATION_DESCRIPTION),
    options,
    &chat_messenger_run,
    app
  ) == GNUNET_OK? EXIT_SUCCESS : EXIT_FAILURE);

  return NULL;
}

void
application_run(MESSENGER_Application *app)
{
  g_assert(app);

  // Start thread to run GNUnet scheduler
  pthread_create(
    &(app->chat.tid),
    NULL,
    _application_chat_thread,
    app
  );

  app->ui.status = g_application_run(
    G_APPLICATION(app->application),
    app->argc,
    app->argv
  );

  if (app->ui.status != 0)
    application_exit(app, MESSENGER_FAIL);

  // Wait for other thread to stop properly
  pthread_join(app->chat.tid, NULL);

  close(app->chat.pipe[0]);
  close(app->chat.pipe[1]);

  pthread_mutex_destroy(&(app->chat.mutex));

  GList *list;
  
  // Get rid of open requests
  list = app->requests;

  while (list)
  {
    if (list->data)
    {
      request_cancel((MESSENGER_Request*) list->data);
      request_delete((MESSENGER_Request*) list->data);
    }

    list = list->next;
  }
  
  // Get rid of open notifications
  list = app->notifications;

  while (list)
  {
    if (list->data)
      notify_notification_close(NOTIFY_NOTIFICATION(list->data), NULL);

    list = list->next;
  }

  if (app->requests)
    g_list_free(app->requests);

  if (app->notifications)
    g_list_free(app->notifications);

  notify_uninit();

  resources_unregister();

  g_object_unref(app->application);
}

static void
_request_background_callback(MESSENGER_Application *app,
                             gboolean success,
                             gboolean error,
                             gpointer user_data)
{
  g_assert((app) && (user_data));

  gboolean *setting = (gboolean*) user_data;
  *setting = success;
}

void
application_show_window(MESSENGER_Application *app)
{
  g_assert(app);

  gtk_widget_show(GTK_WIDGET(app->ui.messenger.main_window));

  request_new_background(
    app,
    XDP_BACKGROUND_FLAG_AUTOSTART,
    _request_background_callback,
    &(app->settings.autostart)
  );

  request_new_background(
    app,
    XDP_BACKGROUND_FLAG_ACTIVATABLE,
    _request_background_callback,
    &(app->settings.background_task)
  );
}

typedef struct MESSENGER_ApplicationEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationEvent event;
} MESSENGER_ApplicationEventCall;

static gboolean
_application_event_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) user_data;

  // Locking the mutex for synchronization
  pthread_mutex_lock(&(call->app->chat.mutex));
  call->event(call->app);
  pthread_mutex_unlock(&(call->app->chat.mutex));

  GNUNET_free(call);
  return FALSE;
}

void
application_call_event(MESSENGER_Application *app,
		                   MESSENGER_ApplicationEvent event)
{
  g_assert((app) && (event));

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) GNUNET_malloc(
    sizeof(MESSENGER_ApplicationEventCall)
  );

  call->app = app;
  call->event = event;

  g_timeout_add(0, G_SOURCE_FUNC(_application_event_call), call);
}

typedef struct MESSENGER_ApplicationMessageEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationMessageEvent event;

  struct GNUNET_CHAT_Context *context;
  const struct GNUNET_CHAT_Message *message;
} MESSENGER_ApplicationMessageEventCall;

static gboolean
_application_message_event_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_ApplicationMessageEventCall *call;

  call = (MESSENGER_ApplicationMessageEventCall*) user_data;

  // Locking the mutex for synchronization
  pthread_mutex_lock(&(call->app->chat.mutex));
  call->event(call->app, call->context, call->message);
  pthread_mutex_unlock(&(call->app->chat.mutex));

  GNUNET_free(call);
  return FALSE;
}

void
application_call_message_event(MESSENGER_Application *app,
                               MESSENGER_ApplicationMessageEvent event,
                               struct GNUNET_CHAT_Context *context,
                               const struct GNUNET_CHAT_Message *message)
{
  g_assert((app) && (event) && (message));

  MESSENGER_ApplicationMessageEventCall *call;

  call = (MESSENGER_ApplicationMessageEventCall*) GNUNET_malloc(
    sizeof(MESSENGER_ApplicationMessageEventCall)
  );

  call->app = app;
  call->event = event;

  call->context = context;
  call->message = message;

  g_idle_add(G_SOURCE_FUNC(_application_message_event_call), call);
}

static int
destroy_global(void *obj,
               UNUSED void *data)
{
  struct pw_properties *properties = (struct pw_properties*) obj;

  if (!properties)
    return 0;

	pw_properties_free(properties);
	return 0;
}

void
application_exit(MESSENGER_Application *app,
		             MESSENGER_ApplicationSignal signal)
{
  g_assert(app);

  // Forward a signal to the other thread causing it to shutdown the
  // GNUnet handles of the application.
  write(app->chat.pipe[1], &signal, sizeof(signal));

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if (app->portal)
    g_object_unref(app->portal);

  app->portal = NULL;
#endif

  if (app->pw.registry)
    pw_proxy_destroy((struct pw_proxy*) app->pw.registry);

  if (app->pw.core)
  {
    pw_map_for_each(&(app->pw.globals), destroy_global, NULL);
	  pw_map_clear(&(app->pw.globals));

    pw_core_disconnect(app->pw.core);
  }

  if (app->pw.context)
    pw_context_destroy(app->pw.context);

	if (app->pw.main_loop)
  {
    pw_main_loop_quit(app->pw.main_loop);
    pw_main_loop_destroy(app->pw.main_loop);
  }

  gst_deinit();
  pw_deinit();
}

int
application_status(MESSENGER_Application *app)
{
  g_assert(app);
  
  if (EXIT_SUCCESS != app->chat.status)
    return app->chat.status;

  return app->ui.status;
}
