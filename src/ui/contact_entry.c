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
 * @file ui/contact_entry.c
 */

#include "contact_entry.h"

#include "../application.h"
#include "../ui.h"

UI_CONTACT_ENTRY_Handle*
ui_contact_entry_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_CONTACT_ENTRY_Handle* handle = g_malloc(sizeof(UI_CONTACT_ENTRY_Handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/contact_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->entry_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "entry_avatar")
  );

  handle->title_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "title_label")
  );

  handle->subtitle_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "subtitle_label")
  );

  return handle;
}

void
ui_contact_entry_set_contact(UI_CONTACT_ENTRY_Handle* handle,
                             const struct GNUNET_CHAT_Contact *contact)
{
  g_assert((handle) && (contact));

  const char *name = GNUNET_CHAT_contact_get_name(contact);
  const char *key = GNUNET_CHAT_contact_get_key(contact);

  ui_avatar_set_text(handle->entry_avatar, name);
  ui_label_set_text(handle->title_label, name);
  ui_label_set_text(handle->subtitle_label, key);
}

void
ui_contact_entry_delete(UI_CONTACT_ENTRY_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  g_free(handle);
}
