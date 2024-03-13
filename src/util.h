/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

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
 * @file util.h
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <glib-2.0/glib.h>

#define UNUSED __attribute__((unused))

#define _(String) (           \
  (const gchar*) g_dgettext(  \
    MESSENGER_APPLICATION_ID, \
    (const gchar*) String     \
  )                           \
)

#endif /* UTIL_H_ */
