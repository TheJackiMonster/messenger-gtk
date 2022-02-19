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

#include "bindings.h"
#include "util.h"

MESSENGER_Bindings*
bindings_create()
{
  MESSENGER_Bindings *bindings = GNUNET_new(MESSENGER_Bindings);

  bindings->map = GNUNET_CONTAINER_multishortmap_create(8, GNUNET_NO);

  return bindings;
}

void
bindings_put(MESSENGER_Bindings *bindings,
	     gconstpointer key,
	     gpointer value)
{
  struct GNUNET_ShortHashCode hash;
  memset(&hash, 0, sizeof(hash));
  memcpy(&hash, &key, sizeof(key));

  GNUNET_CONTAINER_multishortmap_put(
      bindings->map,
      &hash,
      (void*) value,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_REPLACE
  );
}

int
_bindings_append_list(void *cls,
		      UNUSED const struct GNUNET_ShortHashCode *key,
		      void *value)
{
  GList **list = (GList**) cls;
  *list = g_list_append(*list, (gpointer) value);
  return GNUNET_YES;
}

void
bindings_remove(MESSENGER_Bindings *bindings,
		const void *key,
		void *value,
		void (destroy)(void*))
{
  struct GNUNET_ShortHashCode hash;
  memset(&hash, 0, sizeof(hash));
  memcpy(&hash, &key, sizeof(key));

  if (value)
  {
    GNUNET_CONTAINER_multishortmap_remove(
	bindings->map,
	&hash,
	(void*) value
    );

    if (destroy)
      destroy(value);
  }
  else
  {
    GList *values = NULL;

    GNUNET_CONTAINER_multishortmap_get_multiple(
	bindings->map,
	&hash,
	_bindings_append_list,
	&values
    );

    GNUNET_CONTAINER_multishortmap_remove_all(
	bindings->map,
	&hash
    );

    if (destroy)
      g_list_free_full(values, destroy);
    else
      g_list_free(values);
  }
}

void*
bindings_get(const MESSENGER_Bindings *bindings,
	     const void *key)
{
  struct GNUNET_ShortHashCode hash;
  memset(&hash, 0, sizeof(hash));
  memcpy(&hash, &key, sizeof(key));

  return GNUNET_CONTAINER_multishortmap_get(
      bindings->map,
      &hash
  );
}

void
bindings_destroy(MESSENGER_Bindings *bindings)
{
  GNUNET_CONTAINER_multishortmap_destroy(bindings->map);

  GNUNET_free(bindings);
}
