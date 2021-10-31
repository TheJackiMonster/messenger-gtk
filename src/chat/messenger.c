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
 * @file chat/messenger.c
 */

#include "messenger.h"

#include "../event.h"

static void
_chat_messenger_idle(void *cls)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (MESSENGER_NONE == app->chat.signal)
  {
    app->chat.messenger.idle = GNUNET_SCHEDULER_add_delayed_with_priority(
	GNUNET_TIME_relative_get_second_(),
	GNUNET_SCHEDULER_PRIORITY_IDLE,
	&_chat_messenger_idle,
	app
    );

    return;
  }

  GNUNET_CHAT_stop(app->chat.messenger.handle);
  app->chat.messenger.handle = NULL;

  if (MESSENGER_QUIT != app->chat.signal)
    GNUNET_SCHEDULER_shutdown();
}

static int
_chat_messenger_message(void *cls,
			UNUSED struct GNUNET_CHAT_Context *context,
			const struct GNUNET_CHAT_Message *message)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (GNUNET_CHAT_KIND_LOGIN == GNUNET_CHAT_message_get_kind(message))
    application_call_event(app, event_update_profile);
  else if (GNUNET_CHAT_KIND_TEXT == GNUNET_CHAT_message_get_kind(message))
    printf("%s\n", GNUNET_CHAT_message_get_text(message));

  return GNUNET_YES;
}

void
chat_messenger_run(void *cls,
		   UNUSED char *const *args,
		   UNUSED const char *cfgfile,
		   const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  app->chat.messenger.handle = GNUNET_CHAT_start(
      cfg,
      "messenger-gtk",
      "test",
      &_chat_messenger_message,
      app
  );

  app->chat.messenger.idle = GNUNET_SCHEDULER_add_delayed_with_priority(
      GNUNET_TIME_relative_get_zero_(),
      GNUNET_SCHEDULER_PRIORITY_IDLE,
      &_chat_messenger_idle,
      app
  );
}
