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

  ui_accounts_dialog_init(app, &(app->ui.accounts));
  ui_accounts_dialog_refresh(app, &(app->ui.accounts));

  gtk_widget_show(GTK_WIDGET(app->ui.accounts.dialog));
  return FALSE;
}

static void
_application_activate(UNUSED GtkApplication* application,
		      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_messenger_init(app, &(app->ui.messenger));

  if (app->chat.identity)
    gtk_widget_show(GTK_WIDGET(app->ui.messenger.main_window));
  else
    g_idle_add(G_SOURCE_FUNC(_application_accounts), app);
}

void
application_init(MESSENGER_Application *app,
		 int argc,
		 char **argv)
{
  memset(app, 0, sizeof(*app));

  app->argc = argc;
  app->argv = argv;

  gtk_init(&argc, &argv);
  hdy_init();

  app->application = gtk_application_new(
      "org.gnunet.Messenger",
      G_APPLICATION_NON_UNIQUE
  );

  resources_register();

  notify_init("Messenger-GTK");
  app->notifications = NULL;

  _load_ui_stylesheets(app);

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;
  pipe(app->chat.pipe);

  pthread_mutex_init(&(app->chat.mutex), NULL);

  app->ui.bindings = g_hash_table_new(g_direct_hash, g_direct_equal);

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
      "messenger-gtk",
      gettext_noop("A GTK based GUI for the Messenger service of GNUnet."),
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

  g_hash_table_destroy(app->ui.bindings);

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
  call->event(call->app);

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

  g_idle_add(G_SOURCE_FUNC(_application_event_call), call);
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
