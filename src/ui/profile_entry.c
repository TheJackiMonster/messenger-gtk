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
 * @file ui/profile_entry.c
 */

#include "profile_entry.h"

#include "../application.h"

UI_PROFILE_ENTRY_Handle*
ui_profile_entry_new(MESSENGER_Application *app)
{
  UI_PROFILE_ENTRY_Handle* handle = g_malloc(sizeof(UI_PROFILE_ENTRY_Handle));

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/profile_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->entry_avatar = HDY_AVATAR(
      gtk_builder_get_object(handle->builder, "entry_avatar")
  );

  handle->entry_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "entry_label")
  );

  return handle;
}

void
ui_profile_entry_delete(UI_PROFILE_ENTRY_Handle *handle)
{
  g_object_unref(handle->builder);

  g_free(handle);
}
