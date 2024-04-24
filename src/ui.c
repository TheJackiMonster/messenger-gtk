/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file ui.c
 */

#include "ui.h"

#include <gnunet/gnunet_common.h>

GtkBuilder*
ui_builder_from_resource(const char *resource_path)
{
  GtkBuilder *builder = gtk_builder_new();

  if (!builder)
    return NULL;

  gtk_builder_set_translation_domain(builder, MESSENGER_APPLICATION_DOMAIN);

  if (!gtk_builder_add_from_resource(builder, resource_path, NULL))
  {
    g_object_unref(builder);
    return NULL;
  }

  return builder;
}

void
ui_label_set_text(GtkLabel *label, const char *text)
{
  g_assert(label);

  if (!text)
  {
    gtk_label_set_text(label, "");
    return;
  }

  gchar *_text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
  gtk_label_set_text(label, _text);
  g_free(_text);
}

void
ui_label_set_size(GtkLabel *label,
                  uint64_t size)
{
  g_assert(label);

  GString* string = g_string_new(NULL);
  
  if (size < 100)
    g_string_printf(string, "%lu B", size);
  else
  {
    char dim = 'K';
    size = (size + 50) / 100;

    while (size >= 1000)
    {
      size = (size + 500) / 1000;
      switch (dim)
      {
        case 'K':
          dim = 'M';
          break;
        case 'M':
          dim = 'G';
          break;
        case 'G':
          dim = 'T';
          break;
        case 'T':
          dim = 'P';
          break;
        default:
          break;
      }
    }

    g_string_printf(
      string,
      "%lu.%lu %cB",
      size / 10,
      size % 10,
      dim
    );
  }

  gtk_label_set_text(label, string->str);
  g_string_free(string, TRUE);
}

void
ui_entry_set_text(GtkEntry *entry, const char *text)
{
  g_assert(entry);

  if (!text)
  {
    gtk_entry_set_text(entry, "");
    return;
  }

  gchar *_text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
  gtk_entry_set_text(entry, _text);
  g_free(_text);
}

char*
ui_entry_get_text(GtkEntry *entry)
{
  g_assert(entry);

  const gchar *text = gtk_entry_get_text(entry);

  if (!text)
    return NULL;

  gchar *_text = g_locale_from_utf8(text, -1, NULL, NULL, NULL);
  char *result = GNUNET_strdup(_text);
  g_free(_text);

  return result;
}

void
ui_avatar_set_text(HdyAvatar *avatar, const char *text)
{
  g_assert(avatar);

  if (!text)
  {
    const gchar *state = hdy_avatar_get_text(avatar);

    if ((!state) || (!g_utf8_strlen(state, 1)))
      return;

    hdy_avatar_set_text(avatar, "");
    return;
  }

  gchar *_text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
  hdy_avatar_set_text(avatar, _text);
  g_free(_text);
}

void
ui_avatar_set_icon(HdyAvatar *avatar,
                   GIcon *icon)
{
  g_assert(avatar);

  if (!icon)
    hdy_avatar_set_loadable_icon(avatar, NULL);
  else
    hdy_avatar_set_loadable_icon(avatar, G_LOADABLE_ICON(icon));
}
