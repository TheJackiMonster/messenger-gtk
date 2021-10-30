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

void
application_init(MESSENGER_Application *app,
		 int argc,
		 char **argv)
{
  app->argc = argc;
  app->argv = argv;

  gtk_init(&argc, &argv);

  _load_ui_stylesheets();

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;
  app->chat.signal = MESSENGER_NONE;

  ui_messenger_init(app, &(app->ui.messenger));
}

static void*
_application_chat_thread(void *args)
{
  MESSENGER_Application *app = (MESSENGER_Application*) args;

  struct GNUNET_GETOPT_CommandLineOption options[] = {
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
application_start(MESSENGER_Application *app)
{
  pthread_create(&(app->chat.tid), NULL, _application_chat_thread, app);

  gtk_main();

  pthread_join(app->chat.tid, NULL);
}

typedef struct MESSENGER_ApplicationEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationEvent event;
} MESSENGER_ApplicationEventCall;

static gboolean
_application_event_call(gpointer user_data)
{
  //printf("_application_event_call\n");

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
  //printf("application_call_event\n");

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

  gtk_main_quit();
}

int
application_status(MESSENGER_Application *app)
{
  return app->chat.status;
}
