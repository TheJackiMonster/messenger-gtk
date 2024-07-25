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
 * @file discourse.h
 */

#ifndef DISCOURSE_H_
#define DISCOURSE_H_

#include "application.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_chat_lib.h>
#include <pthread.h>

typedef enum MESSENGER_DiscourseControl {
  MESSENGER_DISCOURSE_CTRL_MICROPHONE = 1,
  MESSENGER_DISCOURSE_CTRL_SPEAKERS = 2,
  MESSENGER_DISCOURSE_CTRL_WEBCAM = 3,
  MESSENGER_DISCOURSE_CTRL_SCREEN_CAPTURE = 4,

  MESSENGER_DISCOURSE_CTRL_UNKNOWN = 0
} MESSENGER_DiscourseControl;

typedef struct MESSENGER_DiscourseInfo
{
  struct GNUNET_CHAT_Discourse *discourse;

  GstElement *record_audio_pipeline;
  GstElement *record_audio_sink;

  GstElement *record_video_pipeline;
  GstElement *record_video_sink;

  GstElement *mix_pipeline;
  GstElement *mix_element;
  GstElement *volume_element;

  guint sending_task;
  pthread_mutex_t mutex;

  GList *samples;
  GList *subscriptions;
} MESSENGER_DiscourseInfo;

typedef struct MESSENGER_DiscourseSubscriptionInfo
{
  MESSENGER_DiscourseInfo *discourse;
  struct GNUNET_CHAT_Contact *contact;

  GstElement *stream_source;
  GstElement *converter;

  GstPad *mix_pad;

  uint64_t position;
} MESSENGER_DiscourseSubscriptionInfo;

enum GNUNET_GenericReturnValue
discourse_create_info(struct GNUNET_CHAT_Discourse *discourse);

void
discourse_destroy_info(struct GNUNET_CHAT_Discourse *discourse);

void
discourse_update_subscriptions(struct GNUNET_CHAT_Discourse *discourse);

void
discourse_stream_message(struct GNUNET_CHAT_Discourse *discourse,
                         const struct GNUNET_CHAT_Message *message);

bool
discourse_has_controls(struct GNUNET_CHAT_Discourse *discourse,
                       MESSENGER_DiscourseControl control);

void
discourse_set_volume(struct GNUNET_CHAT_Discourse *discourse,
                     double volume);

double
discourse_get_volume(struct GNUNET_CHAT_Discourse *discourse);

void
discourse_set_mute(struct GNUNET_CHAT_Discourse *discourse,
                   bool mute);

bool
discourse_is_mute(struct GNUNET_CHAT_Discourse *discourse);

#endif /* DISCOURSE_H_ */
