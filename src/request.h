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
 * @file request.h
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <gio/gio.h>
#include <libportal/portal.h>

#include "application.h"

typedef struct MESSENGER_Request {
  MESSENGER_Application *application;
  GCancellable *cancellable;
  gpointer user_data;
} MESSENGER_Request;

MESSENGER_Request*
request_new(MESSENGER_Application *application,
            GCancellable *cancellable,
            gpointer user_data);

MESSENGER_Request*
request_new_background(MESSENGER_Application *application,
                       XdpBackgroundFlags flags,
                       GAsyncReadyCallback callback,
                       gpointer user_data);

void
request_cancel(MESSENGER_Request *request);

void
request_cleanup(MESSENGER_Request *request);

void
request_delete(MESSENGER_Request *request);

#endif /* REQUEST_H_ */
