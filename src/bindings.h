/*
   This file is part of GNUnet.
   Copyright (C) 2022 GNUnet e.V.

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
 * @file bindings.h
 */

#ifndef BINDINGS_H_
#define BINDINGS_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_container_lib.h>

#include <glib.h>

typedef struct MESSENGER_Bindings
{
  struct GNUNET_CONTAINER_MultiShortmap *map;
} MESSENGER_Bindings;

MESSENGER_Bindings*
bindings_create();

void
bindings_put(MESSENGER_Bindings *bindings,
	     gconstpointer key,
	     gpointer value);

void
bindings_remove(MESSENGER_Bindings *bindings,
		gconstpointer key,
		gpointer value,
		GDestroyNotify destroy);

void*
bindings_get(const MESSENGER_Bindings *bindings,
	     gconstpointer key);

void
bindings_destroy(MESSENGER_Bindings *bindings);

#endif /* BINDINGS_H_ */
