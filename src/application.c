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
 * @file application.c
 */

#include "application.h"
#include "resources.h"

#include <gstreamer-1.0/gst/gst.h>
#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>

static void
_load_ui_stylesheets(MESSENGER_Application *app)
{
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
  ui_messenger_init(app, &(app->ui.messenger));

  if (app->chat.identity)
    gtk_widget_show(GTK_WIDGET(app->ui.messenger.main_window));
  else
    app->init = g_idle_add(G_SOURCE_FUNC(_application_accounts), app);
}

static void
_application_activate(GApplication* application,
		      gpointer user_data)
{
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

void
application_init(MESSENGER_Application *app,
		 int argc,
		 char **argv)
{
  memset(app, 0, sizeof(*app));

  app->argc = argc;
  app->argv = argv;

  gst_init(&argc, &argv);
  gtk_init(&argc, &argv);
  hdy_init();

  app->application = gtk_application_new(
      MESSENGER_APPLICATION_ID,
      G_APPLICATION_HANDLES_OPEN |
      G_APPLICATION_NON_UNIQUE
  );

  resources_register();

  notify_init(MESSENGER_APPLICATION_NAME);
  app->notifications = NULL;

  _load_ui_stylesheets(app);

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;
  pipe(app->chat.pipe);

  pthread_mutex_init(&(app->chat.mutex), NULL);

  app->quarks.widget = g_quark_from_string("messenger_widget");
  app->quarks.data = g_quark_from_string("messenger_data");
  app->quarks.ui = g_quark_from_string("messenger_ui");

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
  pthread_create(&(app->chat.tid), NULL, _application_chat_thread, app);

  app->ui.status = g_application_run(
      G_APPLICATION(app->application),
      app->argc,
      app->argv
  );

  if (app->ui.status != 0)
    application_exit(app, MESSENGER_FAIL);

  pthread_join(app->chat.tid, NULL);

  close(app->chat.pipe[0]);
  close(app->chat.pipe[1]);

  pthread_mutex_destroy(&(app->chat.mutex));

  GList *list = app->notifications;

  while (list) {
    if (list->data)
      notify_notification_close(NOTIFY_NOTIFICATION(list->data), NULL);

    list = list->next;
  }

  if (app->notifications)
    g_list_free(app->notifications);

  notify_uninit();

  resources_unregister();

  g_object_unref(app->application);
}

typedef struct MESSENGER_ApplicationEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationEvent event;
} MESSENGER_ApplicationEventCall;

static gboolean
_application_event_call(gpointer user_data)
{
  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) user_data;

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
  MESSENGER_ApplicationMessageEventCall *call;

  call = (MESSENGER_ApplicationMessageEventCall*) user_data;

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
  MESSENGER_ApplicationMessageEventCall *call;

  if (!event)
    return;

  call = (MESSENGER_ApplicationMessageEventCall*) GNUNET_malloc(
      sizeof(MESSENGER_ApplicationMessageEventCall)
  );

  call->app = app;
  call->event = event;

  call->context = context;
  call->message = message;

  g_idle_add(G_SOURCE_FUNC(_application_message_event_call), call);
}

void
application_exit(MESSENGER_Application *app,
		 MESSENGER_ApplicationSignal signal)
{
  write(app->chat.pipe[1], &signal, sizeof(signal));
}

int
application_status(MESSENGER_Application *app)
{
  if (EXIT_SUCCESS != app->chat.status)
    return app->chat.status;

  return app->ui.status;
}
