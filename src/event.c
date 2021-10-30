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
 * @file event.c
 */

#include "event.h"

void
event_update_profile(MESSENGER_Application *app)
{
  UI_MESSENGER_Handle* ui = &(app->ui.messenger);

  const char *name = GNUNET_CHAT_get_name(app->chat.messenger.handle);

  if (name)
  {
    hdy_avatar_set_text(ui->profile_avatar, name);
    gtk_label_set_text(ui->profile_label, name);
  }
}
