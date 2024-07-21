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
 * @file schedule.h
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_util_lib.h>
#include <pthread.h>

typedef enum MESSENGER_ScheduleSignal : unsigned char {
  MESSENGER_SCHEDULE_SIGNAL_RUN = 1,
  MESSENGER_SCHEDULE_SIGNAL_LOCK = 2,
} MESSENGER_ScheduleSignal;

typedef struct MESSENGER_Schedule {
  int push_pipe [2];
  int sync_pipe [2];

  pthread_mutex_t push_mutex;
  pthread_mutex_t sync_mutex;
  gboolean locked;

  GSourceFunc function;
  gpointer data;

  struct GNUNET_SCHEDULER_Task *task;
  guint poll;
} MESSENGER_Schedule;

void
schedule_init(MESSENGER_Schedule *schedule);

void
schedule_load_gnunet(MESSENGER_Schedule *schedule);

void
schedule_load_glib(MESSENGER_Schedule *schedule);

void
schedule_cleanup(MESSENGER_Schedule *schedule);

void
schedule_sync_run(MESSENGER_Schedule *schedule,
                  GSourceFunc function,
                  gpointer data);

void
schedule_sync_lock(MESSENGER_Schedule *schedule);

void
schedule_sync_unlock(MESSENGER_Schedule *schedule);

#endif /* SCHEDULE_H_ */
