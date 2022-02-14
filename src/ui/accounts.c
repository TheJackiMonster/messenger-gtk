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
 * @file ui/accounts.c
 */

#include "accounts.h"

#include "account_entry.h"
#include "../application.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
			  gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean
_open_new_account_dialog(gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_account_dialog_init(app, &(app->ui.new_account));

  gtk_widget_show(GTK_WIDGET(app->ui.new_account.dialog));
  return FALSE;
}

static void
handle_accounts_listbox_row_activated(UNUSED GtkListBox* listbox,
				      GtkListBoxRow* row,
				      gpointer user_data)
{
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
  {
    g_idle_add(G_SOURCE_FUNC(_open_new_account_dialog), app);
    goto close_dialog;
  }

  struct GNUNET_CHAT_Account *account = (struct GNUNET_CHAT_Account*) (
      g_hash_table_lookup(app->ui.bindings, row)
  );

  if (!account)
    goto close_dialog;

  // TODO

close_dialog:
  gtk_window_close(GTK_WINDOW(app->ui.accounts.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		      gpointer user_data)
{
  ui_accounts_dialog_cleanup((UI_ACCOUNTS_Handle*) user_data);
}

static int
_iterate_accounts(void *cls,
		  const struct GNUNET_CHAT_Handle *handle,
		  struct GNUNET_CHAT_Account *account)
{
  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  const gchar *name = GNUNET_CHAT_account_get_name(account);

  UI_ACCOUNT_ENTRY_Handle *entry = ui_account_entry_new(app);

  hdy_avatar_set_text(entry->entry_avatar, name);
  gtk_label_set_text(entry->entry_label, name);

  gtk_list_box_prepend(ui->accounts_listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_hash_table_insert(ui->bindings, row, account);

  ui.accounts.account_entries = g_list_append(
      ui.accounts.account_entries,
      entry
  );

  return GNUNET_YES;
}

void
ui_accounts_dialog_init(MESSENGER_Application *app,
			UI_ACCOUNTS_Handle *handle)
{
  handle->account_entries = NULL;
  handle->bindings = app->ui.bindings;

  handle->builder = gtk_builder_new_from_resource(
      application_get_resource_path(app, "ui/accounts.ui")
  );

  handle->dialog = GTK_DIALOG(
      gtk_builder_get_object(handle->builder, "accounts_dialog")
  );

  gtk_window_set_title(
      GTK_WINDOW(handle->dialog),
      _("Contacts")
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->dialog),
      GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->accounts_listbox = GTK_LIST_BOX(
      gtk_builder_get_object(handle->builder, "accounts_listbox")
  );

  g_signal_connect(
      handle->accounts_listbox,
      "row-activated",
      G_CALLBACK(handle_accounts_listbox_row_activated),
      app
  );

  handle->close_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
      handle->close_button,
      "clicked",
      G_CALLBACK(handle_close_button_click),
      handle->dialog
  );

  g_signal_connect(
      handle->dialog,
      "destroy",
      G_CALLBACK(handle_dialog_destroy),
      handle
  );

  GNUNET_CHAT_iterate_accounts(
      app->chat.messenger.handle,
      _iterate_accounts,
      app
  );
}

void
ui_accounts_dialog_cleanup(UI_ACCOUNTS_Handle *handle)
{
  g_object_unref(handle->builder);

  GList *list = handle->account_entries;

  while (list) {
    if (list->data)
      ui_account_entry_delete((UI_ACCOUNT_ENTRY_Handle*) list->data);

    list = list->next;
  }

  if (handle->account_entries)
    g_list_free(handle->account_entries);
}
