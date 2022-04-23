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
 * @file application.h
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <pthread.h>
#include <gnunet/gnunet_chat_lib.h>

#include "chat/messenger.h"

#include "ui/about.h"
#include "ui/accounts.h"
#include "ui/contact_info.h"
#include "ui/contacts.h"
#include "ui/delete_messages.h"
#include "ui/invite_contact.h"
#include "ui/messenger.h"
#include "ui/new_account.h"
#include "ui/new_contact.h"
#include "ui/new_group.h"
#include "ui/new_lobby.h"
#include "ui/new_platform.h"
#include "ui/send_file.h"
#include "ui/settings.h"

#include "util.h"

#define MESSENGER_APPLICATION_APPNAME "GNUnet Messenger"
#define MESSENGER_APPLICATION_BINARY "messenger-gtk"
#define MESSENGER_APPLICATION_ID "org.gnunet.Messenger"
#define MESSENGER_APPLICATION_NAME "Messenger-GTK"
#define MESSENGER_APPLICATION_DESCRIPTION \
  "A GTK based GUI for the Messenger service of GNUnet."
#define MESSENGER_APPLICATION_TITLE "Messenger"
#define MESSENGER_APPLICATION_SUBTITLE "GNUnet"
#define MESSENGER_APPLICATION_VERSION "0.7.0"

typedef enum MESSENGER_ApplicationSignal
{
  MESSENGER_NONE = 0,
  MESSENGER_QUIT = 1,
  MESSENGER_FAIL = 2
} MESSENGER_ApplicationSignal;

typedef struct MESSENGER_Application
{
  char **argv;
  int argc;

  GtkApplication *application;
  GList *notifications;
  guint init;

  struct {
    GQuark widget;
    GQuark data;
    GQuark ui;
  } quarks;

  struct {
    int status;
    pthread_t tid;
    char *identity;

    int pipe [2];
    pthread_mutex_t mutex;

    CHAT_MESSENGER_Handle messenger;
  } chat;

  struct {
    int status;

    UI_MESSENGER_Handle messenger;
    UI_ABOUT_Handle about;

    UI_CONTACT_INFO_Handle contact_info;
    UI_DELETE_MESSAGES_Handle delete_messages;
    UI_INVITE_CONTACT_Handle invite_contact;
    UI_SEND_FILE_Handle send_file;

    UI_NEW_CONTACT_Handle new_contact;
    UI_NEW_GROUP_Handle new_group;
    UI_NEW_LOBBY_Handle new_lobby;
    UI_NEW_PLATFORM_Handle new_platform;
    UI_NEW_ACCOUNT_Handle new_account;

    UI_ACCOUNTS_Handle accounts;
    UI_CONTACTS_Handle contacts;
    UI_SETTINGS_Handle settings;
  } ui;

  struct {
    gboolean mobile_design;
    gboolean hide_delete_dialog;

    gboolean disable_notifications;

    gboolean send_read_receipts;
    gboolean show_whispering;

    gulong auto_delete_delay;

    gboolean accept_all_invitations;
    gulong delete_invitations_delay;

    gboolean accept_all_files;
    gchar *download_folder_path;
    gulong delete_files_delay;

    gulong leave_chats_delay;
  } settings;
} MESSENGER_Application;

/**
 * Initializes the messenger application with
 * startup arguments.
 *
 * @param app Messenger application
 * @param argc Amount of arguments
 * @param argv Arguments
 */
void
application_init(MESSENGER_Application *app,
		 int argc,
		 char **argv);

/**
 * Returns the path from resources of the
 * messenger application relative to its storage.
 *
 * @param app Messenger application
 * @param path Path
 * @return Resource path
 */
const gchar*
application_get_resource_path(MESSENGER_Application *app,
			      const char *path);

/**
 * Runs the messenger application starting the
 * second thread and waiting for the application
 * to finish.
 *
 * @param app Messenger application
 */
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

/**
 * Calls a given event with the messenger application
 * asyncronously but explicitly synchronized via mutex.
 *
 * @param app Messenger application
 * @param event Event
 */
void
application_call_event(MESSENGER_Application *app,
		       MESSENGER_ApplicationEvent event);

/**
 * Calls a given message event with the messenger
 * application asyncronously but explicitly synchronized
 * via mutex.
 *
 * @param app Messenger application
 * @param event Message event
 * @param context Chat context
 * @param message Message
 */
void
application_call_message_event(MESSENGER_Application *app,
			       MESSENGER_ApplicationMessageEvent event,
			       struct GNUNET_CHAT_Context *context,
                               const struct GNUNET_CHAT_Message *message);

/**
 * Signals the second thread to exit the application.
 *
 * @param app Messenger application
 * @param signal Exit signal
 */
void
application_exit(MESSENGER_Application *app,
		 MESSENGER_ApplicationSignal signal);

/**
 * Returns the exit status of the messenger application.
 *
 * @param app Messenger application
 * @return Exit status
 */
int
application_status(MESSENGER_Application *app);

#endif /* APPLICATION_H_ */
