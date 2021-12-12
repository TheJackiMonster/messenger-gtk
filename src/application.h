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
 * @file application.h
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <pthread.h>
#include <gnunet/gnunet_chat_lib.h>

#include "chat/messenger.h"

#include "ui/contacts.h"
#include "ui/messenger.h"
#include "ui/new_contact.h"
#include "ui/new_group.h"
#include "ui/new_platform.h"
#include "ui/new_profile.h"
#include "ui/settings.h"

#include "util.h"

typedef enum MESSENGER_ApplicationSignal
{
  MESSENGER_NONE = 0,
  MESSENGER_QUIT = 1,
  MESSENGER_FAIL = 2
} MESSENGER_ApplicationSignal;

typedef struct MESSENGER_Application
{
  char** argv;
  int argc;

  GtkApplication* application;

  struct {
    int status;
    pthread_t tid;
    char *identity;

    MESSENGER_ApplicationSignal signal;

    CHAT_MESSENGER_Handle messenger;
  } chat;

  struct {
    int status;
    gboolean mobile;

    GHashTable *bindings;

    UI_MESSENGER_Handle messenger;

    UI_NEW_CONTACT_Handle new_contact;
    UI_NEW_GROUP_Handle new_group;
    UI_NEW_PLATFORM_Handle new_platform;
    UI_NEW_PROFILE_Handle new_profile;
    UI_CONTACTS_Handle contacts;
    UI_SETTINGS_Handle settings;
  } ui;
} MESSENGER_Application;

void
application_init(MESSENGER_Application *app,
		 int argc,
		 char **argv);

void
application_run(MESSENGER_Application *app);

typedef void (*MESSENGER_ApplicationEvent) (
    MESSENGER_Application *app
);

typedef void (*MESSENGER_ApplicationMessageEvent) (
    MESSENGER_Application *app,
    struct GNUNET_CHAT_Context *context,
    const struct GNUNET_CHAT_Message *msg
);

void
application_call_event(MESSENGER_Application *app,
		       MESSENGER_ApplicationEvent event);

void
application_call_message_event(MESSENGER_Application *app,
			       MESSENGER_ApplicationMessageEvent event,
			       struct GNUNET_CHAT_Context *context,
                               const struct GNUNET_CHAT_Message *message);

void
application_exit(MESSENGER_Application *app,
		 MESSENGER_ApplicationSignal signal);

int
application_status(MESSENGER_Application *app);

#endif /* APPLICATION_H_ */
