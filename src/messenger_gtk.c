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
 * @file messenger_gtk.c
 */

#include "chat/messenger.h"
#include "ui/messenger.h"

#include <pthread.h>

#define UNUSED __attribute__((unused))

struct main_program
{
  int argc;
  char** argv;

  bool exit;

  struct CHAT_MESSENGER_Handle chat;
  struct UI_MESSENGER_Handle ui;
};

gboolean gtk_set_profile_name(gpointer user_data)
{
  struct main_program *program = (struct main_program*) user_data;

  const char *name = GNUNET_CHAT_get_name(program->chat.handle);

  if (name)
  {
    hdy_avatar_set_text(program->ui.profile_avatar, name);
    gtk_label_set_text(program->ui.profile_label, name);
  }

  return FALSE;
}

int gnunet_chat_message(void *cls,
			UNUSED struct GNUNET_CHAT_Context *context,
			const struct GNUNET_CHAT_Message *message)
{
  struct main_program *program = (struct main_program*) cls;

  printf("Hello world\n");

  if (GNUNET_CHAT_KIND_LOGIN == GNUNET_CHAT_message_get_kind(message))
    g_idle_add(gtk_set_profile_name, program);

  return GNUNET_YES;
}

void gnunet_idle(void *cls)
{
  struct main_program *program = (struct main_program*) cls;

  if (program->exit)
  {
    GNUNET_CHAT_stop(program->chat.handle);
    program->chat.handle = NULL;

    GNUNET_SCHEDULER_shutdown();
    return;
  }

  program->chat.idle = GNUNET_SCHEDULER_add_delayed_with_priority(
      GNUNET_TIME_relative_get_second_(),
      GNUNET_SCHEDULER_PRIORITY_IDLE,
      gnunet_idle,
      program
  );
}

void gnunet_task(void *cls,
		 UNUSED char *const *args,
		 UNUSED const char *cfgfile,
		 const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  struct main_program *program = (struct main_program*) cls;

  program->chat.handle = GNUNET_CHAT_start(
      cfg,
      "messenger-gtk",
      "test",
      &gnunet_chat_message,
      program
  );

  program->chat.idle = GNUNET_SCHEDULER_add_delayed_with_priority(
      GNUNET_TIME_relative_get_zero_(),
      GNUNET_SCHEDULER_PRIORITY_IDLE,
      gnunet_idle,
      program
  );
}

void *gnunet_thread(void *args)
{
  struct main_program *program = (struct main_program*) args;

  struct GNUNET_GETOPT_CommandLineOption options[] = {
      GNUNET_GETOPT_OPTION_END
  };

  GNUNET_PROGRAM_run(
      program->argc,
      program->argv,
      "messenger-gtk",
      gettext_noop("A GTK based GUI for the Messenger service of GNUnet."),
      options,
      &gnunet_task,
      program
  );

  return NULL;
}

int main(int argc, char **argv) {
  struct main_program program;
  program.argc = argc;
  program.argv = argv;

  program.exit = FALSE;

  pthread_t gnunet_tid;
  gtk_init(&argc, &argv);

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

  ui_messenger_init(&(program.ui));

  pthread_create(&gnunet_tid, NULL, gnunet_thread, &program);

  gtk_main();

  program.exit = TRUE;

  pthread_join(gnunet_tid, NULL);
  return 0;
}
