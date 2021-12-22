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
 * @file contact.h
 */

#ifndef CONTACT_H_
#define CONTACT_H_

#include "application.h"

typedef struct MESSENGER_ContactInfo
{
  GList *name_labels;
  GList *name_avatars;
} MESSENGER_ContactInfo;

void
contact_create_info(struct GNUNET_CHAT_Contact *contact);

void
contact_destroy_info(struct GNUNET_CHAT_Contact *contact);

void
contact_add_name_label_to_info(const struct GNUNET_CHAT_Contact *contact,
			       GtkLabel *label);

void
contact_add_name_avatar_to_info(const struct GNUNET_CHAT_Contact *contact,
				HdyAvatar *avatar);

void
contact_update_info(const struct GNUNET_CHAT_Contact *contact);

#endif /* CONTACT_H_ */
