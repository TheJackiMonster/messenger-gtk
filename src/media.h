/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file media.h
 */

#ifndef MEDIA_H_
#define MEDIA_H_

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal/portal.h>
#endif

#include <pipewire/pipewire.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef struct MESSENGER_MediaInfo
{
  MESSENGER_Application *app;

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  XdpSession *session;
#endif
  
  struct {
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook core_listener;
	  struct spa_hook registry_listener;
    struct pw_map globals;
    int pending;
  } pw;
} MESSENGER_MediaInfo;

typedef void
(*MESSENGER_MediaNodeIterator) (void *cls,
                                const char *name,
                                const char *description,
                                const char *role);

void
media_pw_init(MESSENGER_MediaInfo *media,
              MESSENGER_Application *app,
              int fd);

void
media_init_camera_capturing(MESSENGER_MediaInfo *media,
                            MESSENGER_Application *app);

void
media_init_screen_sharing(MESSENGER_MediaInfo *media,
                          MESSENGER_Application *app);

void
media_pw_cleanup(MESSENGER_MediaInfo *media);

void
media_pw_main_loop_run(MESSENGER_MediaInfo *media);

void
media_pw_iterate_nodes(MESSENGER_MediaInfo *media,
                       MESSENGER_MediaNodeIterator it,
                       void *cls);

#endif /* MEDIA_H_ */
