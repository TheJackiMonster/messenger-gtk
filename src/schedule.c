/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file schedule.c
 */

#include "schedule.h"

#include <glib-2.0/glib-unix.h>
#include <glib-2.0/glib.h>
#include <pthread.h>

void
schedule_init(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  memset(schedule, 0, sizeof(MESSENGER_Schedule));

  g_assert(0 == pipe(schedule->push_pipe));
  g_assert(0 == pipe(schedule->sync_pipe));

  pthread_mutex_init(&(schedule->push_mutex), NULL);
  pthread_mutex_init(&(schedule->sync_mutex), NULL);
}

static gboolean
__schedule_pushed_handling(MESSENGER_Schedule *schedule,
                           MESSENGER_ScheduleSignal val)
{
  g_assert(schedule);

  gboolean keep;

  switch (val)
  {
    case MESSENGER_SCHEDULE_SIGNAL_RUN:
      g_assert(schedule->function);

      keep = schedule->function(schedule->data);

      schedule->function = NULL;
      schedule->data = NULL;
      break;
    case MESSENGER_SCHEDULE_SIGNAL_LOCK:
      g_assert(!(schedule->function));

      keep = TRUE;

      pthread_mutex_unlock(&(schedule->push_mutex));
      pthread_mutex_lock(&(schedule->sync_mutex));
      pthread_mutex_unlock(&(schedule->sync_mutex));
      break;
    default:
      return FALSE;
  }

  return keep;
}

static void
__schedule_setup_push_task(MESSENGER_Schedule *schedule);

static void
__schedule_pushed_task(void *cls)
{
  MESSENGER_Schedule *schedule = cls;
  MESSENGER_ScheduleSignal val;

  g_assert(schedule);
  schedule->task = NULL;

  g_assert(sizeof(val) == read(schedule->push_pipe[0], &val, sizeof(val)));

  if (__schedule_pushed_handling(schedule, val))
    __schedule_setup_push_task(schedule);

  g_assert(sizeof(val) == write(schedule->sync_pipe[1], &val, sizeof(val)));
}

static void
__schedule_setup_push_task(MESSENGER_Schedule *schedule)
{
  struct GNUNET_NETWORK_FDSet *fd = GNUNET_NETWORK_fdset_create ();
  GNUNET_NETWORK_fdset_set_native(fd, schedule->push_pipe[0]);

  schedule->task = GNUNET_SCHEDULER_add_select(
    GNUNET_SCHEDULER_PRIORITY_DEFAULT,
    GNUNET_TIME_relative_get_forever_(),
    fd,
    NULL,
    __schedule_pushed_task,
    schedule
  );

  GNUNET_NETWORK_fdset_destroy(fd);
}

void
schedule_load_gnunet(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  __schedule_setup_push_task(schedule);
}

static gboolean
__schedule_pushed(gint fd,
                  GIOCondition condition,
                  gpointer user_data)
{
  MESSENGER_Schedule *schedule = user_data;
  MESSENGER_ScheduleSignal val;
  gboolean keep;
  guint task;

  g_assert(schedule);
  task = schedule->poll;
  schedule->poll = 0;

  g_assert(sizeof(val) == read(schedule->push_pipe[0], &val, sizeof(val)));

  keep = __schedule_pushed_handling(schedule, val);

  if (keep)
    schedule->poll = task;

  g_assert(sizeof(val) == write(schedule->sync_pipe[1], &val, sizeof(val)));
  return keep;
}

void
schedule_load_glib(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  schedule->poll = g_unix_fd_add(
    schedule->push_pipe[0],
    G_IO_IN,
    __schedule_pushed,
    schedule
  );
}

void
schedule_cleanup(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);

  if (schedule->task)
    GNUNET_SCHEDULER_cancel(schedule->task);
  if (schedule->poll)
    g_source_remove(schedule->poll);

  pthread_mutex_destroy(&(schedule->push_mutex));
  pthread_mutex_destroy(&(schedule->sync_mutex));

  close(schedule->push_pipe[0]);
  close(schedule->push_pipe[1]);

  close(schedule->sync_pipe[0]);
  close(schedule->sync_pipe[1]);

  memset(schedule, 0, sizeof(MESSENGER_Schedule));
}

void
schedule_sync_run(MESSENGER_Schedule *schedule,
                  GSourceFunc function,
                  gpointer data)
{
  g_assert((schedule) && (function));

  schedule->function = function;
  schedule->data = data;

  const MESSENGER_ScheduleSignal push = MESSENGER_SCHEDULE_SIGNAL_RUN;
  MESSENGER_ScheduleSignal sync;

  g_assert(sizeof(push) == write(schedule->push_pipe[1], &push, sizeof(push)));
  g_assert(sizeof(sync) == read(schedule->sync_pipe[0], &sync, sizeof(sync)));
  g_assert(push == sync);
}

void
schedule_sync_lock(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);

  const MESSENGER_ScheduleSignal push = MESSENGER_SCHEDULE_SIGNAL_LOCK;

  pthread_mutex_lock(&(schedule->push_mutex));
  pthread_mutex_lock(&(schedule->sync_mutex));

  g_assert(sizeof(push) == write(schedule->push_pipe[1], &push, sizeof(push)));

  pthread_mutex_lock(&(schedule->push_mutex));
  pthread_mutex_unlock(&(schedule->push_mutex));
}

void
schedule_sync_unlock(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);

  MESSENGER_ScheduleSignal sync;

  pthread_mutex_unlock(&(schedule->sync_mutex));

  g_assert(sizeof(sync) == read(schedule->sync_pipe[0], &sync, sizeof(sync)));
  g_assert(MESSENGER_SCHEDULE_SIGNAL_LOCK == sync);
}
