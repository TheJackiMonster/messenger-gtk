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
 * @file application.c
 */

#include "application.h"

static void
_load_ui_stylesheets(void)
{
  GdkScreen* screen = gdk_screen_get_default();
  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(
      provider,
      "resources/css/style.css",
      NULL
  );

  gtk_style_context_add_provider_for_screen(
      screen,
      GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

static void
_application_activate(UNUSED GtkApplication* application,
		      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_messenger_run(app);
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
      "org.gnunet.MESSENGER-GTK",
      G_APPLICATION_NON_UNIQUE
  );

  notify_init("Messenger-GTK");

  _load_ui_stylesheets();

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;
  app->chat.signal = MESSENGER_NONE;

  app->ui.mobile = FALSE;

  g_application_add_main_option(
      G_APPLICATION(app->application),
      "mobile",
      'm',
      G_OPTION_FLAG_NONE,
      G_OPTION_ARG_NONE,
      "Optimize UI spacing for mobile devices",
      NULL
  );

  g_signal_connect(
      app->application,
      "activate",
      G_CALLBACK(_application_activate),
      app
  );
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
	  &(app->ui.mobile)
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

  pthread_join(app->chat.tid, NULL);

  notify_uninit();

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

  g_idle_add(_application_event_call, call);
}

void
application_exit(MESSENGER_Application *app,
		 MESSENGER_ApplicationSignal signal)
{
  app->chat.signal = signal;
}

int
application_status(MESSENGER_Application *app)
{
  if (EXIT_SUCCESS != app->chat.status)
    return app->chat.status;

  return app->ui.status;
}
