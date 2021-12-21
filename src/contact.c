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
 * @file contact.c
 */

#include "contact.h"

void
contact_create_info(struct GNUNET_CHAT_Contact *contact)
{
  if (GNUNET_CHAT_contact_get_user_pointer(contact))
    return;

  MESSENGER_ContactInfo* info = g_malloc(sizeof(MESSENGER_ContactInfo));

  info->name_labels = NULL;
  info->name_avatars = NULL;

  GNUNET_CHAT_contact_set_user_pointer(contact, info);
}

void
contact_destroy_info(struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  if (info->name_labels)
    g_list_free(info->name_labels);

  if (info->name_avatars)
    g_list_free(info->name_avatars);

  if (info->name_notifications)
    g_list_free(info->name_notifications);

  g_free(info);

  GNUNET_CHAT_contact_set_user_pointer(contact, NULL);
}

void
contact_add_name_label_to_info(const struct GNUNET_CHAT_Contact *contact,
			       GtkLabel *label)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if ((!info) || (!label))
    return;

  const char *name = GNUNET_CHAT_contact_get_name(contact);
  gtk_label_set_text(label, name? name : "");

  info->name_labels = g_list_append(info->name_labels, label);
}

void
contact_add_name_avatar_to_info(const struct GNUNET_CHAT_Contact *contact,
				HdyAvatar *avatar)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if ((!info) || (!avatar))
   return;

  const char *name = GNUNET_CHAT_contact_get_name(contact);
  hdy_avatar_set_text(avatar, name? name : "");

  info->name_avatars = g_list_append(info->name_avatars, avatar);
}

void
contact_update_info(const struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  GList* list;
  const char *name = GNUNET_CHAT_contact_get_name(contact);

  for (list = info->name_labels; list; list = list->next)
    gtk_label_set_text(GTK_LABEL(list->data), name? name : "");

  if (!name)
    return;

  for (list = info->name_avatars; list; list = list->next)
    hdy_avatar_set_text(HDY_AVATAR(list->data), name);
}
