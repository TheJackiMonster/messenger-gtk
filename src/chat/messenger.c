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
 * @file chat/messenger.c
 */

#include "messenger.h"

#include "../contact.h"
#include "../event.h"

int
_chat_messenger_destroy_contacts(UNUSED void *cls,
				 UNUSED struct GNUNET_CHAT_Handle *handle,
				 struct GNUNET_CHAT_Contact *contact)
{
  if (contact)
    contact_destroy_info(contact);
  return GNUNET_YES;
}

static void
_chat_messenger_quit(void *cls)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  MESSENGER_ApplicationSignal signal;
  int received = read(app->chat.pipe[0], &signal, sizeof(signal));

  if (received < 0)
    signal = MESSENGER_FAIL;

  GNUNET_CHAT_iterate_contacts(
      app->chat.messenger.handle,
      _chat_messenger_destroy_contacts,
      NULL
  );

  GNUNET_CHAT_stop(app->chat.messenger.handle);
  app->chat.messenger.handle = NULL;

  GNUNET_SCHEDULER_shutdown();
}

static int
_chat_messenger_message(void *cls,
			struct GNUNET_CHAT_Context *context,
			const struct GNUNET_CHAT_Message *message)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  pthread_mutex_lock(&(app->chat.mutex));

  const enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(message);
  const struct GNUNET_CHAT_Contact* sender = GNUNET_CHAT_message_get_sender(message);

  struct GNUNET_TIME_Absolute time = GNUNET_CHAT_message_get_timestamp(message);

  printf("- %d, %lu", kind, time.abs_value_us);

  if (GNUNET_YES == GNUNET_CHAT_message_is_sent(message))
    printf(", [!]\n");
  else if (sender)
    printf(", %s\n", GNUNET_CHAT_contact_get_name(sender));
  else
    printf("\n");

  switch (kind)
  {
    case GNUNET_CHAT_KIND_LOGIN:
    {
      application_call_event(app, event_update_profile);
      break;
    }
    case GNUNET_CHAT_KIND_UPDATE:
    {
      application_call_message_event(
	  app,
	  event_update_chats,
	  context,
	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_JOIN:
    {
      application_call_message_event(
	  app,
	  (GNUNET_YES == GNUNET_CHAT_message_is_sent(message)?
	      event_update_chats :
	      event_joining_contact
	  ),
	  context,
	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_LEAVE:
    {
      // TODO: add status message
      break;
    }
    case GNUNET_CHAT_KIND_CONTACT:
    {
      application_call_message_event(
      	  app,
      	  event_update_contacts,
      	  context,
      	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_INVITATION:
    {
      application_call_message_event(
	  app,
	  event_invitation,
	  context,
	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_TEXT:
    case GNUNET_CHAT_KIND_FILE:
    {
      application_call_message_event(
	  app,
	  event_receive_message,
	  context,
	  message
      );
      break;
    }
    default:
      break;
  }

  pthread_mutex_unlock(&(app->chat.mutex));

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
      ".messenger",
      app->chat.identity,
      &_chat_messenger_message,
      app
  );

  struct GNUNET_NETWORK_FDSet *fd = GNUNET_NETWORK_fdset_create ();
  GNUNET_NETWORK_fdset_set_native(fd, app->chat.pipe[0]);

  app->chat.messenger.quit = GNUNET_SCHEDULER_add_select(
      GNUNET_SCHEDULER_PRIORITY_URGENT,
      GNUNET_TIME_relative_get_forever_(),
      fd,
      NULL,
      &_chat_messenger_quit,
      app
  );

  GNUNET_NETWORK_fdset_destroy(fd);
}
