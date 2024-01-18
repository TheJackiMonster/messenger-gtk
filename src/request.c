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
 * @file request.c
 */

#include "request.h"

MESSENGER_Request*
request_new(MESSENGER_Application *application,
            GCancellable *cancellable,
            gpointer user_data)
{
  MESSENGER_Request* request = g_malloc(sizeof(MESSENGER_Request));

  request->application = application;
  request->cancellable = cancellable;
  request->user_data = user_data;

  application->requests = g_list_append(
    application->requests, 
    request
  );

  return request;
}

MESSENGER_Request*
request_new_background(MESSENGER_Application *application,
                       XdpBackgroundFlags flags,
                       GAsyncReadyCallback callback,
                       gpointer user_data)
{
  GCancellable* cancellable = g_cancellable_new();

  MESSENGER_Request* request = request_new(
    application,
    cancellable,
    user_data
  );

  xdp_portal_request_background(
    application->portal,
    application->parent,
    NULL,
    NULL,
    flags,
    cancellable,
    callback,
    request
  );

  return request;
}

MESSENGER_Request*
request_new_camera(MESSENGER_Application *application,
                   XdpCameraFlags flags,
                   GAsyncReadyCallback callback,
                   gpointer user_data)
{
  GCancellable* cancellable = g_cancellable_new();

  MESSENGER_Request* request = request_new(
    application,
    cancellable,
    user_data
  );

  xdp_portal_access_camera(
    application->portal,
    application->parent,
    flags,
    cancellable,
    callback,
    request
  );

  return request;
}

void
request_cancel(MESSENGER_Request *request)
{
  if (!request->cancellable)
    return;
  
  if (!g_cancellable_is_cancelled(request->cancellable))
    g_cancellable_cancel(request->cancellable);
}

void
request_cleanup(MESSENGER_Request *request)
{
  if (!request->cancellable)
    return;

  g_object_unref(request->cancellable);
  request->cancellable = NULL;
}

void
request_drop(MESSENGER_Request *request)
{
  if (request->application->requests)
    request->application->requests = g_list_remove(
      request->application->requests,
      request
    );

  request_delete(request);
}

void
request_delete(MESSENGER_Request *request)
{
  request_cleanup(request);
  g_free(request);
}
